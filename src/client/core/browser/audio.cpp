#include "audio.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <AL/alc.h>

bool AudioManager::Initialize()
{
    if (device_)
        return true;

    device_ = alcOpenDevice(nullptr);
    if (!device_) {
        LOG_ERROR_EX("Failed to open OpenAL audio device");
        return false;
    }

    context_ = alcCreateContext(static_cast<ALCdevice*>(device_), nullptr);
    if (!context_ || !alcMakeContextCurrent(static_cast<ALCcontext*>(context_))) {
        LOG_ERROR_EX("Failed to create OpenAL audio context");

        if (context_)
            alcDestroyContext(static_cast<ALCcontext*>(context_));
        context_ = nullptr;

        if (device_)
            alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;

        return false;
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    LOG_INFO("OpenAL initialized successfully");

    terminate_ = false;
    audio_thread_ = std::thread(&AudioManager::AudioThreadLoop, this);
    LOG_INFO("Audio thread started");

    return true;
}

void AudioManager::Shutdown()
{
    if (terminate_.exchange(true))
        return;

    LOG_INFO("Shutting down audio system...");

    cv_.notify_all();

    if (audio_thread_.joinable()) {
        audio_thread_.join();
    }

    LOG_INFO("Audio thread finished");
}

void AudioManager::OnPcmPacket(int browserId, const float** data, int frames, int channels, int sampleRate)
{
    if (!device_ || !context_)
        return;

    if (terminate_ || frames == 0)
        return;

    AudioPacket packet;
    packet.browserId = browserId;
    packet.sampleRate = sampleRate;
    packet.pcm_data.resize(frames);

    if (channels == 1) {
        for (int i = 0; i < frames; ++i) {
            const float s = std::max(-1.0f, std::min(1.0f, data[0][i]));
            packet.pcm_data[i] = static_cast<int16_t>(s * 32767.0f);
        }
    }
    else {
        for (int i = 0; i < frames; ++i) {
            float mix = 0.0f;
            for (int ch = 0; ch < channels; ++ch) mix += data[ch][i];
            mix /= static_cast<float>(channels);
            mix = std::max(-1.0f, std::min(1.0f, mix));
            packet.pcm_data[i] = static_cast<int16_t>(mix * 32767.0f);
        }
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        packet_queue_.push(std::move(packet));
    }

    cv_.notify_one();
}

void AudioManager::EnsureStream(int browserId)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    if (streams_.count(browserId))
        return;

    streams_[browserId];
    LOG_DEBUG("Audio stream structure prepared for browserId={}", browserId);
}

void AudioManager::RemoveStream(int browserId)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto it = streams_.find(browserId);
    if (it != streams_.end()) {
        it->second.pending_destroy.store(true);
        LOG_DEBUG("Stream for browserId={} scheduled for deferred deletion", browserId);
    }
}

void AudioManager::UpdateSourcePosition(int browserId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);

    auto it = streams_.find(browserId);
    if (it != streams_.end()) {
        it->second.pos_x.store(x);
        it->second.pos_y.store(y);
        it->second.pos_z.store(z);
        it->second.position_dirty.store(true);
    }
}

void AudioManager::UpdateListenerPosition(float x, float y, float z, float at_x, float at_y, float at_z, float up_x, float up_y, float up_z)
{
    listener_pos_x_.store(x);
    listener_pos_y_.store(y);
    listener_pos_z_.store(z);

    // Invert forward vector to match GTA coordinates
    listener_at_x_.store(-at_x);
    listener_at_y_.store(-at_y);
    listener_at_z_.store(-at_z);

    listener_up_x_.store(up_x);
    listener_up_y_.store(up_y);
    listener_up_z_.store(up_z);

    listener_dirty_.store(true);
}

void AudioManager::SetStreamMuted(int browserId, bool muted)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = streams_.find(browserId);
    if (it != streams_.end()) it->second.is_muted.store(muted);
}

void AudioManager::SetStreamAudioSettings(int browserId, float max_distance, float ref_distance)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = streams_.find(browserId);
    if (it != streams_.end()) {
        it->second.max_distance.store(max_distance);
        it->second.ref_distance.store(ref_distance);
        it->second.settings_dirty.store(true);
    }
}

void AudioManager::SetStreamAudioMode(int browserId, AudioMode mode)
{
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = streams_.find(browserId);
    if (it != streams_.end()) {
        it->second.audio_mode.store(mode);
        it->second.settings_dirty.store(true);
    }
}

void AudioManager::AudioThreadLoop()
{
    // This thread must own the OpenAL context
    alcMakeContextCurrent(static_cast<ALCcontext*>(context_));
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    while (!terminate_) {
        // Update listener and stream states
        {
            std::lock_guard<std::mutex> lock(streams_mutex_);

            // Apply listener position/orientation changes
            if (listener_dirty_.exchange(false)) {
                alListener3f(AL_POSITION, listener_pos_x_.load(), listener_pos_y_.load(), listener_pos_z_.load());
                float orientation[6] = {
                    listener_at_x_.load(), listener_at_y_.load(), listener_at_z_.load(),
                    listener_up_x_.load(), listener_up_y_.load(), listener_up_z_.load()
                };
                alListenerfv(AL_ORIENTATION, orientation);
            }

            // Process stream updates and handle deferred destruction
            std::vector<int> to_erase; to_erase.reserve(streams_.size());
            for (auto& [id, stream] : streams_) {
                // Handle safe destruction on audio thread
                if (stream.pending_destroy.load()) {
                    if (stream.source != 0) {
                        alSourceStop(stream.source);
                        // Unqueue all remaining buffers before deletion
                        ALint queued = 0; alGetSourcei(stream.source, AL_BUFFERS_QUEUED, &queued);
                        while (queued-- > 0) { ALuint buf; alSourceUnqueueBuffers(stream.source, 1, &buf); }
                        alDeleteSources(1, &stream.source); stream.source = 0;
                        if (!stream.buffers.empty()) { alDeleteBuffers((ALsizei)stream.buffers.size(), stream.buffers.data()); stream.buffers.clear(); }
                    }
                    to_erase.push_back(id); continue;
                }

                if (stream.source == 0) continue;

                // Apply audio mode and distance settings
                if (stream.settings_dirty.exchange(false)) {
                    const int mode = stream.audio_mode.load();
                    if (mode == AUDIO_MODE_UI) {
                        alSourcei(stream.source, AL_SOURCE_RELATIVE, AL_TRUE);
                        alSource3f(stream.source, AL_POSITION, 0.0f, 0.0f, 0.0f);
                    } else {
                        alSourcei(stream.source, AL_SOURCE_RELATIVE, AL_FALSE);
                        stream.position_dirty.store(true);
                    }
                    alSourcef(stream.source, AL_MAX_DISTANCE, stream.max_distance.load());
                    alSourcef(stream.source, AL_REFERENCE_DISTANCE, stream.ref_distance.load());
                    alSourcef(stream.source, AL_ROLLOFF_FACTOR, 4.0f);
                }

                // Update 3D position
                if (stream.position_dirty.exchange(false)) {
                    alSource3f(stream.source, AL_POSITION, stream.pos_x.load(), stream.pos_y.load(), stream.pos_z.load());
                }

                // Distance culling (world mode only)
                const float max_dist = stream.max_distance.load();
                const float dx = listener_pos_x_.load() - stream.pos_x.load();
                const float dy = listener_pos_y_.load() - stream.pos_y.load();
                const float dz = listener_pos_z_.load() - stream.pos_z.load();
                const float distance_sq = dx*dx + dy*dy + dz*dz;
                const bool should_be_culled = (distance_sq > max_dist*max_dist) && (stream.audio_mode.load() == AUDIO_MODE_WORLD);
                const bool is_culled = stream.is_culled.load();
                if (should_be_culled && !is_culled) { alSourcePause(stream.source); stream.is_culled.store(true); }
                else if (!should_be_culled && is_culled) { alSourcePlay(stream.source); stream.is_culled.store(false); }
            }
            for (int id : to_erase) { streams_.erase(id); LOG_DEBUG("Stream for browserId={} destroyed and removed", id); }
        }

        // Process incoming audio packets
        AudioPacket packet;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            constexpr auto timeout = std::chrono::milliseconds(16); // ~60fps update rate
            if (cv_.wait_for(lock, timeout, [this]{ return !packet_queue_.empty() || terminate_; })) {
                if (terminate_) break;
                packet = std::move(packet_queue_.front());
                packet_queue_.pop();
            } else {
                continue;
            }
        }

        // Find target stream (skip if muted or destroyed)
        AudioStream* stream_ptr = nullptr;
        {
            std::lock_guard<std::mutex> lock(streams_mutex_);
            auto it = streams_.find(packet.browserId);
            if (it == streams_.end() || it->second.is_muted.load() || it->second.pending_destroy.load()) continue;
            stream_ptr = &it->second;
            // Lazy OpenAL resource allocation
            if (stream_ptr->source == 0) {
                alGenSources(1, &stream_ptr->source);
                stream_ptr->buffers.resize(4);
                alGenBuffers((ALsizei)stream_ptr->buffers.size(), stream_ptr->buffers.data());
                stream_ptr->settings_dirty.store(true);
                LOG_DEBUG("OpenAL resources created for browserId={}", packet.browserId);
            }
        }

        // Find available buffer (processed or unused slot)
        ALint processed = 0; alGetSourcei(stream_ptr->source, AL_BUFFERS_PROCESSED, &processed);
        if (processed == 0) {
            ALint queued_count = 0; alGetSourcei(stream_ptr->source, AL_BUFFERS_QUEUED, &queued_count);
            if (queued_count >= (ALint)stream_ptr->buffers.size()) continue; // drop packet
        }

        ALuint buffer_to_use = 0;
        if (processed > 0) { alSourceUnqueueBuffers(stream_ptr->source, 1, &buffer_to_use); }
        else { ALint queued_count = 0; alGetSourcei(stream_ptr->source, AL_BUFFERS_QUEUED, &queued_count); buffer_to_use = stream_ptr->buffers[queued_count]; }

        alBufferData(buffer_to_use, AL_FORMAT_MONO16,
            packet.pcm_data.data(), (ALsizei)(packet.pcm_data.size() * sizeof(int16_t)), packet.sampleRate);
        alSourceQueueBuffers(stream_ptr->source, 1, &buffer_to_use);

        ALint state; alGetSourcei(stream_ptr->source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && !stream_ptr->is_culled.load()) { alSourcePlay(stream_ptr->source); }
    }

    // Cleanup on thread exit
    LOG_DEBUG("Cleaning up OpenAL resources in audio thread...");
    {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        for (auto& [id, stream] : streams_) {
            if (stream.source != 0) {
                alSourceStop(stream.source);
                alDeleteSources(1, &stream.source);
                if (!stream.buffers.empty()) {
                    alDeleteBuffers((ALsizei)stream.buffers.size(), stream.buffers.data());
                }
            }
        }
        streams_.clear();
    }

    alcMakeContextCurrent(nullptr);
    if (context_) alcDestroyContext((ALCcontext*)context_);
    if (device_)  alcCloseDevice((ALCdevice*)device_);
    context_ = nullptr; device_ = nullptr;
    LOG_INFO("OpenAL context and device destroyed");
}