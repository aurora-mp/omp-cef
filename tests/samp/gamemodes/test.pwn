#include <a_samp>
#include <cef>
#include <sscanf2>

#define COLOR_ERROR 0xFF0000FF
#define COLOR_USAGE 0xFFFFFFFF
#define COLOR_OPTION 0xB1B1B8FF

#define MAIN_WEBVIEW "http://localhost:5173/"
#define WORLD_BROWSER_URL "https://www.youtube.com/embed/QdBZY2fkU-0?autoplay=1&controls=0"

forward CheckCefPluginStatus(playerid);

forward StringEventName(playerid, browserid, const string[]);
forward DoubleStringEventName(playerid, browserid, const string1[], const string2[]);
forward MultipleArgumentsEventName(playerid, browserid, const string[], integer, bool:boolean);

new gWorldBrowserObject;
new gWorldBrowserObjectBillboard;

main()
{
    print("open.mp CEF Test GameMode loaded.");
    print("Cet été je vais allé dans le hôt du cièel à noël");
}

public OnGameModeInit()
{
	CEF_AddResource("example_inventory_ui");
	CEF_AddResource("example_phone_ui");

	CEF_RegisterEvent("StringEventName", Argument_String);
	CEF_RegisterEvent("DoubleStringEventName", Argument_String, Argument_String);
	CEF_RegisterEvent("MultipleArgumentsEventName", Argument_String, Argument_Integer, Argument_Bool);
    return true;
}

public OnGameModeExit()
{
    return true;
}

public OnPlayerConnect(playerid)
{
	print("GAMEMODE OnPlayerConnect");

    SetTimerEx("CheckCefPluginStatus", 2000, false, "i", playerid);
	RemoveBuildingForPlayer(playerid, 5854, 992.5313, -962.7344, 60.7813, 0.25);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
	print("GAMEMODE OnPlayerRequestClass");

	SendClientMessage(playerid, -1, "Welcome!");

	CEF_ToggleSpawnScreen(playerid, false);

	SetPlayerSkin(playerid, 60);
    SetPlayerPos(playerid, 2296.5662, 2451.6270, 10.8202);
    SetPlayerFacingAngle(playerid, 87.8271);
    SetPlayerCameraPos(playerid, 2293.3640, 2451.7341, 12.8202);
    SetPlayerCameraLookAt(playerid, 2296.5662, 2451.6270, 10.8203);
    return true;
}

public OnPlayerSpawn(playerid)
{
    return true;
}

public OnPlayerDeath(playerid, killerid, reason)
{
    return true;
}

public OnPlayerFinishedDownloading(playerid, virtualworld)
{
	print("GAMEMODE OnPlayerFinishedDownloading");
    return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
	print("GAMEMODE OnDialogResponse");
	return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    new cmd[32], option[32], params[128];

    if (sscanf(cmdtext, "s[32]s[32]S()[128]", cmd, option, params))
    {
        SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef <option>");
        SendClientMessage(playerid, COLOR_OPTION, "OPTIONS: create, testtv, testbill, destroy, reload, emit");
        SendClientMessage(playerid, COLOR_OPTION, "OPTIONS: attachbrowser (ab), detachbrowser (db), mute, mode");
        return 1;
    }

	if (!strcmp(option, "create", true)) {
        new browserid = -1, bool:focused, bool:controls_chat;

        if(sscanf(params, "dbb", browserid, focused, controls_chat)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef create <browserid> <focused (0 - 1)> <controls_chat (0 - 1)>");
            return true;
        }

		CEF_CreateBrowser(playerid, browserid, MAIN_WEBVIEW, focused, controls_chat);
        return true;
    }
	else if (!strcmp(option, "createlocal", true)) {
        new browserid = -1, bool:focused, bool:controls_chat;

        if(sscanf(params, "dbb", browserid, focused, controls_chat)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef create <browserid> <focused (0 - 1)> <controls_chat (0 - 1)>");
            return true;
        }

		CEF_CreateBrowser(playerid, browserid, "cef://example_inventory_ui/index.html", focused, controls_chat);
        return true;
    }
	else if (!strcmp(option, "testtv", true)) {
        new browserid = -1, Float:width = 128, Float:height = 128;

		// 5846
		// CJ_SPRUNK_FRONT2 (256x128)
		// CJ_TV_SCREEN

        if(sscanf(params, "dff", browserid, width, height)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef testtv <browserid> <width> <height>");
            return true;
        }

		new Float:x, Float:y, Float:z;
    	GetPlayerPos(playerid, x, y, z);

		gWorldBrowserObject = CreateObject(14772, x, y, z + 1, 0.0, 0.0, 0.0);
		CEF_CreateWorldBrowser(playerid, browserid, WORLD_BROWSER_URL, "CJ_TV_SCREEN", width, height);
        return true;
    }
	else if (!strcmp(option, "testbill", true)) {
        new browserid = -1, Float:width = 128, Float:height = 128;

		// 5846
		// CJ_SPRUNK_FRONT2 (256x128)
		// CJ_TV_SCREEN

        if(sscanf(params, "dff", browserid, width, height)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef testbill <browserid> <width> <height>");
            return true;
        }

		gWorldBrowserObjectBillboard = CreateObject(5854, 992.5313, -962.7344, 60.7813, 0.25, 0.0, 0.0);
		CEF_CreateWorldBrowser(playerid, browserid, WORLD_BROWSER_URL, "bobobillboard1", width, height);
        return true;
    }
	// gWorldBrowserObjectBillboard
	// RemoveBuildingForPlayer(playerid, 5854, 992.5313, -962.7344, 60.7813, 0.25);
	else if (!strcmp(option, "attachbrowser", true) || !strcmp(option, "ab", true)) {
		new browserid = -1;

        if(sscanf(params, "d", browserid)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef attachbrowser (ab) <browserid>");
            return true;
        }

		CEF_AttachBrowserToObject(playerid, browserid, gWorldBrowserObject);
		// CEF_AttachBrowserToObject(playerid, browserid, gWorldBrowserObjectBillboard);
		return true;
	}
	else if (!strcmp(option, "detachbrowser", true) || !strcmp(option, "db", true)) {
		new browserid = -1;

        if(sscanf(params, "d", browserid)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef detachbrowser (db) <browserid>");
            return true;
        }

		CEF_DetachBrowserFromObject(playerid, browserid, gWorldBrowserObject);
		CEF_DetachBrowserFromObject(playerid, browserid, gWorldBrowserObjectBillboard);
		return true;
	}
	else if (!strcmp(option, "reload", true)) {
		new browserid = -1, bool:ignore_cache = false;

        if(sscanf(params, "db", browserid, ignore_cache)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef reload <browserid> <ignore cache (0 - 1)>");
            return true;
        }

		CEF_ReloadBrowser(playerid, browserid, ignore_cache);
		return true;
	}
	else if (!strcmp(option, "emit", true)) {
		new browserid = -1;

        if(sscanf(params, "d", browserid)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef emit <browserid>");
            return true;
        }

		CEF_EmitEvent(playerid, browserid, "ping", CEF_STR("Hello brother"));
		return true;
	}
	else if (!strcmp(option, "destroy", true)) {
		new browserid = -1;

        if(sscanf(params, "d", browserid)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef destroy <browserid>");
            return true;
        }

		CEF_DestroyBrowser(playerid, browserid);
		return true;
	}
	else if (!strcmp(option, "mute", true)) {
		new browserid = -1, bool:mute;

        if(sscanf(params, "db", browserid, mute)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef mute <browserid> <0 - 1>");
            return true;
        }

		CEF_SetBrowserMuted(playerid, browserid, mute);
		return true;
	}
	else if (!strcmp(option, "mode", true)) {
		new browserid = -1, mode;

        if(sscanf(params, "dd", browserid, mode)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef mute <browserid> <0 (AUDIO_MODE_WORLD) - 1 (AUDIO_MODE_UI)>");
            return true;
        }

		CEF_SetBrowserAudioMode(playerid, browserid, E_CEF_AUDIO_MODE:mode);
		return true;
	}
	else if (!strcmp(option, "car", true)) {
		new Float:x, Float:y, Float:z;
    	GetPlayerPos(playerid, x, y, z);

		new vehicleid = CreateVehicle(560, x, y, z, 0, 1, 1, 1);
		PutPlayerInVehicle(playerid, vehicleid, 0);
		return true;
	}
	else if (!strcmp(option, "tp", true)) {
		SetPlayerPos(playerid, 1028.5376, -963.4943, 42.3691);
		return true;
	}
	else if (!strcmp(option, "edit", true)) {
		EditObject(playerid, gWorldBrowserObject);
		return true;
	}
	else if (!strcmp(option, "tc", true)) {
		new bool:toggle, E_HUD_COMPONENT:componentid;

        if(sscanf(params, "db", _:componentid, toggle)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef tc <component> <toggle>");
            return true;
        }

		CEF_ToggleHudComponent(playerid, componentid, toggle);
		return true;
	}
	else if (!strcmp(option, "focus", true)) {
		new browserid = -1, bool:focus;

        if(sscanf(params, "db", browserid, focus)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef focus <browserid> <toggle>");
            return true;
        }

		CEF_FocusBrowser(playerid, browserid, focus);
		return true;
	}
	else if (!strcmp(option, "weapon", true)) {
		GivePlayerWeapon(playerid, WEAPON_SILENCED, 999);
		return true;
	}
	else if (!strcmp(option, "armor", true)) {
		SetPlayerArmour(playerid, 50.0);
		return true;
	}
	else if (!strcmp(option, "stars", true)) {
		SetPlayerWantedLevel(playerid, 2);
		return true;
	}
	else if (!strcmp(option, "ts", true)) {
		new bool:toggle;

        if(sscanf(params, "b", toggle)) {
            SendClientMessage(playerid, COLOR_USAGE, "USAGE: /cef ts <toggle>");
            return true;
        }

		CEF_ToggleSpawnScreen(playerid, toggle);
		return true;
	}

    return false;
}

public CheckCefPluginStatus(playerid)
{
    if (!IsPlayerConnected(playerid)) {
        return true;
    }

    if (!CEF_PlayerHasPlugin(playerid)) {
        SendClientMessage(playerid, COLOR_ERROR, "ERROR: Your CEF plugin could not be initialized.");
        SendClientMessage(playerid, COLOR_ERROR, "Please check your installation or download the latest version.");
		Kick(playerid);
		return false;
    }

    return true;
}

public OnCefInitialize(playerid, bool:success)
{
    if (success) {
        //SendClientMessage(playerid, 88AA62, "The omp-cef plugin is ready !");
		//CEF_ToggleSpawnScreen(playerid, false);
    }

    return true;
}

public OnCefBrowserCreated(playerid, browserid, bool:success, E_CEF_CREATE_STATUS:code, const reason[])
{
	printf("OnCefBrowserCreated	playerid=%d, browserid=%d, success=%d, code=%d, reason=%s", playerid, browserid, success, _:code, reason);
	return true;
}

public StringEventName(playerid, browserid, const string[])
{
	printf("StringEventName playerid=%d, browserid=%d, string=%s", playerid, browserid, string);
}

public DoubleStringEventName(playerid, browserid, const string1[], const string2[])
{
	printf("DoubleStringEventName playerid=%d, browserid=%d, string1=%s, string2=%s", playerid, browserid, string1, string2);

	new msg[128];
    format(msg, sizeof msg, "DoubleStringEventName playerid=%d, browserid=%d, string1=%s, string2=%s", playerid, browserid, string1, string2);
    SendClientMessage(playerid, -1, msg);
}

public MultipleArgumentsEventName(playerid, browserid, const string[], integer, bool:boolean)
{
	printf("MultipleArgumentsEventName playerid=%d, browserid=%d, string=%s, integer=%d, boolean=%d", playerid, browserid, string, integer, boolean);
}

public OnCefEntityClicked(playerid, entity_type, network_id)
{
	if (entity_type == 2)
    {
        new string[128];
        format(string, sizeof(string), "You clicked on vehicle ID %d.", network_id);
        SendClientMessage(playerid, -1, string);
    }

	return true;
}


/*public OnPlayerCommandText(playerid, cmdtext[])
{
	new cmd[128], subCmd[128], tmp[128], idx = 0;
	cmd = strtok(cmdtext, idx);

	if (strcmp(cmd, "/cef", true) == 0)
	{
		subCmd = strtok(cmdtext, idx);

		if (!strlen(subCmd)) {
			SendClientMessage(playerid, -1, "/cef <option>");
			SendClientMessage(playerid, -1, "create, createworld (cw)");
			return 1;
		}

		if(strcmp(subCmd, "create", true) == 0)
		{
			if (!strlen(tmp)) {
				SendClientMessage(playerid, -1, "/cef createworld (cw) <browserid> <focused>");
				return 1;
			}

			new browseridStr[24], focusedStr[24];

			browseridStr = strtok(cmdtext, idx);
			focusedStr = strtok(cmdtext, idx);

			if (!strlen(browseridStr) || !strlen(focusedStr)) {
				SendClientMessage(playerid, -1, "/cef createworld (cw) <browserid> <focused>");
				return 1;
			}

			new browserId = strval(browseridStr);
			new bool:focused = (strval(focusedStr) != 0);

			CEF_CreateBrowser(playerid, browserId, MAIN_WEBVIEW, focused);
		}

		if(strcmp(subCmd, "createworld", true) == 0 || strcmp(subCmd, "cw", true) == 0)
		{
			if (!strlen(tmp)) {
				SendClientMessage(playerid, -1, "/cef createworld (cw) <browserid> <focused>");
				return 1;
			}

			new browseridStr[24], focusedStr[24];

			browseridStr = strtok(cmdtext, idx);
			focusedStr = strtok(cmdtext, idx);

			if (!strlen(browseridStr) || !strlen(focusedStr)) {
				SendClientMessage(playerid, -1, "/cef createworld (cw) <browserid> <focused>");
				return 1;
			}

			new browserId = strval(browseridStr);
			new bool:focused = (strval(focusedStr) != 0);

			CEF_CreateBrowser(playerid, browserId, MAIN_WEBVIEW, focused);
		}
	}

	if(!strcmp(cmd, "/cmode", true))
	{
		tmp = strtok(cmdtext, idx);
		if(!strlen(tmp))
		{
			SendClientMessage(playerid, -1, "Cmd: /cmode <mode>");
			return 1;
		}

		new mode = strval(tmp);
		TestCursor(playerid, mode);
	}
	if (!strcmp(cmd, "/cef", true)) {
		tmp = strtok(cmdtext, idx);
		if(!strlen(tmp))
		{
			SendClientMessage(playerid, -1, "Cmd: /cef <focused>");
			return 1;
		}

		new focused = strval(tmp);

		if (focused) {
			CEF_CreateBrowser(playerid, 1, "http://localhost:5173/", true);
		}
		else {
			CEF_CreateBrowser(playerid, 1, "http://localhost:5173/", false);
		}
	}
	else if (!strcmp(cmd, "/cefb", true)) {
		new Float:x, Float:y, Float:z;
    	GetPlayerPos(playerid, x, y, z);

		gWorldBrowserObject = CreateObject(14772, x, y, z + 1, 0.0, 0.0, 0.0);
		CEF_CreateWorldBrowser(playerid, 1, "https://www.youtube.com/embed/05MF_6XN374?autoplay=1&controls=0", "CJ_TV_SCREEN", 5.0);
		// SetTimerEx("createbbrowser", 5000, false, "ii", playerid, browserid);
	}
	else if (!strcmp(cmd, "/attachbrowser", true)) {
		CEF_AttachBrowserToObject(playerid, 1, gWorldBrowserObject);
	}
	else if (!strcmp(cmd, "/detachbrowser", true)) {
		CEF_DetachBrowserFromObject(playerid, 1, gWorldBrowserObject);
	}
	else if (!strcmp(cmd, "/reloadbrowser", true)) {
		CEF_ReloadBrowser(playerid, 1);
	}
	// CEF_AttachBrowserToObject
	else if (!strcmp(cmd, "/mute", true)) {
		CEF_SetBrowserMuted(playerid, 1, true);
	}
	else if (!strcmp(cmd, "/unmute", true)) {
		CEF_SetBrowserMuted(playerid, 1, false);
	}
	// CEF_SetBrowserAudioMode
	else if (!strcmp(cmd, "/setfocus", true)) {
		CEF_FocusBrowser(playerid, 1, true);
	}
	else if (!strcmp(cmd, "/setunfocus", true)) {
		CEF_FocusBrowser(playerid, 1, false);
	}
	else if (!strcmp(cmd, "/cefd", true)) {
		CEF_DestroyBrowser(playerid, 1);
		print("OK /cefd");
	}
	else if (!strcmp(cmd, "/ee", true)) {
		CEF_EmitEvent(playerid, 1, "ping");
		print("OK /ee");
	}
	else if (!strcmp(cmd, "/ee1", true)) {
		CEF_EmitEvent(playerid, 1, "ping", CEF_BOOL(true));
		print("OK /ee1");
	}
	else if (!strcmp(cmd, "/ee2", true)) {
		CEF_EmitEvent(playerid, 1, "ping", CEF_STR("pong"));
		print("OK /ee2");
	}
	else if (!strcmp(cmd, "/ee3", true)) {
		CEF_EmitEvent(playerid, 1, "ping", CEF_FLOAT(1.75));
		print("OK /ee3");
	}
	else if (!strcmp(cmd, "/ee4", true)) {
		CEF_EmitEvent(playerid, 1, "ping", CEF_INT(1992));
		print("OK /ee4");
	}
	else if (!strcmp(cmd, "/eefull", true)) {
		CEF_EmitEvent(playerid, 1, "ping", CEF_INT(1992), CEF_BOOL(true), CEF_FLOAT(0.21), CEF_STR("Hello Gemini how are you?"), CEF_BOOL(false));
		print("OK /ee4");
	}
    else
    {
        SendClientMessage(playerid, -1, "Invalid command.");
    }

	SendClientMessage(playerid, -1, "Invalid command.");
    return true;
}*/

/*public OnCefEvent(playerid, browserid, const eventName[], count)
{
	printf("OnCefEvent FROM GM playerid = %d - browserid = %d - eventName = %s - count = %d", playerid, browserid, eventName, count);
    for (new i = 0; i < argCount; i++)
    {
        // On affiche l'index et la valeur de l'argument.
        // Comme tous les arguments sont pass?s en tant que cha?nes, on peut utiliser %s.
        printf("  -> Argument %d: %s", i, args[i]);
    }
}*/

/*public OnCefInitialized(playerid)
{
	print("OnCefInitialized FROM GM");
	SendClientMessage(playerid, -1, "------------------- OnCefInitialized");
    return true;
}*/


