#if defined _INC_cef
	#endinput
#endif
#define _INC_cef

/**
 * Result codes for the CEF client initialization / handshake.
 *
 * These values are passed to OnCefInitialize() to indicate why the CEF client
 * was accepted or rejected for a player.
 *
 * - If success == true: reason is CEF_INIT_OK and message is usually empty.
 * - If success == false: reason describes the failure cause and message may
 *   contain additional details (it can be empty depending on the failure).
 */
enum E_CEF_INIT_REASON
{
    // Initialization succeeded, secure channel is ready.
    CEF_INIT_OK = 0,

    // Handshake did not complete within the timeout (client.dll missing/blocked, no connection).
    CEF_INIT_TIMEOUT,

    // Client/server versions differ (server was updated but client.dll wasn't).
    CEF_INIT_VERSION_MISMATCH,

    // UDP endpoint/IP mismatch detected (spoofed packet or networking mismatch).
    CEF_INIT_IP_MISMATCH,

    // Handshake failed (invalid cookie / key exchange failed / invalid state).
    CEF_INIT_HANDSHAKE_FAILED,

    // Unknown/unclassified failure.
    CEF_INIT_UNKNOWN
};

/**
 * Defines the possible status codes for a browser creation attempt.
 */
enum E_CEF_CREATE_STATUS
{
    SUCCESS,
    ERROR_GENERIC,
    ERROR_ID_ALREADY_IN_USE
};

/**
 * Defines the data types for arguments passed from Pawn to C++.
 */
enum E_CEF_ARGUMENT_TYPE 
{
    Argument_String,
    Argument_Integer,
    Argument_Float,
    Argument_Bool
};

/**
 * Helper macros to simplify passing typed arguments to natives.
 *
 * CEF_EmitEvent("eventName", CEF_STR("some_string"), CEF_INT(123), CEF_BOOL(true));
 */
#define CEF_STR(%0)     Argument_String, %0
#define CEF_INT(%0)     Argument_Integer, %0
#define CEF_FLOAT(%0)   Argument_Float, %0
#define CEF_BOOL(%0)    Argument_Bool, %0

/**
 * Defines the audio playback mode for a browser stream.
 */
enum E_CEF_AUDIO_MODE {
    /**
     * 3D World Audio: The sound is spatialized, positioned at the object's location,
     * and attenuates with distance. Ideal for in-game objects like TVs/boombox ...
     */
    AUDIO_MODE_WORLD,

    /**
     * 2D UI Audio: The sound is played directly to the player, non-spatialized,
     * like a user interface sound. Ideal for car radios (from inside), phone calls,
     * global server announcements ...
     */
    AUDIO_MODE_UI
};

/**
 * Defines the different components of the game's HUD that can be controlled.
 */
enum E_HUD_COMPONENT
{
    HUD_COMPONENT_ALL,
    HUD_COMPONENT_AMMO,
    HUD_COMPONENT_ARMOUR,
    HUD_COMPONENT_BREATH,
    HUD_COMPONENT_CROSSHAIR,
    HUD_COMPONENT_HEALTH,
    HUD_COMPONENT_MONEY,
    HUD_COMPONENT_RADAR,
    HUD_COMPONENT_WANTED_STARS,
    HUD_COMPONENT_WEAPON
};

 /**
 * Checks if a player has successfully completed the CEF "handshake".
 *
 * @param playerid      The ID of the player to check.
 *
 * @return              true if the player has the plugin and is ready, false otherwise.
*/
native bool:CEF_PlayerHasPlugin(playerid);

/**
 * Adds a resource (UI) to be downloaded by clients.
 * This must be called from OnGameModeInit or a similar early-stage callback.
 *
 * @param resourceName      The path to the directory (scriptfiles/cef/my_ui)
 */
native CEF_AddResource(const resourceName[]);

/**
 * Creates a 2D browser overlay for a specific player.
 *
 * @param playerid          The ID of the player who will see this browser.
 * @param browserid         A unique identifier for this browser.
 * @param url               The URL to load.
 * @param focused           If 'true', the browser will immediately gain input focus.
 * @param controls_chat     (Optional) If 'true' (default), the game's chat input will be
 * disabled when the player is typing in an HTML input field.
 * @param width             (Optional) The width of the browser viewport in pixels. Defaults to -1 for fullscreen width.
 * @param height            (Optional) The height of the browser viewport in pixels. Defaults to -1 for fullscreen height.
*/
native CEF_CreateBrowser(playerid, browserid, const url[], bool:focused, bool:controls_chat = true, Float:width = 0.0, Float:height = 0.0);

/**
 * Creates a 3D browser rendered in the world for a specific player.
 *
 * @param playerid          The ID of the player who will see this browser.
 * @param browserid         A unique identifier for this browser.
 * @param url               The URL to load.
 * @param textureName       The name of the texture that this browser will replace.
 * @param width             The width of the browser view in pixels.
 * @param height            The height of the browser view in pixels.
 */
native CEF_CreateWorldBrowser(playerid, browserid, const url[], const textureName[], Float:width, Float:height);

/**
 * Creates a CEF browser rendered in 2D (overlay) whose screen position is automatically
 * updated every frame from a 3D world position using GTA's World-to-Screen projection
 * (CalcScreenCoors).
 *
 * This is ideal for "world tooltips" such as: "Press E to interact", item labels, etc.
 *
 * @param playerid      The ID of the player who will see this browser.
 * @param browserid     A unique browser ID for this player (choose a constant like 9000 for tooltips).
 * @param url           The URL to load.
 * @param worldX        Initial target world X coordinate.
 * @param worldY        Initial target world Y coordinate.
 * @param worldZ        Initial target world Z coordinate.
 * @param width         Browser width in pixels. If 0.0, a default/fallback size may be used.
 * @param height        Browser height in pixels. If 0.0, a default/fallback size may be used.
 * @param offsetZ       Extra vertical offset added by the client when projecting (e.g. to place above the head).
 *                      Prefer using 0.0 here and handle per-interactable offset by adding it to worldZ in SetWorld2DBrowserPos().
 * @param pivotX        Horizontal anchor (0.0..1.0). 0.5 = centered. 0.0 = left.
 * @param pivotY        Vertical anchor (0.0..1.0). 1.0 = above the point (bottom-aligned). 0.0 = top.
 */
native CEF_CreateWorld2DBrowser(playerid, browserid, const url[], Float:worldX, Float:worldY, Float:worldZ, Float:width = 0.0, Float:height = 0.0, Float:offsetZ = 0.0, Float:pivotX = 0.0, Float:pivotY = 0.0);

/**
 * Updates the 3D world position used by a World2D browser created with CEF_CreateWorld2DBrowser().
 * The plugin will project this position to 2D each frame and move the browser accordingly.
 *
 * @param playerid      The ID of the player who will see this browser.
 * @param browserid     The World2D browser ID previously created.
 * @param worldX        Target world X coordinate.
 * @param worldY        Target world Y coordinate.
 * @param worldZ        Target world Z coordinate.
 */
native CEF_SetWorld2DBrowserPos(playerid, browserid, Float:worldX, Float:worldY, Float:worldZ);

/**
 * Shows or hides an existing browser for a player without destroying it.
 *
 * Notes:
 * - Hidden browsers are not drawn.
 * - If a hidden browser was focused, it should lose focus to prevent stuck input (implementation-dependent).
 *
 * @param playerid      The ID of the player who will see this browser.
 * @param browserid     The browser ID to show/hide.
 * @param visible       true to show, false to hide.
 */
native CEF_SetBrowserVisible(playerid, browserid, bool:visible);

/**
 * Destroys a browser instance for a specific player.
 *
 * @param playerid          The ID of the player.
 * @param browserid         The ID of the browser to destroy.
 */
native CEF_DestroyBrowser(playerid, browserid);

/**
 * Registers an event that can be emitted from CEF (JavaScript) to Pawn.
 * This function defines the expected argument types for the event.
 *
 * @param eventName         The name of the event to register.
 * @param callback          The name of the event callback.
 * @param ...               A variadic list of argument types (e.g., Argument_String, Argument_Integer ...).
 */
native CEF_RegisterEvent(const eventName[], const callback[], E_CEF_ARGUMENT_TYPE:...);

/**
 * Emits an event from Pawn to a specific CEF browser instance (JavaScript).
 *
 * @param playerid          The ID of the player.
 * @param browserid         The ID of the browser to receive the event.
 * @param eventName         The name of the event to emit.
 * @param ...               A variadic list of arguments, using the CEF_* helper macros.
 */
native CEF_EmitEvent(playerid, browserid, const eventName[], {E_CEF_ARGUMENT_TYPE, Float, _}:...);

/**
 * Reloads the current page of a browser for a specific player.
 *
 * This is useful for restarting a video or for refreshing a web page's content.
 *
 * @param playerid      The ID of the player for whom the browser will be reloaded.
 * @param browserid     The ID of the browser to reload.
 * @param ignore_cache  If true, the page is reloaded bypassing the browser's cache. Defaults to false.
*/
native CEF_ReloadBrowser(playerid, browserid, bool:ignore_cache = false);

/**
 * Makes a browser focused for a specific player.
 *
 * @param playerid          The ID of the player.
 * @param id                The ID of the browser to focus or unfocus.
 * @param focused           Set to 'true' to give focus, 'false' to remove it.
 */
native CEF_FocusBrowser(playerid, id, bool:focused);

/**
 * Enables or disables Chromium DevTools for a specific browser of a player.
 *
 * @param playerid  The ID of the player.
 * @param browserid The ID of the target browser.
 * @param enabled   true to show DevTools, false to hide.
 */
native CEF_EnableDevTools(playerid, browserid, bool:enabled);

/**
 * Attaches a world browser to a specific object for a specific player.
 *
 * @param playerid          The ID of the player.
 * @param browserid         The ID of the world browser to attach.
 * @param objectid          The ID of the object to attach to.
 */
native CEF_AttachBrowserToObject(playerid, browserid, objectid);

/**
 * Detaches a world browser from an object for a specific player.
 *
 * @param playerid          The ID of the player.
 * @param browserid         The ID of the world browser to detach.
 * @param objectid          The ID of the object to detach from.
 */
native CEF_DetachBrowserFromObject(playerid, browserid, objectid);

/**
 * NATIVES (Audio)
 */

/**
 * Mutes or unmutes a specific browser's audio stream for a specific player.
 *
 * This function allows you to control the audio state of a browser for an specific player.
 * It does not affect what other players hear, making it ideal for personal settings or
 * player-specific interactions.
 *
 * @param playerid      The ID of the player for whom the audio will be muted or unmuted.
 * @param browserid     The ID of the browser to affect.
 * @param mute          Set to 'true' to mute the audio, 'false' to unmute.
*/
native CEF_SetBrowserMuted(playerid, browserid, bool:mute);

/**
 * Sets the audio playback mode for a specific browser stream for a specific player.
 *
 * This function allows you to dynamically switch a browser's audio between 3D spatialized
 * sound (AUDIO_MODE_WORLD) and 2D non-spatialized sound (AUDIO_MODE_UI).
 *
 * @param playerid      The ID of the player for whom the audio mode will be changed.
 * @param browserid     The ID of the browser to affect.
 * @param mode          The audio mode to set, using the E_CEF_AUDIO_MODE enum.
 *
 * @return              This native does not return any specific value.
*/
native CEF_SetBrowserAudioMode(playerid, browserid, E_CEF_AUDIO_MODE:mode);

/**
 * Adjusts the 3D audio settings for a specific browser stream.
 *
 * This function allows to define how a sound's volume attenuates over distance,
 * giving precise control over the audio experience for each world browser.
 *
 * @param playerid     			The ID of the player for whom the audio settings will be changed.
 * @param browserid             The ID of the browser to affect.
 * @param max_distance          The maximum distance (in meters) at which the sound can be heard.
 *                              Beyond this distance, the sound will be completely silent.
 * @param reference_distance    The distance (in meters) at which the sound is heard at its full volume (100%).
 *                              The volume will start to decrease beyond this point.
*/
native CEF_SetBrowserAudioSettings(playerid, browserid, Float:max_distance, Float:reference_distance);

/**
 * NATIVES (Utils)
 */

/**
 * Toggles the visibility of a specific component of the game's default HUD.
 *
 * @param playerid          The ID of the player.
 * @param componentid       The HUD component to toggle, from the E_HUD_COMPONENT enum.
 * @param toggle            Set to 'true' to show the component, 'false' to hide it.
 */
native CEF_ToggleHudComponent(playerid, E_HUD_COMPONENT:componentid, bool:toggle);

/**
 * Toggles the visibility of the default SA-MP class selection screen.
 *
 * This utility function allows you to hide the entire class selection interface,
 * including the skin preview, arrows, and the "Spawn" button. This is crucial
 * for creating a fully custom login or character selection UI with CEF for example.
 *
 * @param playerid      The ID of the player for whom the screen will be toggled.
 * @param toggle        Set to 'true' to show the screen, 'false' to hide it.
*/
native CEF_ToggleSpawnScreen(playerid, bool:toggle);

/**
 * Clears the in-game chat for a specific player.
 *
 * Why this native exists:
 * Historically, many gamemodes "cleared" the chat by spamming a large number of empty
 * messages using SendClientMessage (or similar loops). That approach is a hack:
 * - It floods the chat buffer and can cause visible flicker / scroll jumps.
 * - It produces unnecessary network traffic (many messages sent just to push old lines away).
 *
 * This native provides a clean and reliable way to clear the chat client-side without
 * sending dozens of useless messages.
 *
 * @param playerid      The ID of the player whose chat will be cleared.
 */
native CEF_ClearChat(playerid);

/**
 * Enables or disables the SA:MP chat input for a specific player.
 *
 * When disabled, the player will not be able to open the chat input
 * (e.g. with the "T" or "F6" key).
 *
 * This function only affects the chat input box — it does not hide
 * the chat messages already displayed on screen.
 *
 * @param playerid      The ID of the player whose chat input state will be changed.
 * @param toggle        Set to 'true' to enable the chat input, 'false' to disable it.
 */
native CEF_ToggleChatInput(playerid, bool:toggle);

/**
 * Returns whether the SA:MP chat input is currently open for a player.
 *
 * Note:
 * - This is the state of the *input box* (open/closed), not the chat messages panel.
 * - The value is updated client-side and synchronized to the server by the plugin.
 *
 * @param playerid  The player ID.
 *
 * @return 1 if the chat input is open, 0 if it is closed.
 */
native CEF_IsChatInputOpen(playerid);

/**
 * Enables or disables forwarding of a specific key to OnCefPressKey for a player.
 * Keys are Windows Virtual-Key codes (VK).
 *
 * @param playerid  The player ID.
 * @param key       The Virtual-Key cod.
 * @param enabled   true to forward this key, false to stop forwarding it.
 */
native CEF_EnableKey(playerid, key, bool:enabled);

/**
 * Enables or disables CEF keyboard event capture for a given player.
 *
 * When key capture is enabled, the client/plugin forwards keyboard events to the server
 * via the callback `OnCefPressKey(playerid, key, scancode, modifiers, down, repeat)`.
 *
 * Important:
 * - If disabled, no `OnCefPressKey` events will be emitted for the player.
 * - You should usually keep this disabled by default to avoid unnecessary traffic.
 *
 * @param playerid  The ID of the player.
 * @param enabled   true to capture/forward keyboard events to `OnCefPressKey`, false to stop.
 */
native CEF_SetKeyCapture(playerid, bool:enabled);

/**
 * Exit the game for a given player.
 *
 * @param playerid  The ID of the player.
 */
native CEF_ExitGame(playerid);

/**
 * CALLBACKS
 */

/**
 * Called when a player's CEF client has successfully initialized
 * and established a secure connection with the server plugin.
 *
 * This is the earliest and most reliable point to start sending CEF commands
 * to a player.
 *
 * @param playerid      The ID of the player whose client is ready.
 * @param success       Always true. (Reserved for future use).
 * @param reason        Detailed initialization result (see E_CEF_INIT_REASON).
 * @param message       Additional details (can be empty).
*/
forward OnCefInitialize(playerid, bool:success, E_CEF_INIT_REASON:reason, const message[]);

/*
 * Called when player start downloading resources.
 *
 * @param playerid      The ID of the player.
 */
forward OnCefDownloadStart(playerid);

/*
 * Called when player finish downloading resources.
 *
 * @param playerid      The ID of the player.
 */
forward OnCefDownloadFinish(playerid);

/**
 * Called when a player's CEF client is fully ready (ressources downloaded ... etc)
 *
 * Useful when you want to start creating browser or emit events (like for showing a login screen ...)
 *
 * @param playerid      The ID of the player.
 */
forward OnCefReady(playerid);

/**
 * Called when a browser creation request is processed by the client.
 *
 * This callback is the unified response for any browser creation attempt.
 * It informs the scripter about the success or failure of the operation.
 *
 * @param playerid      The ID of the player for whom the browser was created.
 * @param browserid     The ID of the browser that was requested.
 * @param success       'true' if the browser was created successfully, 'false' otherwise.
 * @param code          A status code from the E_CEF_CREATE_STATUS enum.
 * @param reason        A string explaining the result (e.g., "Successfully created" or "ID already in use").
*/
forward OnCefBrowserCreated(playerid, browserid, bool:success, E_CEF_CREATE_STATUS:code, const reason[]);

/**
 * Called when the CEF input layer detects a keyboard event for a player.
 *
 * This callback is triggered by the CEF client/plugin when a key event is captured.
 *
 * @param playerid   The ID of the player who generated the key event.
 * @param key        Virtual-key code of the key (Windows VK code).
 * @param scancode   Hardware scan code for the key.
 * @param modifiers  Bitmask of modifier keys currently held (Shift/Ctrl/Alt/etc.).
 * @param down       True if the key is pressed; false if released.
 * @param repeat     True if this key event is an auto-repeat; false otherwise.
*/
forward OnCefPressKey(playerid, key, scancode, modifiers, bool:down, bool:repeat);

/**
 * Called when the client detects a change in the SA:MP chat input state (open/closed).
 *
 * @param playerid  The player ID.
 * @param open      true if chat input is open, false if closed.
 */
forward OnCefChatInputState(playerid, bool:open);

/**
 * Called when a player clicks on a world entity (player, vehicle, object) that
 * has a browser attached to it.
 *
 * @param playerid      The ID of the player who clicked.
 * @param entity_type   The type of the entity clicked (TODO: define enum).
 * @param network_id    The network ID of the entity.
 */
// TODO: forward OnCefEntityClicked(playerid, entity_type, network_id);