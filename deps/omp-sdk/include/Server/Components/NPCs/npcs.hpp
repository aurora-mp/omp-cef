#pragma once

#include <sdk.hpp>

enum NPCMoveType
{
	NPCMoveType_None,
	NPCMoveType_Walk,
	NPCMoveType_Jog,
	NPCMoveType_Sprint,
	NPCMoveType_Drive,
	NPCMoveType_Auto
};

enum class EntityCheckType : uint8_t
{
	None = 0,
	Player = 1,
	NPC = 2,
	Actor = 4,
	Vehicle = 8,
	Object = 16,
	ProjectOrig = 32,
	ProjectTarg = 64,
	Map = 128,
	All = 255
};

struct INPC : public IExtensible, public IIDProvider
{
	/// Get player instance of NPC.
	virtual IPlayer* getPlayer() = 0;

	/// Get the entity's position
	virtual Vector3 getPosition() const = 0;

	/// Set NPC position with ability to decide whether it needs immediate update or not
	virtual void setPosition(const Vector3& position, bool immediateUpdate) = 0;

	/// Get the entity's rotation
	virtual GTAQuat getRotation() const = 0;

	/// Set NPC rotation with ability to decide whether it needs immediate update or not
	virtual void setRotation(const GTAQuat& rotation, bool immediateUpdate) = 0;

	/// Get the entity's virtual world
	virtual int getVirtualWorld() const = 0;

	/// Set the entity's virtual world
	virtual void setVirtualWorld(int vw) = 0;

	/// Spawn NPC.
	virtual void spawn() = 0;

	/// Respawn the NPC with the same stats and state it is already in
	virtual void respawn() = 0;

	/// Move NPC to a specified location.
	virtual bool move(Vector3 position, NPCMoveType moveType, float moveSpeed = NPC_MOVE_SPEED_AUTO, float stopRange = 1.0f) = 0;

	/// Move NPC to follow a player, continuously updating position as player moves.
	virtual bool moveToPlayer(IPlayer& targetPlayer, NPCMoveType moveType, float moveSpeed = NPC_MOVE_SPEED_AUTO, float stopRange = 0.2f, Milliseconds posCheckUpdateDelay = Milliseconds(500), bool autoRestart = false) = 0;

	/// Stop NPC from moving.
	virtual void stopMove() = 0;

	/// Check if NPC is moving
	virtual bool isMoving() const = 0;

	/// Set NPC skin
	virtual void setSkin(int model) = 0;

	/// Check if a player is streamed in for the current NPC
	virtual bool isStreamedInForPlayer(const IPlayer& other) const = 0;

	/// Get the NPC which are streamed in for this player
	virtual const FlatPtrHashSet<IPlayer>& streamedForPlayers() const = 0;

	/// Set NPC interior, it doesn't do much but keeps a record internally to use getter functions later
	virtual void setInterior(unsigned int interior) = 0;

	/// Get NPC interior, just the value that is stored internally
	virtual unsigned int getInterior() const = 0;

	/// Get NPC velocity
	virtual Vector3 getVelocity() const = 0;

	/// Set NPC velocity
	virtual void setVelocity(Vector3 position, bool update = false) = 0;

	/// Set health
	virtual void setHealth(float health) = 0;

	/// Get health
	virtual float getHealth() const = 0;

	/// Set armour
	virtual void setArmour(float armour) = 0;

	/// Get armour
	virtual float getArmour() const = 0;

	/// Check if NPC is dead
	virtual bool isDead() const = 0;

	/// Set weapon
	virtual void setWeapon(uint8_t weapon) = 0;

	/// Get weapon
	virtual uint8_t getWeapon() const = 0;

	/// Set ammo
	virtual void setAmmo(int ammo) = 0;

	/// Get ammo
	virtual int getAmmo() const = 0;

	/// Set weapon skill level
	virtual void setWeaponSkillLevel(PlayerWeaponSkill weaponSkill, int level) = 0;

	/// Get weapon skill level
	virtual int getWeaponSkillLevel(PlayerWeaponSkill weaponSkill) const = 0;

	/// Set keys
	virtual void setKeys(uint16_t upAndDown, uint16_t leftAndRight, uint16_t keys) = 0;

	/// Get keys
	virtual void getKeys(uint16_t& upAndDown, uint16_t& leftAndRight, uint16_t& keys) const = 0;

	/// Melee attack
	virtual void meleeAttack(int time, bool secondaryMeleeAttack = false) = 0;

	/// Stop melee attack
	virtual void stopMeleeAttack() = 0;

	/// Check if npc is doing melee attack
	virtual bool isMeleeAttacking() const = 0;

	/// Set fighting style
	virtual void setFightingStyle(PlayerFightingStyle style) = 0;

	/// Get fighting style
	virtual PlayerFightingStyle getFightingStyle() const = 0;

	/// Set state to reloading
	virtual void enableReloading(bool toggle) = 0;

	/// Check if NPC is reloading
	virtual bool isReloadEnabled() const = 0;

	/// Check if reloading
	virtual bool isReloading() const = 0;

	/// Enable or disable infinite ammo
	virtual void enableInfiniteAmmo(bool enable) = 0;

	/// Check if NPC is reloading
	virtual bool isInfiniteAmmoEnabled() const = 0;

	/// Get weapon state
	virtual PlayerWeaponState getWeaponState() const = 0;

	/// Set ammo in current clip
	virtual void setAmmoInClip(int ammo) = 0;

	/// Get ammo in current clip
	virtual int getAmmoInClip() const = 0;

	/// Trigger a weapon shot
	virtual void shoot(int hitId, PlayerBulletHitType hitType, uint8_t weapon, const Vector3& endPoint, const Vector3& offset, bool isHit, EntityCheckType betweenCheckFlags) = 0;

	/// Check if shooting
	virtual bool isShooting() const = 0;

	/// Aim at a position
	virtual void aimAt(const Vector3& point, bool shoot, int shootDelay, bool setAngle, const Vector3& offsetFrom, EntityCheckType betweenCheckFlags) = 0;

	/// Aim at a specific player
	virtual void aimAtPlayer(IPlayer& atPlayer, bool shoot, int shootDelay, bool setAngle, const Vector3& offset, const Vector3& offsetFrom, EntityCheckType betweenCheckFlags) = 0;

	/// Stop aiming
	virtual void stopAim() = 0;

	/// Check if aiming
	virtual bool isAiming() const = 0;

	/// Check if it's aiming at a specific player
	virtual bool isAimingAtPlayer(IPlayer& player) const = 0;

	/// Set weapon accuracy for the NPC. Default is 1.0f
	virtual void setWeaponAccuracy(uint8_t weapon, float accuracy) = 0;

	/// Get weapon accuracy
	virtual float getWeaponAccuracy(uint8_t weapon) const = 0;

	/// Set weapon reload time
	virtual void setWeaponReloadTime(uint8_t weapon, int time) = 0;

	/// Get weapon reload time
	virtual int getWeaponReloadTime(uint8_t weapon) = 0;

	/// Get weapon actual reload time based on skill and weapon being double handed or not
	virtual int getWeaponActualReloadTime(uint8_t weapon) = 0;

	/// Set weapon shoot time
	virtual void setWeaponShootTime(uint8_t weapon, int time) = 0;

	/// Get weapon shoot time
	virtual int getWeaponShootTime(uint8_t weapon) = 0;

	/// Set weapon clip size
	virtual void setWeaponClipSize(uint8_t weapon, int size) = 0;

	/// Get weapon clip size
	virtual int getWeaponClipSize(uint8_t weapon) = 0;

	/// Get weapon actual reload time based on skill and weapon being double handed or not; infinite ammo or not.
	virtual int getWeaponActualClipSize(uint8_t weapon) = 0;

	/// Attempt to enter a vehicle
	virtual void enterVehicle(IVehicle& vehicle, uint8_t seatId, NPCMoveType moveType) = 0;

	/// Exit a vehicle
	virtual void exitVehicle() = 0;

	/// Put NPC in vehicle
	virtual bool putInVehicle(IVehicle& vehicle, uint8_t seat) = 0;

	/// Remove NPC from vehicle
	virtual bool removeFromVehicle() = 0;

	/// Move NPC through a path created previously
	virtual bool moveByPath(int pathId, NPCMoveType moveType = NPCMoveType_Auto, float moveSpeed = NPC_MOVE_SPEED_AUTO, bool reverse = false) = 0;

	/// Pause NPC path movement temporarily to resume later
	virtual void pausePath() = 0;

	/// Resume NPC path movement that was paused before
	virtual void resumePath() = 0;

	/// Stop path movement (or just use stopMove)
	virtual void stopPath() = 0;

	/// Check if NPC is moving according to a path
	virtual bool isMovingByPath() const = 0;

	/// Check if path movement is paused
	virtual bool isPathPaused() const = 0;

	/// Get current path id NPC is moving by
	virtual int getCurrentPathId() const = 0;

	/// Get current point index in current path movement
	virtual int getCurrentPathPointIndex() const = 0;

	/// Get NPC vehicle
	virtual IVehicle* getVehicle() = 0;

	/// Get NPC vehicle seat
	virtual int getVehicleSeat() const = 0;

	/// Get NPC's entering vehicle
	virtual IVehicle* getEnteringVehicle() = 0;

	/// Get NPC's entering vehicle's seat
	virtual int getEnteringVehicleSeat() const = 0;

	/// Set NPC vehicle siren
	virtual void useVehicleSiren(bool use = true) = 0;

	/// Check if vehicle siren is used
	virtual bool isVehicleSirenUsed() const = 0;

	/// Set NPC vehicle health;
	virtual void setVehicleHealth(float health) = 0;

	/// Get NPC vehicle health
	virtual float getVehicleHealth() const = 0;

	/// Set NPC vehicle hydra thruster direction
	virtual void setVehicleHydraThrusters(int direction) = 0;

	/// Get NPC vehicle hydra thruster direction
	virtual int getVehicleHydraThrusters() const = 0;

	/// Set NPC vehicle gear state
	virtual void setVehicleGearState(int gear) = 0;

	/// Get NPC vehicle gear state
	virtual int getVehicleGearState() const = 0;

	/// Set NPC train speed
	virtual void setVehicleTrainSpeed(float speed) = 0;

	/// Get NPC train speed
	virtual float getVehicleTrainSpeed() const = 0;

	/// Reset NPC animation in foot sync
	virtual void resetAnimation() = 0;

	/// Set NPC animation in foot sync
	virtual void setAnimation(int animationId, float delta, bool loop, bool lockX, bool lockY, bool freeze, int time) = 0;

	/// Get NPC animation from foot sync
	virtual void getAnimation(int& animationId, float& delta, bool& loop, bool& lockX, bool& lockY, bool& freeze, int& time) = 0;

	/// Apply NPC animation
	virtual void applyAnimation(const AnimationData& animationData) = 0;

	/// Clear NPC animations
	virtual void clearAnimations() = 0;

	/// Set NPC special action
	virtual void setSpecialAction(PlayerSpecialAction action) = 0;

	/// Get NPC special action
	virtual PlayerSpecialAction getSpecialAction() const = 0;

	/// Start playing a recorded playback
	virtual bool startPlayback(StringView recordName, bool autoUnload = true, const Vector3& point = Vector3(0.0f, 0.0f, 0.0f), const GTAQuat& rotation = GTAQuat()) = 0;

	/// Start playing a preloaded playback by record ID
	virtual bool startPlayback(int recordId, bool autoUnload = true, const Vector3& point = Vector3(0.0f, 0.0f, 0.0f), const GTAQuat& rotation = GTAQuat()) = 0;

	/// Stop current playback
	virtual void stopPlayback() = 0;

	/// Pause/resume current playback
	virtual void pausePlayback(bool paused = true) = 0;

	/// Check if playback is currently playing
	virtual bool isPlayingPlayback() const = 0;

	/// Check if playback is paused
	virtual bool isPlaybackPaused() const = 0;

	/// Start node-based movement for the NPC
	virtual bool playNode(int nodeId, NPCMoveType moveType, float moveSpeed = NPC_MOVE_SPEED_AUTO, float radius = 0.0f, bool setAngle = true) = 0;

	/// Stop node-based movement
	virtual void stopPlayingNode() = 0;

	/// Pause node-based movement
	virtual void pausePlayingNode() = 0;

	/// Resume node-based movement
	virtual void resumePlayingNode() = 0;

	/// Check if node movement is paused
	virtual bool isPlayingNodePaused() const = 0;

	/// Check if NPC is currently following a node path
	virtual bool isPlayingNode() const = 0;

	/// Change to a different node with a specific target point
	virtual uint16_t changeNode(int nodeId, uint16_t targetPointId) = 0;

	/// Update the current node point
	virtual bool updateNodePoint(uint16_t pointId) = 0;

	/// Set NPC invulnerability
	virtual void setInvulnerable(bool toggle) = 0;

	/// Check for NPC invulnerability
	virtual bool isInvulnerable() const = 0;

	/// Set NPC surfing data
	virtual void setSurfingData(const PlayerSurfingData& data) = 0;

	/// Get NPC surfing data
	virtual PlayerSurfingData getSurfingData() = 0;

	/// Reset NPC surfing data
	virtual void resetSurfingData() = 0;

	/// Check if NPC is moving towards a specific player
	virtual bool isMovingToPlayer(IPlayer& player) const = 0;

	/// Simulate NPC death
	virtual void kill(IPlayer* killer, uint8_t weapon) = 0;

	/// Get player aiming at
	virtual IPlayer* getPlayerAimingAt() = 0;

	/// Get player moving to
	virtual IPlayer* getPlayerMovingTo() = 0;

	/// Set weapon state
	virtual void setWeaponState(PlayerWeaponState state) = 0;

	/// Get position NPC is moving to
	virtual Vector3 getPositionMovingTo() const = 0;
};

struct NPCEventHandler
{
	virtual void onNPCFinishMove(INPC& npc) {};
	virtual void onNPCCreate(INPC& npc) {};
	virtual void onNPCDestroy(INPC& npc) {};
	virtual void onNPCSpawn(INPC& npc) {};
	virtual void onNPCRespawn(INPC& npc) {};
	virtual void onNPCWeaponStateChange(INPC& npc, PlayerWeaponState newState, PlayerWeaponState oldState) {};
	virtual bool onNPCTakeDamage(INPC& npc, IPlayer& damager, float damage, uint8_t weapon, BodyPart bodyPart) { return true; };
	virtual bool onNPCGiveDamage(INPC& npc, IPlayer& damaged, float damage, uint8_t weapon, BodyPart bodyPart) { return true; };
	virtual void onNPCDeath(INPC& npc, IPlayer* killer, int reason) {};
	virtual bool onNPCShotMissed(INPC& npc, const PlayerBulletData& bulletData) { return true; }
	virtual bool onNPCShotPlayer(INPC& npc, IPlayer& target, const PlayerBulletData& bulletData) { return true; }
	virtual bool onNPCShotNPC(INPC& npc, INPC& target, const PlayerBulletData& bulletData) { return true; }
	virtual bool onNPCShotVehicle(INPC& npc, IVehicle& target, const PlayerBulletData& bulletData) { return true; }
	virtual bool onNPCShotObject(INPC& npc, IObject& target, const PlayerBulletData& bulletData) { return true; }
	virtual bool onNPCShotPlayerObject(INPC& npc, IPlayerObject& target, const PlayerBulletData& bulletData) { return true; }
	virtual void onNPCPlaybackStart(INPC& npc, int recordId) { }
	virtual void onNPCPlaybackEnd(INPC& npc, int recordId) { }
	virtual void onNPCFinishNodePoint(INPC& npc, int nodeId, uint16_t pointId) { }
	virtual void onNPCFinishNode(INPC& npc, int nodeId) { }
	virtual bool onNPCChangeNode(INPC& npc, int newNodeId, int oldNodeId) { return true; }
	virtual void onNPCFinishMovePathPoint(INPC& npc, int pathId, int pointId) { }
	virtual void onNPCFinishMovePath(INPC& npc, int pathId) {};
};

static const UID NPCComponent_UID = UID(0x3D0E59E87F4E90BC);
struct INPCComponent : public IPool<INPC>, public INetworkComponent
{
	PROVIDE_UID(NPCComponent_UID)

	/// Get the npc event dispatcher
	virtual IEventDispatcher<NPCEventHandler>& getEventDispatcher() = 0;

	/// Create a controllable NPC
	virtual INPC* create(StringView name) = 0;

	/// Destroy an NPC. We need this because it's more than just an entity removal from a pool
	virtual void destroy(INPC& npc) = 0;

	/// Create path for NPC to use
	virtual int createPath() = 0;

	/// Destroy path
	virtual bool destroyPath(int pathId) = 0;

	/// Destroy all paths
	virtual void destroyAllPaths() = 0;

	/// Get number of paths
	virtual size_t getPathCount() const = 0;

	/// Adds a point with stop range to a path container
	virtual bool addPointToPath(int pathId, const Vector3& position, float stopRange = 1.0f) = 0;

	/// Removes a point added previously from a path
	virtual bool removePointFromPath(int pathId, size_t pointIndex) = 0;

	/// Removes all points from a path and makes it empty
	virtual bool clearPath(int pathId) = 0;

	/// Get number of points in a path
	virtual size_t getPathPointCount(int pathId) = 0;

	/// Gets point information by the given point index in a path
	virtual bool getPathPoint(int pathId, size_t pointIndex, Vector3& position, float& stopRange) = 0;

	virtual bool hasPathPointInRange(int pathId, const Vector3& position, float radius) = 0;

	/// Check if a path id is valid
	virtual bool isValidPath(int pathId) = 0;

	/// Load a record file for playback
	virtual int loadRecord(StringView filePath) = 0;

	/// Unload a previously loaded record
	virtual bool unloadRecord(int recordId) = 0;

	/// Check if a record ID is valid
	virtual bool isValidRecord(int recordId) = 0;

	/// Get total number of loaded records
	virtual size_t getRecordCount() const = 0;

	/// Unload all records
	virtual void unloadAllRecords() = 0;

	/// Open a node file for NPC path navigation
	virtual bool openNode(int nodeId) = 0;

	/// Close a previously opened node file
	virtual void closeNode(int nodeId) = 0;

	/// Check if a node is currently open
	virtual bool isNodeOpen(int nodeId) const = 0;

	/// Get node type information
	virtual uint8_t getNodeType(int nodeId) = 0;

	/// Set the current point in a node
	virtual bool setNodePoint(int nodeId, uint16_t pointId) = 0;

	/// Get the position of a specific point in a node
	virtual bool getNodePointPosition(int nodeId, Vector3& position) = 0;

	/// Get the total number of points in a node
	virtual int getNodePointCount(int nodeId) = 0;

	/// Get node information (vehicle nodes, pedestrian nodes, navigation nodes)
	virtual bool getNodeInfo(int nodeId, uint32_t& vehicleNodes, uint32_t& pedNodes, uint32_t& naviNodes) = 0;
};
