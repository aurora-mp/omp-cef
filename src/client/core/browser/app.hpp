#pragma once

#include "include/cef_app.h"

class DefaultCefApp : public CefApp {
public:
    DefaultCefApp() = default;

    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override 
    {
        command_line->AppendSwitchWithValue("force-device-scale-factor", "1.0");
        command_line->AppendSwitchWithValue("high-dpi-support", "1");
        command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
        command_line->AppendSwitchWithValue("allow-browser-signin", "false");

        // TODO: command_line->AppendSwitch("enable-begin-frame-scheduling");
    }

private:
    IMPLEMENT_REFCOUNTING(DefaultCefApp);
};