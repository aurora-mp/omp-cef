#include "samp.hpp"

#include "common.hpp"
#include "hooks/hook_manager.hpp"
#include "samp/addresses.hpp"
#include "samp/components/game.hpp"
#include "system/logger.hpp"

static Samp* g_samp_self = nullptr;

bool Samp::Initialize()
{
    g_samp_self = this;

    void* addrInit = SampAddresses::Instance().SampInitialization();
    void* addrDeinit = SampAddresses::Instance().SampDeinitialization();

    if (!addrInit || !addrDeinit)
    {
        LOG_FATAL("One or more SA-MP addresses are null.");
        return false;
    }

    if (!hooks_.Install("SampInitialization", addrInit, reinterpret_cast<void*>(&SampInitializationHook)) ||
        !hooks_.Install("SampDeinitialization", addrDeinit, reinterpret_cast<void*>(&SampDeinitializationHook)))
    {
        LOG_FATAL("Failed to hook Samp.");
        return false;
    }

    InitializeComponents();

    return true;
}

void* Samp::GetOriginal(const char* name) const
{
    return hooks_.GetOriginal(name);
}

void Samp::DisableHook(const char* name)
{
    hooks_.Disable(name);
}

/*void Samp::SetControlsLocked(bool locked)
{
    if (auto* game = GetComponent<GameComponent>())
    {
        if (locked)
        {
            if (g_pD3DDeviceHook)
                g_pD3DDeviceHook->SetForceHideCursor(TRUE);
            game->SetCursorMode(CMODE_LOCKCAMANDCONTROL, TRUE);
        }
        else
        {
            if (g_pD3DDeviceHook)
                g_pD3DDeviceHook->SetForceHideCursor(FALSE);
            game->SetCursorMode(CMODE_NONE, TRUE);
        }
    }
}*/

void __declspec(naked) Samp::SampInitializationHook() noexcept
{
    static void* return_address = nullptr;

    __asm {
        pushad
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE
    }

    LOG_INFO("Loading SA-MP ...");

    return_address = g_samp_self ? g_samp_self->GetOriginal("SampInitialization") : nullptr;
    if (g_samp_self)
        g_samp_self->DisableHook("SampInitialization");

    if (g_samp_self && g_samp_self->OnLoaded)
        g_samp_self->OnLoaded();

    LOG_INFO("SA-MP loaded.");

    __asm {
        mov esp, ebp
        popad
        jmp return_address
    }
}

void __declspec(naked) Samp::SampDeinitializationHook() noexcept
{
    static void* return_address = nullptr;

    __asm {
        pushad
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE
    }

    LOG_INFO("Exit SA-MP ...");

    return_address = g_samp_self ? g_samp_self->GetOriginal("SampDeinitialization") : nullptr;
    if (g_samp_self)
        g_samp_self->DisableHook("SampDeinitialization");

    if (g_samp_self && g_samp_self->OnExit)
        g_samp_self->OnExit();

    __asm {
        mov esp, ebp
        popad
        jmp return_address
    }
}