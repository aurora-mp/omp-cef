#pragma once

#include "packet.hpp"

static inline void WriteString(std::ostream& os, const std::string& str)
{
	uint16_t length = static_cast<uint16_t>(str.length());

	if (length > 4096) {
		LOG_ERROR("String too long: {} bytes, truncating to 4096", length);
		length = 4096;
	}

	os.write(reinterpret_cast<const char*>(&length), sizeof(length));

	if (length > 0) {
		os.write(str.data(), std::min(static_cast<size_t>(length), str.length()));
	}
}

static inline bool ReadString(std::istream& is, std::string& str)
{
	uint16_t length = 0;

	is.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (is.gcount() != sizeof(length))
		return false;

	if (length > 0) {
		str.resize(length);
		is.read(&str[0], length);

		if (is.gcount() != length)
			return false;
	}
	else {
		str.clear();
	}

	return true;
}

static inline void WriteBytes(std::ostream& os, const std::vector<uint8_t>& bytes)
{
	uint32_t length = static_cast<uint32_t>(bytes.size());

	os.write(reinterpret_cast<const char*>(&length), sizeof(length));

	if (length > 0) {
		os.write(reinterpret_cast<const char*>(bytes.data()), length);
	}
}

static inline bool ReadBytes(std::istream& is, std::vector<uint8_t>& bytes)
{
	uint32_t length = 0;

	is.read(reinterpret_cast<char*>(&length), sizeof(length));
	if (is.gcount() != sizeof(length))
		return false;

	if (length > 0) {
		bytes.resize(length);

		is.read(reinterpret_cast<char*>(bytes.data()), length);
		if (is.gcount() != length)
			return false;
	}
	else {
		bytes.clear();
	}

	return true;
}

inline bool SerializePacket(const NetworkPacket& packet, std::string& out)
{
	try {
		std::ostringstream os(std::ios::binary);
		os.put(static_cast<uint8_t>(packet.type));

		std::visit([&os](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, RequestJoinPacket>) {
				os.write(reinterpret_cast<const char*>(&arg.playerid), sizeof(arg.playerid));
				os.write(reinterpret_cast<const char*>(&arg.client_version), sizeof(arg.client_version));
			}
			else if constexpr (std::is_same_v<T, HandshakeChallengePacket>) {
				WriteBytes(os, arg.cookie);
				WriteBytes(os, arg.server_public_key);
			}
			else if constexpr (std::is_same_v<T, HandshakeFinalizePacket>) {
				WriteBytes(os, arg.cookie);
				WriteBytes(os, arg.client_public_key);
			}
			else if constexpr (std::is_same_v<T, JoinResponsePacket>) {
				os.put(arg.accepted ? 1 : 0);
				os.write(reinterpret_cast<const char*>(&arg.kcp_conv_id), sizeof(arg.kcp_conv_id));
				WriteString(os, arg.manifest_json);
				os.put(static_cast<char>(arg.reject_reason));
				os.write(reinterpret_cast<const char*>(&arg.server_version), sizeof(arg.server_version));
				os.write(reinterpret_cast<const char*>(&arg.client_version), sizeof(arg.client_version));
				WriteString(os, arg.message);
			}
			else if constexpr (std::is_same_v<T, ServerConfigPacket>) {
				WriteBytes(os, arg.master_resource_key);
				os.put(arg.resources_loader_ui ? 1 : 0);
			}
			else if constexpr (std::is_same_v<T, RequestFilesPacket>) {
				uint16_t count = static_cast<uint16_t>(arg.files.size());
				os.write(reinterpret_cast<const char*>(&count), sizeof(count));

				if (!os.good())
					return;

				for (size_t i = 0; i < arg.files.size(); ++i) {
					const auto& [resourceName, relativePath] = arg.files[i];

					if (resourceName.empty() || relativePath.empty())
						continue;

					WriteString(os, resourceName);

					if (!os.good())
						return;

					WriteString(os, relativePath);

					if (!os.good())
						return;
				}
			}
			else if constexpr (std::is_same_v<T, FileDataPacket>) {
				WriteString(os, arg.resourceName);
				WriteString(os, arg.relativePath);
				WriteString(os, arg.fileHash);
				os.write(reinterpret_cast<const char*>(&arg.chunkIndex), sizeof(arg.chunkIndex));
				os.write(reinterpret_cast<const char*>(&arg.totalChunks), sizeof(arg.totalChunks));
				WriteBytes(os, arg.data);
			}
			else if constexpr (std::is_same_v<T, EmitEventPacket> || std::is_same_v<T, ClientEmitEventPacket>) {
				os.write(reinterpret_cast<const char*>(&arg.browserId), sizeof(arg.browserId));
				WriteString(os, arg.name);

				uint8_t count = static_cast<uint8_t>(arg.args.size());
				os.put(count);

				for (const auto& argument : arg.args) {
					os.put(static_cast<uint8_t>(argument.type));

					switch (argument.type) {
						case ArgumentType::String:
							WriteString(os, argument.stringValue);
							break;
						case ArgumentType::Integer:
							os.write(reinterpret_cast<const char*>(&argument.intValue), sizeof(int));
							break;
						case ArgumentType::Float:
							os.write(reinterpret_cast<const char*>(&argument.floatValue), sizeof(float));
							break;
						case ArgumentType::Bool:
							os.put(argument.boolValue ? 1 : 0);
							break;
					}
				}
			}
		}, packet.payload);

		if (!os.good()) {
			return false;
		}

		out = os.str();
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERROR("[Serialize] Exception: {}", e.what());
		return false;
	}
	catch (...) {
		LOG_ERROR("[Serialize] Unknown exception");
		return false;
	}
}

inline bool DeserializePacket(const char* data, size_t size, NetworkPacket& out)
{
	std::istringstream is(std::string(data, size), std::ios::binary);
	uint8_t packet_type_val = 0;

	is.get(reinterpret_cast<char&>(packet_type_val));
	if (!is.good())
		return false;

	out.type = static_cast<PacketType>(packet_type_val);

	switch (out.type)
	{
		case PacketType::RequestJoin: {
			RequestJoinPacket packet{};

			is.read(reinterpret_cast<char*>(&packet.playerid), sizeof(packet.playerid));
			if (!is.good())
				return false;

			packet.client_version = 0;
			if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.client_version))) {
				is.read(reinterpret_cast<char*>(&packet.client_version), sizeof(packet.client_version));
				if (!is.good()) {
					packet.client_version = 0;
					is.clear();
				}
			}

			out.payload = packet;
			break;
		}
		case PacketType::HandshakeChallenge: {
			HandshakeChallengePacket packet{};

			if (!ReadBytes(is, packet.cookie))
				return false;

			if (!ReadBytes(is, packet.server_public_key))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::HandshakeFinalize: {
			HandshakeFinalizePacket packet{};

			if (!ReadBytes(is, packet.cookie))
				return false;

			if (!ReadBytes(is, packet.client_public_key))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::JoinResponse: {
			JoinResponsePacket packet{};

			char accepted;
			is.get(accepted);

			if (!is.good())
				return false;

			packet.accepted = accepted != 0;
			is.read(reinterpret_cast<char*>(&packet.kcp_conv_id), sizeof(packet.kcp_conv_id));

			if (!is.good())
				return false;

			if (!ReadString(is, packet.manifest_json))
				return false;

			packet.reject_reason = static_cast<uint8_t>(JoinRejectReason::None);
			packet.server_version = 0;
			packet.client_version = 0;
			packet.message.clear();

			if (is.rdbuf()->in_avail() > 0) {
				char buf = 0;
				is.get(buf);

				if (is.good()) {
					packet.reject_reason = static_cast<uint8_t>(buf);
					
					if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.server_version)))
						is.read(reinterpret_cast<char*>(&packet.server_version), sizeof(packet.server_version));
					
					if (is.rdbuf()->in_avail() >= static_cast<std::streamsize>(sizeof(packet.client_version)))
						is.read(reinterpret_cast<char*>(&packet.client_version), sizeof(packet.client_version));

					if (is.rdbuf()->in_avail() >= 2) {
						ReadString(is, packet.message);
					}
				}

				if (!is.good()) {
					is.clear();
				}
			}

			out.payload = packet;
			break;
		}
		case PacketType::ServerConfig: {
			ServerConfigPacket packet{};

			if (!ReadBytes(is, packet.master_resource_key))
				return false;

			char b = 0;
			is.get(b);
			if (!is.good())
				return false;

			packet.resources_loader_ui = (b != 0);

			out.payload = packet;
			break;
		}
		case PacketType::RequestFiles: {
			RequestFilesPacket packet{};

			uint16_t count{};
			is.read(reinterpret_cast<char*>(&count), sizeof(count));
			if (is.gcount() != sizeof(count))
				return false;

			for (uint16_t i = 0; i < count; ++i) {
				std::string resourceName;
				std::string relativePath;

				if (!ReadString(is, resourceName) || !ReadString(is, relativePath)) {
					return false;
				}

				packet.files.emplace_back(resourceName, relativePath);
			}
			out.payload = packet;
			break;
		}
		case PacketType::FileData: {
			FileDataPacket packet{};

			if (!ReadString(is, packet.resourceName))
				return false;

			if (!ReadString(is, packet.relativePath))
				return false;

			if (!ReadString(is, packet.fileHash))
				return false;

			is.read(reinterpret_cast<char*>(&packet.chunkIndex), sizeof(packet.chunkIndex));
			if (is.gcount() != sizeof(packet.chunkIndex))
				return false;

			is.read(reinterpret_cast<char*>(&packet.totalChunks), sizeof(packet.totalChunks));
			if (is.gcount() != sizeof(packet.totalChunks))
				return false;

			if (!ReadBytes(is, packet.data))
				return false;

			out.payload = packet;
			break;
		}
		case PacketType::EmitEvent:
		case PacketType::EmitBrowserEvent:
		case PacketType::ClientEmitEvent: {
            EmitEventPacket packet{};

            is.read(reinterpret_cast<char*>(&packet.browserId), sizeof(packet.browserId));
            if (!ReadString(is, packet.name))
                return false;

            uint8_t count{};
            is.get(reinterpret_cast<char&>(count));

            if (!is.good()) 
                return false;

            for (uint8_t i = 0; i < count; ++i) {
                uint8_t type{};
                is.get(reinterpret_cast<char&>(type));

                Argument arg;
                arg.type = static_cast<ArgumentType>(type);

                switch (arg.type) 
                {
                    case ArgumentType::String:  
                        if (!ReadString(is, arg.stringValue)) 
                            return false;
                        break;
                    case ArgumentType::Integer: 
                        is.read(reinterpret_cast<char*>(&arg.intValue), sizeof(int)); 
                        break;
                    case ArgumentType::Float:   
                        is.read(reinterpret_cast<char*>(&arg.floatValue), sizeof(float)); 
                        break;
                    case ArgumentType::Bool:    
                        char boolean; is.get(boolean); arg.boolValue = (boolean != 0);
                        break;
                }

                if (!is.good()) 
                    return false;

                packet.args.push_back(arg);
            }

            if (out.type == PacketType::ClientEmitEvent) {
                ClientEmitEventPacket client_packet;

                client_packet.browserId = packet.browserId;
                client_packet.name = packet.name;
                client_packet.args = packet.args;

                out.payload = client_packet;
            }
            else {
                out.payload = packet;
            }

            break;
        }
	}

	return is.good();
}