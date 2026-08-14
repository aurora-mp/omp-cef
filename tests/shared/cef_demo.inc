#if defined _inc_cef_demo
    #endinput
#endif
#define _inc_cef_demo

/*
 * omp-cef demo
 *
 * Author: Neeko
 * Date: 2026
 *
 * This file is intentionally shared by the SA-MP and open.mp test entrypoints.
 * The wrapper .pwn files include either <a_samp> or <open.mp>, then <cef>, then this shared implementation.
 */

/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* DEFINES
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#define CEF_DEMO_RESOURCE_NAME          "cef_demo"

#define CEF_DEMO_OVERLAY_ID             9101
#define CEF_DEMO_WORLD2D_ID             9102
#define CEF_DEMO_WORLD3D_ID             9103
#define CEF_DEMO_CAMERA_ID              9104

#define CEF_DEMO_WORLD3D_OBJECT_MODEL   14772
#define CEF_DEMO_WORLD3D_TEXTURE_NAME   "CJ_TV_SCREEN"
#define CEF_DEMO_INVALID_OBJECT_ID      (-1)

#define CEF_DEMO_URL_INDEX              "http://cef/cef_demo/index.html"
#define CEF_DEMO_URL_OVERLAY            "http://cef/cef_demo/overlay.html"
#define CEF_DEMO_URL_WORLD2D            "http://cef/cef_demo/world2d.html"
#define CEF_DEMO_URL_WORLD3D            "http://cef/cef_demo/world3d.html"
#define CEF_DEMO_URL_EVENTS             "http://cef/cef_demo/events.html"
#define CEF_DEMO_URL_INPUT              "http://cef/cef_demo/input.html"
#define CEF_DEMO_URL_CAMERA             "http://cef/cef_demo/camera.html"
#define CEF_DEMO_URL_RED                "http://cef/cef_demo/red.html"
#define CEF_DEMO_URL_BLUE               "http://cef/cef_demo/blue.html"
#define CEF_DEMO_URL_EMPTY              "http://cef/cef_demo/empty.html"

#define COLOR_INFO                      0xFDBC16FF
#define COLOR_OK                        0x2CF58FFF
#define COLOR_ERROR                     0xFF4545FF
#define COLOR_MUTED                     0xB8B8B8FF

/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* FORWARDS
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

forward bool:CefDemo_IsReady(playerid);

forward CefDemo_RecreateOverlay(playerid);
forward CefDemo_RecreateWorld2D(playerid);
forward CefDemo_RecreateWorld3D(playerid);
forward CefDemo_RecreateAll(playerid);

forward CefDemo_OnButton(playerid, browserid, const action[]);
forward CefDemo_OnInput(playerid, browserid, const value[]);
forward CefDemo_OnPing(playerid, browserid, const message[], value, bool:flag);

/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* VARIABLES
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

new 
    bool:gCefDemoReady[MAX_PLAYERS],
    gCefDemoWorld3DObject[MAX_PLAYERS],
    gCefDemoNotifyCounter[MAX_PLAYERS];


/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* PUBLICS
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

public OnPlayerConnect(playerid)
{
    gCefDemoReady[playerid] = false;
    gCefDemoNotifyCounter[playerid] = 0;
    gCefDemoWorld3DObject[playerid] = CEF_DEMO_INVALID_OBJECT_ID;
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    CefDemo_ResetForPlayer(playerid);
    return 1;
}

public OnCefInitialize(playerid, bool:success, E_CEF_INIT_REASON:reason, const message[])
{
    printf("[CEF Demo] OnCefInitialize playerid=%d success=%d reason=%d message=%s", playerid, success, _:reason, message);

    if (success)
    {
        CEF_ClearChat(playerid);
        SendClientMessage(playerid, COLOR_MUTED, "[CEF Demo] CEF initialized. Waiting for resources...");
    }
    else
    {
        SendClientMessage(playerid, COLOR_ERROR, "[CEF Demo] CEF initialization failed. Check logs.");
    }

    return 1;
}

public OnCefDownloadStart(playerid)
{
    SendClientMessage(playerid, COLOR_MUTED, "[CEF Demo] CEF resource download started.");
    return 1;
}

public OnCefDownloadFinish(playerid)
{
    SendClientMessage(playerid, COLOR_MUTED, "[CEF Demo] CEF resource download finished.");
    return 1;
}

public OnCefReady(playerid)
{
    gCefDemoReady[playerid] = true;
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] CEF ready. Use /cefhelp.");
    return 1;
}

public OnCefBrowserCreated(playerid, browserid, bool:success, E_CEF_CREATE_STATUS:code, const reason[])
{
    printf("[CEF Demo] OnCefBrowserCreated playerid=%d browserid=%d success=%d code=%d reason=%s", playerid, browserid, success, _:code, reason);

    if (success && browserid == CEF_DEMO_WORLD3D_ID)
    {
        CefDemo_AttachWorld3D(playerid);
    }

    return 1;
}

public OnCefPressKey(playerid, key, scancode, modifiers, bool:down, bool:repeat)
{
    printf("[CEF Demo] OnCefPressKey playerid=%d key=%d scancode=%d modifiers=%d down=%d repeat=%d", playerid, key, scancode, modifiers, _:down, _:repeat);
    return 1;
}

public OnCefChatInputState(playerid, bool:open)
{
    printf("[CEF Demo] OnCefChatInputState playerid=%d open=%d", playerid, _:open);
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/cefhelp", true))
    {
        CefDemo_SendHelp(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstatus", true))
    {
        new msg[64];
        new bool:hasPlugin = CEF_PlayerHasPlugin(playerid);
        format(msg, sizeof msg, "[CEF Demo] ready=%d hasPlugin=%d", _:gCefDemoReady[playerid], _:hasPlugin);
        SendClientMessage(playerid, COLOR_INFO, msg);
        return 1;
    }

    if (!CefDemo_IsReady(playerid)) 
        return 1;

    if (!strcmp(cmdtext, "/cefdevtools", true))
    {
        CEF_EnableDevTools(playerid, CEF_DEMO_OVERLAY_ID, true);
        CEF_EnableDevTools(playerid, CEF_DEMO_WORLD2D_ID, true);
        CEF_EnableDevTools(playerid, CEF_DEMO_WORLD3D_ID, true);
        CEF_EnableDevTools(playerid, CEF_DEMO_CAMERA_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] DevTools enabled for demo browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/jsnotify", true))
    {
        gCefDemoNotifyCounter[playerid]++;
        CEF_EmitEvent(playerid, CEF_DEMO_OVERLAY_ID, "demo:notify", CEF_STR("Hello from Pawn"), CEF_INT(gCefDemoNotifyCounter[playerid]));
        CEF_EmitEvent(playerid, CEF_DEMO_WORLD2D_ID, "demo:notify", CEF_STR("Hello from Pawn"), CEF_INT(gCefDemoNotifyCounter[playerid]));
        CEF_EmitEvent(playerid, CEF_DEMO_WORLD3D_ID, "demo:notify", CEF_STR("Hello from Pawn"), CEF_INT(gCefDemoNotifyCounter[playerid]));
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Sent demo:notify to all demo browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/jsstats", true))
    {
        CEF_EmitEvent(playerid, CEF_DEMO_OVERLAY_ID, "demo:stats", CEF_INT(100), CEF_INT(50), CEF_INT(2500));
        CEF_EmitEvent(playerid, CEF_DEMO_WORLD2D_ID, "demo:stats", CEF_INT(100), CEF_INT(50), CEF_INT(2500));
        CEF_EmitEvent(playerid, CEF_DEMO_WORLD3D_ID, "demo:stats", CEF_INT(100), CEF_INT(50), CEF_INT(2500));
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Sent demo:stats to all demo browsers.");
        return 1;
    }

    // Overlay  
    if (!strcmp(cmdtext, "/overlay", true))
    {
        CefDemo_CreateOverlay(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayhide", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_OVERLAY_ID, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D hidden.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayshow", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_OVERLAY_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D shown.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayred", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_RED);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded red page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayblue", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_BLUE);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded blue page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayempty", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_EMPTY);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded empty page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayanim", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_OVERLAY);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded animation page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayevents", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_EVENTS);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded events page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayinput", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_INPUT);
        CEF_FocusBrowser(playerid, CEF_DEMO_OVERLAY_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D loaded input page and focused browser.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayreload", true))
    {
        CEF_ReloadBrowser(playerid, CEF_DEMO_OVERLAY_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D reloaded.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlaydestroy", true))
    {
        CEF_DestroyBrowser(playerid, CEF_DEMO_OVERLAY_ID);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D destroyed.");
        return 1;
    }

    if (!strcmp(cmdtext, "/overlayrecreate", true))
    {
        CEF_DestroyBrowser(playerid, CEF_DEMO_OVERLAY_ID);
        SetTimerEx("CefDemo_RecreateOverlay", 500, false, "i", playerid);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D recreate requested.");
        return 1;
    }

    if (!strcmp(cmdtext, "/camera", true))
    {
        CEF_CreateBrowser(
            playerid,
            CEF_DEMO_CAMERA_ID,
            CEF_DEMO_URL_CAMERA,
            false,
            false,
            0.0,
            0.0
        );
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Game camera overlay created.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cameradestroy", true))
    {
        CEF_DestroyBrowser(playerid, CEF_DEMO_CAMERA_ID);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Game camera overlay destroyed.");
        return 1;
    }

    // World2D
    if (!strcmp(cmdtext, "/world2d", true))
    {
        CefDemo_CreateWorld2D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world2dmove", true))
    {
        new Float:x, Float:y, Float:z;
        CefDemo_GetWorld2DTarget(playerid, x, y, z);
        CEF_SetWorld2DBrowserPos(playerid, CEF_DEMO_WORLD2D_ID, x, y, z);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D moved near your position.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2dhide", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD2D_ID, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D hidden.");
        return 1;    }

    if (!strcmp(cmdtext, "/world2dshow", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD2D_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D shown.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2dred", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, CEF_DEMO_URL_RED);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D loaded red page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2dblue", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, CEF_DEMO_URL_BLUE);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D loaded blue page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2dempty", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, CEF_DEMO_URL_EMPTY);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D loaded empty page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2danim", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, CEF_DEMO_URL_WORLD2D);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D loaded animation page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2ddestroy", true))
    {
        CEF_DestroyBrowser(playerid, CEF_DEMO_WORLD2D_ID);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D destroyed.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world2drecreate", true))
    {
        CEF_DestroyBrowser(playerid, CEF_DEMO_WORLD2D_ID);
        SetTimerEx("CefDemo_RecreateWorld2D", 500, false, "i", playerid);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D recreate requested.");
        return 1;
    }

    // World3D
    if (!strcmp(cmdtext, "/world3d", true))
    {
        CefDemo_CreateWorld3D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dmove", true))
    {
        CefDemo_MoveWorld3D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dattach", true))
    {
        CefDemo_AttachWorld3D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world3ddetach", true))
    {
        CefDemo_DetachWorld3D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dhide", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD3D_ID, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D hidden.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dshow", true))
    {
        CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD3D_ID, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D shown.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dred", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, CEF_DEMO_URL_RED);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D loaded red page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dblue", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, CEF_DEMO_URL_BLUE);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D loaded blue page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3dempty", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, CEF_DEMO_URL_EMPTY);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D loaded empty page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3danim", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, CEF_DEMO_URL_WORLD3D);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D loaded animation page.");
        return 1;
    }

    if (!strcmp(cmdtext, "/world3ddestroy", true))
    {
        CefDemo_DestroyWorld3D(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/world3drecreate", true))
    {
        CefDemo_DestroyWorld3D(playerid);
        SetTimerEx("CefDemo_RecreateWorld3D", 500, false, "i", playerid);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D recreate requested.");
        return 1;
    }

    // Stress Test
    if (!strcmp(cmdtext, "/cefstress", true))
    {
        CefDemo_CreateAll(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressred", true))
    {
        CefDemo_LoadAll(playerid, CEF_DEMO_URL_RED);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: loaded red page on all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressblue", true))
    {
        CefDemo_LoadAll(playerid, CEF_DEMO_URL_BLUE);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: loaded blue page on all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressempty", true))
    {
        CefDemo_LoadAll(playerid, CEF_DEMO_URL_EMPTY);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: loaded empty page on all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressanim", true))
    {
        CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, CEF_DEMO_URL_OVERLAY);
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, CEF_DEMO_URL_WORLD2D);
        CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, CEF_DEMO_URL_WORLD3D);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: loaded animation pages on all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstresshide", true))
    {
        CefDemo_SetAllVisible(playerid, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: hidden all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressshow", true))
    {
        CefDemo_SetAllVisible(playerid, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: shown all browsers.");
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressdestroy", true))
    {
        CefDemo_DestroyAll(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/cefstressrecreate", true))
    {
        CefDemo_DestroyAll(playerid);
        SetTimerEx("CefDemo_RecreateAll", 700, false, "i", playerid);
        return SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress: recreate requested.");
    }

    if (!strcmp(cmdtext, "/hudhide", true))
    {
        CEF_ToggleHudComponent(playerid, HUD_COMPONENT_ALL, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] GTA HUD hidden.");
        return 1;
    }

    if (!strcmp(cmdtext, "/hudshow", true))
    {
        CEF_ToggleHudComponent(playerid, HUD_COMPONENT_ALL, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] GTA HUD shown.");
        return 1;
    }

    if (!strcmp(cmdtext, "/chatinputoff", true))
    {
        CEF_ToggleChatInput(playerid, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Chat input disabled.");
        return 1;
    }

    if (!strcmp(cmdtext, "/chatinputon", true))
    {
        CEF_ToggleChatInput(playerid, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Chat input enabled.");
        return 1;
    }

    if (!strcmp(cmdtext, "/spawnoff", true))
    {
        CEF_ToggleSpawnScreen(playerid, false);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Spawn screen hidden.");
        return 1;
    }

    if (!strcmp(cmdtext, "/spawnon", true))
    {
        CEF_ToggleSpawnScreen(playerid, true);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Spawn screen shown.");
        return 1;
    }

    if (!strcmp(cmdtext, "/escgta", true))
    {
        CEF_SetEscapeMenuMode(playerid, CEF_ESCAPE_MENU_GTA);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] ESC mode: GTA.");
        return 1;
    }

    if (!strcmp(cmdtext, "/escdisabled", true))
    {
        CEF_SetEscapeMenuMode(playerid, CEF_ESCAPE_MENU_DISABLED);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] ESC mode: disabled.");
        return 1;
    }

    if (!strcmp(cmdtext, "/esccustom", true))
    {
        CEF_SetEscapeMenuMode(playerid, CEF_ESCAPE_MENU_CUSTOM);
        SendClientMessage(playerid, COLOR_OK, "[CEF Demo] ESC mode: custom. JS receives cef:escape_menu.");
        return 1;
    }

    return 0;
}

public CefDemo_RecreateOverlay(playerid)
{
    if (!IsPlayerConnected(playerid) || !CefDemo_IsReady(playerid))
        return 0;

    CefDemo_CreateOverlay(playerid);
    return 1;
}

public CefDemo_RecreateWorld2D(playerid)
{
    if (!IsPlayerConnected(playerid) || !CefDemo_IsReady(playerid))
        return 0;

    CefDemo_CreateWorld2D(playerid);
    return 1;
}

public CefDemo_RecreateWorld3D(playerid)
{
    if (!IsPlayerConnected(playerid) || !CefDemo_IsReady(playerid))
        return 0;

    CefDemo_CreateWorld3D(playerid);
    return 1;
}

public CefDemo_RecreateAll(playerid)
{
    if (!IsPlayerConnected(playerid) || !CefDemo_IsReady(playerid))
        return 0;

    CefDemo_CreateAll(playerid);
    return 1;
}

public CefDemo_OnButton(playerid, browserid, const action[])
{
    new msg[144];
    format(msg, sizeof msg, "[CEF Demo] JS button event from browser %d: %s", browserid, action);
    SendClientMessage(playerid, COLOR_OK, msg);
    printf("[CEF Demo] demo:button playerid=%d browserid=%d action=%s", playerid, browserid, action);
    return 1;
}

public CefDemo_OnInput(playerid, browserid, const value[])
{
    new msg[144];
    format(msg, sizeof msg, "[CEF Demo] JS input from browser %d: %s", browserid, value);
    SendClientMessage(playerid, COLOR_OK, msg);
    printf("[CEF Demo] demo:input playerid=%d browserid=%d value=%s", playerid, browserid, value);
    return 1;
}

public CefDemo_OnPing(playerid, browserid, const message[], value, bool:flag)
{
    new msg[160];
    format(msg, sizeof msg, "[CEF Demo] JS ping from browser %d: msg=%s value=%d flag=%d", browserid, message, value, _:flag);
    SendClientMessage(playerid, COLOR_OK, msg);
    printf("[CEF Demo] demo:ping playerid=%d browserid=%d message=%s value=%d flag=%d", playerid, browserid, message, value, _:flag);
    return 1;
}

/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* FUNCTIONS
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

stock CefDemo_Initialize()
{
    CefDemo_Reset();
    CefDemo_RegisterEvents();

    CEF_AddResource(CEF_DEMO_RESOURCE_NAME);

    print("[CEF Demo] Initialized.");
    return 1;
}

/*
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
* =============================================================================
* PRIVATE FUNCTIONS
* =============================================================================
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

static CefDemo_Reset()
{
    for (new playerid = 0; playerid < MAX_PLAYERS; playerid++)
    {
        gCefDemoReady[playerid] = false;
        gCefDemoNotifyCounter[playerid] = 0;
        gCefDemoWorld3DObject[playerid] = CEF_DEMO_INVALID_OBJECT_ID;
    }

    return 1;
}

static CefDemo_ResetForPlayer(playerid)
{
    gCefDemoReady[playerid] = false;
    gCefDemoNotifyCounter[playerid] = 0;

    if (gCefDemoWorld3DObject[playerid] != CEF_DEMO_INVALID_OBJECT_ID)
    {
        DestroyObject(gCefDemoWorld3DObject[playerid]);
        gCefDemoWorld3DObject[playerid] = CEF_DEMO_INVALID_OBJECT_ID;
    }

    return 1;
}

static CefDemo_RegisterEvents()
{
    CEF_RegisterEvent("demo:button", "CefDemo_OnButton", Argument_String);
    CEF_RegisterEvent("demo:input", "CefDemo_OnInput", Argument_String);
    CEF_RegisterEvent("demo:ping", "CefDemo_OnPing", Argument_String, Argument_Integer, Argument_Bool);
    return 1;
}

static bool:CefDemo_IsReady(playerid)
{
    if (!gCefDemoReady[playerid])
    {
        SendClientMessage(playerid, COLOR_ERROR, "[CEF Demo] CEF is not ready yet. Wait for OnCefReady.");
        return false;
    }

    return true;
}

static CefDemo_SendHelp(playerid)
{
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Main: /cefhelp /cefstatus /cefdevtools /jsnotify /jsstats");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Overlay2D: /overlay /overlayhide /overlayshow /overlayred /overlayblue /overlayempty /overlayanim");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Overlay2D: /overlayevents /overlayinput /overlayreload /overlaydestroy /overlayrecreate");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Screen capture: /camera /cameradestroy");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] World2D: /world2d /world2dmove /world2dhide /world2dshow /world2dred /world2dblue /world2dempty /world2danim");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] World3D: /world3d /world3dmove /world3dattach /world3ddetach /world3dhide /world3dshow /world3dred /world3dblue /world3dempty /world3danim");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Stress: /cefstress /cefstressred /cefstressblue /cefstressempty /cefstressanim /cefstresshide /cefstressshow /cefstressdestroy");
    SendClientMessage(playerid, COLOR_INFO, "[CEF Demo] Utils: /hudhide /hudshow /chatinputoff /chatinputon /spawnoff /spawnon /escgta /esccustom /escdisabled");
    return 1;
}

static CefDemo_CreateOverlay(playerid)
{
    CEF_CreateBrowser(
        playerid,
        CEF_DEMO_OVERLAY_ID,
        CEF_DEMO_URL_OVERLAY,
        false,
        true,
        0.0,
        0.0
    );

    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Overlay2D created.");
    return 1;
}

static CefDemo_CreateWorld2D(playerid)
{
    new Float:x, Float:y, Float:z;
    CefDemo_GetWorld2DTarget(playerid, x, y, z);

    CEF_CreateWorld2DBrowser(
        playerid,
        CEF_DEMO_WORLD2D_ID,
        CEF_DEMO_URL_WORLD2D,
        x,
        y,
        z,
        480.0,
        260.0,
        0.0,
        0.5,
        1.0
    );

    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] World2D created near your position. Use /world2dmove if needed.");
    printf("[CEF Demo] World2D target playerid=%d x=%f y=%f z=%f", playerid, x, y, z);
    return 1;
}

static CefDemo_GetWorld2DTarget(playerid, &Float:x, &Float:y, &Float:z)
{
    GetPlayerPos(playerid, x, y, z);
    x += 3.0;
    z += 1.25;
    return 1;
}

static CefDemo_CreateWorld3D(playerid)
{
    CefDemo_DestroyWorld3DObject(playerid);

    new Float:x, Float:y, Float:z;
    CefDemo_GetWorld3DTarget(playerid, x, y, z);

    gCefDemoWorld3DObject[playerid] = CreateObject(
        CEF_DEMO_WORLD3D_OBJECT_MODEL,
        x,
        y,
        z,
        0.0,
        0.0,
        0.0
    );

    CEF_CreateWorldBrowser(
        playerid,
        CEF_DEMO_WORLD3D_ID,
        CEF_DEMO_URL_WORLD3D,
        CEF_DEMO_WORLD3D_TEXTURE_NAME,
        512.0,
        256.0
    );

    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D browser/object created near your position.");
    SendClientMessage(playerid, COLOR_MUTED, "[CEF Demo] It auto-attaches after OnCefBrowserCreated. Use /world3dattach if needed.");
    printf("[CEF Demo] WorldObject3D target playerid=%d objectid=%d x=%f y=%f z=%f", playerid, gCefDemoWorld3DObject[playerid], x, y, z);
    return 1;
}

static CefDemo_GetWorld3DTarget(playerid, &Float:x, &Float:y, &Float:z)
{
    GetPlayerPos(playerid, x, y, z);
    x += 3.0;
    z += 1.0;
    return 1;
}

static CefDemo_DestroyWorld3DObject(playerid)
{
    if (gCefDemoWorld3DObject[playerid] != CEF_DEMO_INVALID_OBJECT_ID)
    {
        DestroyObject(gCefDemoWorld3DObject[playerid]);
        gCefDemoWorld3DObject[playerid] = CEF_DEMO_INVALID_OBJECT_ID;
    }

    return 1;
}

static CefDemo_AttachWorld3D(playerid)
{
    if (gCefDemoWorld3DObject[playerid] == CEF_DEMO_INVALID_OBJECT_ID)
    {
        SendClientMessage(playerid, COLOR_ERROR, "[CEF Demo] WorldObject3D object does not exist. Use /world3d first.");
        return 1;
    }

    CEF_AttachBrowserToObject(playerid, CEF_DEMO_WORLD3D_ID, gCefDemoWorld3DObject[playerid]);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D browser attached to object.");
    printf("[CEF Demo] WorldObject3D attached playerid=%d browserid=%d objectid=%d", playerid, CEF_DEMO_WORLD3D_ID, gCefDemoWorld3DObject[playerid]);
    return 1;
}

static CefDemo_DetachWorld3D(playerid)
{
    if (gCefDemoWorld3DObject[playerid] == CEF_DEMO_INVALID_OBJECT_ID)
        return 1;

    CEF_DetachBrowserFromObject(playerid, CEF_DEMO_WORLD3D_ID, gCefDemoWorld3DObject[playerid]);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D browser detached from object.");
    return 1;
}

static CefDemo_MoveWorld3D(playerid)
{
    if (gCefDemoWorld3DObject[playerid] == CEF_DEMO_INVALID_OBJECT_ID)
    {
        SendClientMessage(playerid, COLOR_ERROR, "[CEF Demo] WorldObject3D object does not exist. Use /world3d first.");
        return 1;
    }

    new Float:x, Float:y, Float:z;
    CefDemo_GetWorld3DTarget(playerid, x, y, z);

    SetObjectPos(gCefDemoWorld3DObject[playerid], x, y, z);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D moved near your position.");
    return 1;
}

static CefDemo_DestroyWorld3D(playerid)
{
    CefDemo_DetachWorld3D(playerid);
    CEF_DestroyBrowser(playerid, CEF_DEMO_WORLD3D_ID);
    CefDemo_DestroyWorld3DObject(playerid);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] WorldObject3D browser/object destroyed.");
    return 1;
}

static CefDemo_CreateAll(playerid)
{
    CefDemo_CreateOverlay(playerid);
    CefDemo_CreateWorld2D(playerid);
    CefDemo_CreateWorld3D(playerid);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress setup created: Overlay2D + World2D + WorldObject3D.");
    return 1;
}

static CefDemo_DestroyAll(playerid)
{
    CEF_DestroyBrowser(playerid, CEF_DEMO_OVERLAY_ID);
    CEF_DestroyBrowser(playerid, CEF_DEMO_WORLD2D_ID);
    CEF_DestroyBrowser(playerid, CEF_DEMO_CAMERA_ID);
    CefDemo_DestroyWorld3D(playerid);
    SendClientMessage(playerid, COLOR_OK, "[CEF Demo] Stress setup destroyed.");
    return 1;
}

static CefDemo_LoadAll(playerid, const url[])
{
    CEF_LoadUrl(playerid, CEF_DEMO_OVERLAY_ID, url);
    CEF_LoadUrl(playerid, CEF_DEMO_WORLD2D_ID, url);
    CEF_LoadUrl(playerid, CEF_DEMO_WORLD3D_ID, url);
    return 1;
}

static CefDemo_SetAllVisible(playerid, bool:visible)
{
    CEF_SetBrowserVisible(playerid, CEF_DEMO_OVERLAY_ID, visible);
    CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD2D_ID, visible);
    CEF_SetBrowserVisible(playerid, CEF_DEMO_WORLD3D_ID, visible);
    return 1;
}
