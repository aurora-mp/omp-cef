#pragma once

#include <string>
#include <vector>
#include <variant>
#include <cstdint>

enum class ArgumentType : uint8_t
{
	String = 0,
	Integer = 1,
	Float = 2,
	Bool = 3,
};

struct Argument
{
	ArgumentType type;
	std::string stringValue;
	int intValue = 0;
	float floatValue = 0.0f;
	bool boolValue = false;

	Argument() = default;
	Argument(const std::string& value) : type(ArgumentType::String), stringValue(value) {}
	Argument(int value) : type(ArgumentType::Integer), intValue(value) {}
	Argument(float value) : type(ArgumentType::Float), floatValue(value) {}
	Argument(bool value) : type(ArgumentType::Bool), boolValue(value) {}
};

enum class PacketType : uint8_t
{
	RequestJoin,
	HandshakeChallenge,
	HandshakeFinalize,
	JoinResponse,
	ServerConfig,

	RequestFiles,
	FileData,

	DownloadComplete,
	PressKey,

	EmitEvent,
	EmitBrowserEvent,
	ClientEmitEvent,
};

enum class JoinRejectReason : uint8_t
{
	None = 0,
	VersionMismatch = 1,
};

struct RequestJoinPacket
{
	int playerid = -1;
	uint32_t client_version = 0;
};

struct HandshakeChallengePacket
{
	std::vector<uint8_t> cookie;
	std::vector<uint8_t> server_public_key;
};

struct HandshakeFinalizePacket
{
	std::vector<uint8_t> cookie;
	std::vector<uint8_t> client_public_key;
};

struct JoinResponsePacket
{
	bool accepted = false;
	uint32_t kcp_conv_id;
	std::string manifest_json;

	uint8_t reject_reason = static_cast<uint8_t>(JoinRejectReason::None);
	uint32_t server_version = 0;
	uint32_t client_version = 0;
	std::string message;
};

struct ServerConfigPacket
{
	std::vector<uint8_t> master_resource_key;

	// When false, the client will download resources silently (no internal loader UI)
	bool resources_loader_ui = true;
};

struct RequestFilesPacket
{
	std::vector<std::pair<std::string, std::string>> files;
};

struct FileDataPacket
{
	std::string resourceName;
	std::string relativePath;
	std::string fileHash;
	uint32_t chunkIndex;
	uint32_t totalChunks;
	std::vector<uint8_t> data;
};

struct EmitEventPacket 
{
    int browserId;
    std::string name;
    std::vector<Argument> args;
};

struct ClientEmitEventPacket {
    int browserId;
    std::string name;
    std::vector<Argument> args;
};

using PacketPayload = std::variant<
	RequestJoinPacket,
	HandshakeChallengePacket,
	HandshakeFinalizePacket,
	JoinResponsePacket,
	ServerConfigPacket,

	RequestFilesPacket,
	FileDataPacket,

	EmitEventPacket,
	ClientEmitEventPacket
>;

struct NetworkPacket
{
	PacketType type;
	PacketPayload payload;
};