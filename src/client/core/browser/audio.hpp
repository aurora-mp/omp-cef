#pragma once

#include <cstdint>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <AL/al.h>

enum AudioMode 
{
    AUDIO_MODE_WORLD = 0, // 3D spatialized audio positioned in world space
    AUDIO_MODE_UI = 1 // 2D non-spatialized audio relative to listener
};

struct AudioPacket 
{
    int browserId{};
    int sampleRate{};
    std::vector<int16_t> pcm_data; // Mono 16-bit signed PCM data
};

// Represents a single audio stream tied to a browser instance
// Contains OpenAL and thread-safe properties
struct AudioStream {
    ALuint source = 0;
    std::vector<ALuint> buffers; // Circular buffer pool for streaming

    // Thread-safe position properties
    std::atomic<float> pos_x{ 0.0f };
    std::atomic<float> pos_y{ 0.0f };
    std::atomic<float> pos_z{ 0.0f };

    // Dirty flags for audio thread synchronization
    std::atomic<bool> position_dirty{ false };
    std::atomic<bool> settings_dirty{ true };

    std::atomic<bool> is_culled{ false };
    std::atomic<bool> is_muted{ false };

    // 3D audio attenuation parameters
    std::atomic<float> max_distance{ 25.0f };
    std::atomic<float> ref_distance{ 1.0f };

    std::atomic<int> audio_mode{ AUDIO_MODE_WORLD };

    // Safe destruction: main thread sets flag, audio thread performs cleanup
    std::atomic<bool> pending_destroy{ false };
};

// Manages all OpenAL audio playback in a dedicated thread
class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager() { Shutdown(); }

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool Initialize();
    void Shutdown();

    // CEF audio callback, converts multi-channel float to mono int16
    void OnPcmPacket(int browserId, const float** data, int frames, int channels, int sampleRate);

    // Methods to control audio streams from the main thread
    void EnsureStream(int browserId);
    void RemoveStream(int browserId);
    void UpdateSourcePosition(int browserId, float x, float y, float z);
    void UpdateListenerPosition(float x, float y, float z, float at_x, float at_y, float at_z, float up_x, float up_y, float up_z);
    void SetStreamMuted(int browserId, bool muted);
    void SetStreamAudioSettings(int browserId, float max_distance, float ref_distance);
    void SetStreamAudioMode(int browserId, AudioMode mode);

private:
    void AudioThreadLoop();

    void* device_ = nullptr; // ALCdevice* (void* to avoid header dependency)
    void* context_ = nullptr; // ALCcontext*

    std::thread audio_thread_;
    std::atomic<bool> terminate_{ false };

    // Producer-consumer queue for audio packets from CEF
    std::queue<AudioPacket> packet_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;

    // Active audio streams
    std::unordered_map<int, AudioStream> streams_;
    std::mutex streams_mutex_;

    // 3D listener state
    std::atomic<float> listener_pos_x_{ 0.0f }, listener_pos_y_{ 0.0f }, listener_pos_z_{ 0.0f };
    std::atomic<float> listener_at_x_{ 0.0f }, listener_at_y_{ 0.0f }, listener_at_z_{ 1.0f };
    std::atomic<float> listener_up_x_{ 0.0f }, listener_up_y_{ 1.0f }, listener_up_z_{ 0.0f };
    std::atomic<bool> listener_dirty_{ false };
};