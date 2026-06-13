#include "game.hpp"
#include "component_registry.hpp"
#include "samp/addresses.hpp"

#include "r1/game.hpp"
#include "r3/game.hpp"
#include "r5/game.hpp"
#include "dl/game.hpp"

REGISTER_SAMP_COMPONENT(GameComponent);

void GameComponent::Initialize() 
{
    auto version = SampAddresses::Instance().Version();
    switch (version) 
    {
        case SampVersion::V037:
            view_ = std::make_unique<GameView_R1>();
            break;
        case SampVersion::V037R3:
            view_ = std::make_unique<GameView_R3>();
            break;
        case SampVersion::V037R5:
            view_ = std::make_unique<GameView_R5>();
            break;
        case SampVersion::V03DLR1:
            view_ = std::make_unique<GameView_DL>();
            break;
        default:
            view_ = nullptr;
            break;
    }
}

void GameComponent::SetCursorMode(int mode, bool hideCursorImmediately) 
{
    if (!view_)
        return;

    view_->SetCursorMode(mode, hideCursorImmediately);
}

void GameComponent::ProcessInputEnabling() 
{
    if (!view_)
        return;

    view_->ProcessInputEnabling();
}