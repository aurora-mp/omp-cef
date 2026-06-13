#include <open.mp>
#include <cef>

#define MAIN_WEBVIEW_ID 1

#define WEBVIEW_DEV_URL "http://localhost:5173/"
#define WEBVIEW_PROD_URL "http://cef/webview/index.html"

forward OnAccountLogin(playerid, browserid, const password[]);

main()
{
    print("omp-cef example");
}

public OnGameModeInit()
{
	// Register resources
	CEF_AddResource("example");
	CEF_AddResource("webview");

	// Register events
	CEF_RegisterEvent("account:login", "OnAccountLogin", Argument_String);

    return true;
}

public OnGameModeExit()
{
    return true;
}

public OnPlayerRequestClass(playerid, classid)
{
	SendClientMessage(playerid, -1, "Welcome to omp-cef example gamemode!");

	// Hide "Spawn" button instead of making *fucking* camera hack...
	CEF_ToggleSpawnScreen(playerid, false);
		
	SetPlayerSkin(playerid, 60);
    SetPlayerPos(playerid, 2296.5662, 2451.6270, 10.8202);
    SetPlayerFacingAngle(playerid, 87.8271);
    SetPlayerCameraPos(playerid, 2293.3640, 2451.7341, 12.8202);
    SetPlayerCameraLookAt(playerid, 2296.5662, 2451.6270, 10.8203);
    return true;
}

public OnCefInitialize(playerid, bool:success)
{
	if (!success) {
		SendClientMessage(playerid, -1, "It appears you do not have the CEF plugin installed.");
		return false;
	}

    return true;	
}

public OnCefReady(playerid)
{
	printf("CEF Ready for playerid=%d", playerid);

	CEF_CreateBrowser(playerid, MAIN_WEBVIEW_ID, WEBVIEW_PROD_URL, true, true);
	
    return true;	
}

public OnCefBrowserCreated(playerid, browserid, bool:success, E_CEF_CREATE_STATUS:code, const reason[]) 
{
	printf("Browser id=%d created for playerid=%d success=%d code=%d reason=%s", browserid, playerid, success, _:code, reason);
	
	if (success) {
		new playerName[MAX_PLAYER_NAME];
		GetPlayerName(playerid, playerName);
	
		// You can send JSON if you want (plugin needed)
		CEF_EmitEvent(playerid, MAIN_WEBVIEW_ID, "account:info", Argument_String, playerName);
	}
	
	return true;
}

public OnAccountLogin(playerid, browserid, const password[]) 
{
	printf("OnAccountLogin for playerid=%d browserid=%d, password=%s", playerid, browserid, password);

	// Check password ... etc etc
	// Do what you want, I'm not here to code your GM :D
	return true;
}