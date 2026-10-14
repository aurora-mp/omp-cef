#pragma once

#include "Server/Components/Pawn/pawn.hpp"
#include "common/plugin.hpp"

struct ICefOmpComponent : IComponent
{
    PROVIDE_UID(0xD9607C6728B33464);
};

class CefOmpComponent final : public ICefOmpComponent,
                              public PawnEventHandler,
                              public CoreEventHandler,
                              public PlayerConnectEventHandler
{
public:
    StringView componentName() const override;
    SemanticVersion componentVersion() const override;

    ~CefOmpComponent();

    void onLoad(ICore* core) override;
    void onInit(IComponentList* components) override;
    void onReady() override;
    void onFree(IComponent* component) override;
    void provideConfiguration(ILogger& logger, IEarlyConfig& config, bool defaults) override;
    void free() override;
    void reset() override;

    IExtension* getExtension(UID id) override;

    void onAmxLoad(IPawnScript& script) override;
    void onAmxUnload(IPawnScript& script) override;
	void onTick(Microseconds elapsed, TimePoint now) override;

    void onPlayerConnect(IPlayer& player) override;
    void onPlayerClientInit(IPlayer& player) override;
    void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

private:
    static constexpr uint16_t cef_network_port_offset = 2;

private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;

    std::unique_ptr<CefPlugin> plugin_;

    bool debug_enabled_ = false;
    std::vector<uint8_t> master_resource_key_;
    ResourceLoaderUiMode resource_loader_ui_mode_ = ResourceLoaderUiMode::Cef;
    int resource_download_dialog_id_ = ResourceDownloadDialogManager::DefaultDialogId;

    uint16_t server_port_ = 7777;
    uint16_t cef_network_port_ = 7779;
};
