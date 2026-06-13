#include "netgame.hpp"
#include "component_registry.hpp"
#include "samp/addresses.hpp"

#include "r1/netgame.hpp"
#include "r3/netgame.hpp"
#include "r5/netgame.hpp"
#include "dl/netgame.hpp"

REGISTER_SAMP_COMPONENT(NetGameComponent);

void NetGameComponent::Initialize() 
{
	auto version = SampAddresses::Instance().Version();
	switch (version) 
	{
		case SampVersion::V037:
			view_ = std::make_unique<NetGameView_R1>();
			break;
		case SampVersion::V037R3:
			view_ = std::make_unique<NetGameView_R3>();
			break;
		case SampVersion::V037R5:
			view_ = std::make_unique<NetGameView_R5>();
			break;
		case SampVersion::V03DLR1:
			view_ = std::make_unique<NetGameView_DL>();
			break;
		default:
			view_ = nullptr;
			break;
	}
}

std::string NetGameComponent::GetIp() const
{
	if (!view_)
		return "";

	return view_->GetIp();
}

int NetGameComponent::GetPort() const
{
	if (!view_)
		return -1;

	return view_->GetPort();
}

int NetGameComponent::GetState() const
{
    if (!view_)
        return -1;

    return static_cast<int>(view_->GetGameState());
}

bool NetGameComponent::IsConnected() const
{
    if (!view_)
        return false;

    return view_->IsConnected();
}

int NetGameComponent::GetLocalPlayerId()const
{
	if (!view_)
		return -1;

	return view_->GetLocalPlayerId();
}

std::string NetGameComponent::GetLocalPlayerName() const
{
	if (!view_)
		return "";

	return view_->GetLocalPlayerName();
}

IObjectPool* NetGameComponent::GetObjectPool()
{
	if (!view_)
		return nullptr;
	
	return view_->GetObjectPool();
}

IVehiclePool* NetGameComponent::GetVehiclePool()
{
	if (!view_)
		return nullptr;

	return view_->GetVehiclePool();
}

CEntity* NetGameComponent::GetEntityFromObjectId(int objectId)
{
	if (!view_)
		return nullptr;

	return view_->GetEntityFromObjectId(objectId);
}