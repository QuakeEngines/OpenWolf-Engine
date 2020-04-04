////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2006 Tim Angus
// Copyright(C) 2011 - 2020 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code. If not, see <http://www.gnu.org/licenses/>.
//
// -------------------------------------------------------------------------------------
// File name:   bgame_api.h
// Created:
// Compilers:   Microsoft Visual C++ 2019, gcc (Ubuntu 8.3.0-6ubuntu1) 8.3.0
// Description: definitions shared by both the server game and client game modules
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __BG_API_H__
#define __BG_API_H__

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame
#define GAME_VERSION            "main"

#define DEFAULT_GRAVITY         800
#define VOTE_TIME               30000 // 30 seconds before vote times out
#define MINS_Z                  -24
#define DEFAULT_VIEWHEIGHT      26
#define CROUCH_VIEWHEIGHT       12
#define DEAD_VIEWHEIGHT         4 // height from ground
//#define DEAD_VIEWHEIGHT         -14 // watch for mins[ 2 ] less than this causing

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC            2
#define CS_MESSAGE          3   // from the map worldspawn's message field
#define CS_MOTD             4   // g_motd string for server message of the day
#define CS_WARMUP           5   // server time when the match will be restarted
// 6 UNUSED
// 7 UNUSED
#define CS_VOTE_TIME        8
#define CS_VOTE_STRING      9
#define CS_VOTE_YES         10
#define CS_VOTE_NO          11

#define CS_TEAMVOTE_TIME    12
#define CS_TEAMVOTE_STRING  14
#define CS_TEAMVOTE_YES     16
#define CS_TEAMVOTE_NO      18

#define CS_GAME_VERSION     20
#define CS_LEVEL_START_TIME 21    // so the timer only shows the current level
#define CS_INTERMISSION     22    // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_WINNER           23    // string indicating round winner
#define CS_SHADERSTATE      24
#define CS_BOTINFO          25
#define CS_CLIENTS_READY    26

#define CS_BUILDPOINTS      28
#define CS_STAGES           29
#define CS_WOLFINFO         30
#define CS_MODELS           31
#define CS_SOUNDS           (CS_MODELS+MAX_MODELS)
#define CS_SHADERS          (CS_SOUNDS+MAX_SOUNDS)
#define CS_PARTICLE_SYSTEMS (CS_SHADERS+MAX_GAME_SHADERS)
#define CS_PLAYERS          (CS_PARTICLE_SYSTEMS+MAX_GAME_PARTICLE_SYSTEMS)
#define CS_PRECACHES        (CS_PLAYERS+MAX_CLIENTS)
#define CS_LOCATIONS        (CS_PRECACHES+MAX_CLIENTS)
#define CS_BOTINFOS			(CS_LOCATIONS+MAX_LOCATIONS)	// cyr, should only be used for localhost games
#define CS_EFFECTS	        (CS_PARTICLE_SYSTEMS+MAX_LOCATIONS)
#define CS_LIGHT_STYLES     (CS_EFFECTS + MAX_FX)
#define CS_MAX              (CS_LIGHT_STYLES+MAX_CLIENTS)		// (CS_LOCATIONS+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum
{
    GENDER_MALE,
    GENDER_FEMALE,
    GENDER_NEUTER
} gender_t;

extern vec3_t playerMins;
extern vec3_t playerMaxs;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
    PM_NORMAL,        // can accelerate and turn
    PM_NOCLIP,        // noclip movement
    PM_SPECTATOR,     // still run into walls
    PM_JETPACK,       // jetpack physics
    PM_GRABBED,       // like dead, but for when the player is still live
    PM_DEAD,          // no acceleration or turning, but free falling
    PM_FREEZE,        // stuck in place with no control
    PM_INTERMISSION,  // no movement or status bar
} pmtype_t;

// pmtype_t categories
#define PM_Paralyzed( x ) ( (x) == PM_DEAD || (x) == PM_FREEZE ||\
                            (x) == PM_INTERMISSION )
#define PM_Live( x )      ( (x) == PM_NORMAL || (x) == PM_JETPACK ||\
                            (x) == PM_GRABBED )

typedef enum
{
    WEAPON_READY,
    WEAPON_RAISING,
    WEAPON_DROPPING,
    WEAPON_FIRING,
    WEAPON_RELOADING,
    WEAPON_NEEDS_RESET,
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_CROUCH_HELD     4
#define PMF_BACKWARDS_JUMP  8       // go into backwards land
#define PMF_BACKWARDS_RUN   16      // coast down to backwards run
#define PMF_TIME_LAND       32      // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64      // pm_time is an air-accelerate only time
#define PMF_TIME_KNOCKOFF   128     // pm_time is no-wallwalk time
#define PMF_TIME_WATERJUMP  256     // pm_time is waterjump
#define PMF_RESPAWNED       512     // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD   1024
#define PMF_WEAPON_RELOAD   2048    // force a weapon switch
#define PMF_FOLLOW          4096    // spectate following another player
#define PMF_QUEUED          8192    // player is queued
#define PMF_TIME_WALLJUMP   16384   // for limiting wall jumping
#define PMF_CHARGE          32768   // keep track of pouncing
#define PMF_WEAPON_SWITCH   65536   // force a weapon switch

#define PMF_ALL_TIMES (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK|PMF_TIME_KNOCKOFF|PMF_TIME_WALLJUMP)

typedef struct
{
    S32 pouncePayload;
    F32 fallVelocity;
} pmoveExt_t;

#define MAXTOUCH  32
#ifdef GAMEDLL
typedef struct gentity_s gentity_t;
#endif
typedef struct pmove_s
{
    // state (in / out)
    playerState_t* ps;
    pmoveExt_t* pmext;
    // command (in)
    usercmd_t cmd;
    S32 tracemask;      // collide against these types of surfaces
    S32 debugLevel;     // if set, diagnostic output will be printed
    bool noFootsteps;    // if the game is setup for no footsteps by the server
    bool autoWeaponHit[ 32 ];
    S32 framecount;
    // results (out)
    S32 numtouch;
    S32 touchents[ MAXTOUCH ];
    vec3_t mins, maxs;     // bounding box size
    S32 watertype;
    S32 waterlevel;
    F32 xyspeed;
#ifdef GAMEDLL
    struct gclient_s* gent;			// NULL if not a client
#endif
    // for fixed msec Pmove
    S32 pmove_fixed;
    S32 pmove_msec;
    S32 fixedPhysicsFPS;
    // callbacks to test the world
    // these will be different functions during game and cgame
    /*void (*trace)( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentMask );*/
    void ( *trace )( trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, S32 passEntityNum, S32 contentMask );
    S32( *pointcontents )( const vec3_t point, S32 passEntityNum );
} pmove_t;

// player_state->stats[] indexes
typedef enum
{
    STAT_HEALTH,
    STAT_ITEMS,
    STAT_ACTIVEITEMS,
    STAT_WEAPON,    // current primary weapon
    STAT_MAX_HEALTH,// health / armor limit, changable by handicap
    STAT_CLASS,     // player class (for aliens AND humans)
    STAT_TEAM,      // player team
    STAT_STAMINA,   // stamina (human only)
    STAT_STATE,     // client states e.g. wall climbing
    STAT_MISC,      // for uh...misc stuff (pounce, trample, lcannon)
    STAT_BUILDABLE, // which ghost model to display for building
    STAT_FALLDIST,  // the distance the player fell
    STAT_VIEWLOCK   // direction to lock the view in
} statIndex_t;

#define SCA_WALLCLIMBER         0x00000001
#define SCA_TAKESFALLDAMAGE     0x00000002
#define SCA_CANZOOM             0x00000004
#define SCA_FOVWARPS            0x00000008
#define SCA_ALIENSENSE          0x00000010
#define SCA_CANUSELADDERS       0x00000020
#define SCA_WALLJUMPER          0x00000040

#define SS_WALLCLIMBING         0x0001
#define SS_CREEPSLOWED          0x0002
#define SS_SPEEDBOOST           0x0004
#define SS_GRABBED              0x0008
#define SS_BLOBLOCKED           0x0010
#define SS_POISONED             0x0020
#define SS_HOVELING             0x0040
#define SS_BOOSTED              0x0080
#define SS_BOOSTEDWARNING       0x0100 // booster poison is running out
#define SS_SLOWLOCKED           0x0200
#define SS_CHARGING             0x0400
#define SS_HEALING_ACTIVE       0x0800 // medistat for humans, creep for aliens
#define SS_HEALING_2X           0x1000 // medkit or double healing rate
#define SS_HEALING_3X           0x2000 // triple healing rate

#define SB_VALID_TOGGLEBIT      0x00004000

#define MAX_STAMINA             1000

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum
{
    PERS_SCORE,           // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
    PERS_HITS,            // total points damage inflicted so damage beeps can sound on change
    PERS_SPAWNS,          // how many spawns your team has
    PERS_SPECSTATE,
    PERS_SPAWN_COUNT,     // incremented every respawn
    PERS_ATTACKER,        // clientnum of last damage inflicter
    PERS_KILLED,          // count of the number of times you died
    
    PERS_STATE,
    PERS_CREDIT,    // human credit
    PERS_QUEUEPOS,  // position in the spawn queue
    PERS_NEWWEAPON  // weapon to switch to
} persEnum_t;

#define PS_WALLCLIMBINGFOLLOW   0x00000001
#define PS_WALLCLIMBINGTOGGLE   0x00000002
#define PS_NONSEGMODEL          0x00000004
#define PS_ALWAYSSPRINT         0x00000008

// entityState_t->eFlags
#define EF_DEAD             0x00000001    // don't draw a foe marker over players with EF_DEAD
#define EF_TELEPORT_BIT     0x00000002    // toggled every time the origin abruptly changes
#define EF_PLAYER_EVENT     0x00000004
#define EF_BOUNCE           0x00000008    // for missiles
#define EF_BOUNCE_HALF      0x00000010    // for missiles
#define EF_NO_BOUNCE_SOUND  0x00000020    // for missiles
#define EF_B_SPAWNED        0x00000008    // buildable has spawned
#define EF_B_POWERED        0x00000010    // buildable is powered
#define EF_B_MARKED         0x00000020    // buildable is marked for deconstruction
#define EF_WALLCLIMB        0x00000040    // wall walking
#define EF_WALLCLIMBCEILING 0x00000080    // wall walking ceiling hack
#define EF_NODRAW           0x00000100    // may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000200    // for lightning gun
#define EF_FIRING2          0x00000400    // alt fire
#define EF_FIRING3          0x00000800    // third fire
#define EF_MOVER_STOP       0x00001000    // will push otherwise
#define EF_POISONCLOUDED    0x00002000    // player hit with basilisk gas
#define EF_CONNECTION       0x00004000    // draw a connection trouble sprite
#define EF_VOTED            0x00008000    // already cast a vote
#define EF_TEAMVOTED        0x00010000    // already cast a vote
#define EF_BLOBLOCKED       0x00020000    // caught by a trapper
#define EF_WARN_CHARGE      0x00040000    // Lucifer Cannon is about to overcharge

typedef enum
{
    WPM_NONE,
    
    WPM_PRIMARY,
    WPM_SECONDARY,
    WPM_TERTIARY,
    
    WPM_NOTFIRING,
    
    WPM_NUM_WEAPONMODES
} weaponMode_t;

typedef enum
{
    WP_NONE,
    
    WP_ALEVEL0,
    WP_ALEVEL1,
    WP_ALEVEL1_UPG,
    WP_ALEVEL2,
    WP_ALEVEL2_UPG,
    WP_ALEVEL3,
    WP_ALEVEL3_UPG,
    WP_ALEVEL4,
    
    WP_BLASTER,
    WP_MACHINEGUN,
    WP_PAIN_SAW,
    WP_SHOTGUN,
    WP_LAS_GUN,
    WP_MASS_DRIVER,
    WP_CHAINGUN,
    WP_PULSE_RIFLE,
    WP_FLAMER,
    WP_LUCIFER_CANNON,
    WP_GRENADE,
    
    WP_LOCKBLOB_LAUNCHER,
    WP_HIVE,
    WP_TESLAGEN,
    WP_MGTURRET,
    
    WP_ABUILD,
    WP_ABUILD2,
    WP_HBUILD,
    
    WP_NUM_WEAPONS
} weapon_t;

typedef enum
{
    AFEEDBACK_HIT,
    AFEEDBACK_MISS,
    AFEEDBACK_TEAMHIT,
    AFEEDBACK_RANGED_HIT,
    AFEEDBACK_RANGED_MISS,
    AFEEDBACK_RANGED_TEAMHIT,
    
    AFEEDBACK_NUM
} alienFeedback_t;

typedef enum
{
    UP_NONE,
    
    UP_LIGHTARMOUR,
    UP_HELMET,
    UP_MEDKIT,
    UP_BATTPACK,
    UP_JETPACK,
    UP_BATTLESUIT,
    UP_GRENADE,
    
    UP_AMMO,
    
    UP_NUM_UPGRADES
} upgrade_t;

// bitmasks for upgrade slots
#define SLOT_NONE       0x00000000
#define SLOT_HEAD       0x00000001
#define SLOT_TORSO      0x00000002
#define SLOT_ARMS       0x00000004
#define SLOT_LEGS       0x00000008
#define SLOT_BACKPACK   0x00000010
#define SLOT_WEAPON     0x00000020
#define SLOT_SIDEARM    0x00000040

typedef enum
{
    BA_NONE,
    
    BA_A_SPAWN,
    BA_A_OVERMIND,
    
    BA_A_BARRICADE,
    BA_A_ACIDTUBE,
    BA_A_TRAPPER,
    BA_A_BOOSTER,
    BA_A_HIVE,
    
    BA_A_HOVEL,
    
    BA_H_SPAWN,
    
    BA_H_MGTURRET,
    BA_H_TESLAGEN,
    
    BA_H_ARMOURY,
    BA_H_DCC,
    BA_H_MEDISTAT,
    
    BA_H_REACTOR,
    BA_H_REPEATER,
    
    BA_NUM_BUILDABLES
} buildable_t;


#define B_HEALTH_MASK 255

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1   0x00000100
#define EV_EVENT_BIT2   0x00000200
#define EV_EVENT_BITS   (EV_EVENT_BIT1|EV_EVENT_BIT2)

#define EVENT_VALID_MSEC  300

typedef enum
{
    EV_NONE,
    
    EV_FOOTSTEP,
    EV_FOOTSTEP_METAL,
    EV_FOOTSTEP_SQUELCH,
    EV_FOOTSPLASH,
    EV_FOOTWADE,
    EV_SWIM,
    
    EV_STEP_4,
    EV_STEP_8,
    EV_STEP_12,
    EV_STEP_16,
    
    EV_STEPDN_4,
    EV_STEPDN_8,
    EV_STEPDN_12,
    EV_STEPDN_16,
    
    EV_FALL_SHORT,
    EV_FALL_MEDIUM,
    EV_FALL_FAR,
    EV_FALLING,
    
    EV_JUMP,
    EV_WATER_TOUCH, // foot touches
    EV_WATER_LEAVE, // foot leaves
    EV_WATER_UNDER, // head touches
    EV_WATER_CLEAR, // head leaves
    
    EV_NOAMMO,
    EV_CHANGE_WEAPON,
    EV_FIRE_WEAPON,
    EV_FIRE_WEAPON2,
    EV_FIRE_WEAPON3,
    
    EV_PLAYER_RESPAWN, // for fovwarp effects
    EV_PLAYER_TELEPORT_IN,
    EV_PLAYER_TELEPORT_OUT,
    
    EV_GRENADE_BOUNCE,    // eventParm will be the soundindex
    
    EV_GENERAL_SOUND,
    EV_GLOBAL_SOUND,    // no attenuation
    
    EV_BULLET_HIT_FLESH,
    EV_BULLET_HIT_WALL,
    
    EV_SHOTGUN,
    EV_MASS_DRIVER,
    
    EV_MISSILE_HIT,
    EV_MISSILE_MISS,
    EV_MISSILE_MISS_METAL,
    EV_TESLATRAIL,
    EV_BULLET,        // otherEntity is the shooter
    
    EV_LEV1_GRAB,
    EV_LEV4_TRAMPLE_PREPARE,
    EV_LEV4_TRAMPLE_START,
    
    EV_PAIN,
    EV_DEATH1,
    EV_DEATH2,
    EV_DEATH3,
    EV_OBITUARY,
    
    EV_GIB_PLAYER,      // gib a previously living player
    
    EV_BUILD_CONSTRUCT,
    EV_BUILD_DESTROY,
    EV_BUILD_DELAY,     // can't build yet
    EV_BUILD_REPAIR,    // repairing buildable
    EV_BUILD_REPAIRED,  // buildable has full health
    EV_HUMAN_BUILDABLE_EXPLOSION,
    EV_ALIEN_BUILDABLE_EXPLOSION,
    EV_ALIEN_ACIDTUBE,
    
    EV_MEDKIT_USED,
    
    EV_ALIEN_EVOLVE,
    EV_ALIEN_EVOLVE_FAILED,
    
    EV_DEBUG_LINE,
    EV_STOPLOOPINGSOUND,
    EV_TAUNT,
    
    EV_OVERMIND_ATTACK, // overmind under attack
    EV_OVERMIND_DYING,  // overmind close to death
    EV_OVERMIND_SPAWNS, // overmind needs spawns
    
    EV_DCC_ATTACK,      // dcc under attack
    
    EV_MGTURRET_SPINUP, // turret spinup sound should play
    
    EV_RPTUSE_SOUND,    // trigger a sound
    
    EV_ALIEN_HIT,       // Alien attack feedback hit enemy
    EV_ALIEN_MISS,      // Alien attack feedback miss enemy
    EV_ALIEN_TEAMHIT,   // Alien attack feedback hit teammate
    
    EV_ALIENRANGED_HIT,       // Alien ranged attack feedback hit enemy
    EV_ALIENRANGED_MISS,      // Alien ranged attack feedback miss enemy
    EV_ALIENRANGED_TEAMHIT,   // Alien ranged attack feedback hit teammate
    
    EV_LEV2_ZAP
} entity_event_t;

typedef enum
{
    MN_TEAM,
    MN_A_TEAMFULL,
    MN_H_TEAMFULL,
    
    // cmd stuff
    MN_CMD_CHEAT,
    MN_CMD_CHEAT_TEAM,
    MN_CMD_TEAM,
    MN_CMD_SPEC,
    MN_CMD_ALIEN,
    MN_CMD_HUMAN,
    MN_CMD_LIVING,
    
    //alien stuff
    MN_A_CLASS,
    MN_A_BUILD,
    MN_A_INFEST,
    MN_A_HOVEL_OCCUPIED,
    MN_A_HOVEL_BLOCKED,
    MN_A_NOEROOM,
    MN_A_TOOCLOSE,
    MN_A_NOOVMND_EVOLVE,
    MN_A_TEAMCHANGEBUILDTIMER,
    MN_A_EVOLVEBUILDTIMER,
    MN_A_CANTEVOLVE,
    MN_A_EVOLVEWALLWALK,
    MN_A_UNKNOWNCLASS,
    MN_A_CLASSNOTSPAWN,
    MN_A_CLASSNOTALLOWED,
    MN_A_CLASSNOTATSTAGE,
    
    //shared build
    MN_B_NOROOM,
    MN_B_NORMAL,
    MN_B_CANNOT,
    MN_B_LASTSPAWN,
    MN_B_SUDDENDEATH,
    MN_B_REVOKED,
    MN_B_SURRENDER,
    
    //alien build
    MN_A_ONEOVERMIND,
    MN_A_ONEHOVEL,
    MN_A_NOBP,
    MN_A_NOCREEP,
    MN_A_NOOVMND,
    MN_A_HOVEL_EXIT,
    
    //human stuff
    MN_H_SPAWN,
    MN_H_BUILD,
    MN_H_ARMOURY,
    MN_H_UNKNOWNITEM,
    MN_H_NOSLOTS,
    MN_H_NOFUNDS,
    MN_H_ITEMHELD,
    MN_H_TEAMCHANGEBUILDTIMER,
    MN_H_NOARMOURYHERE,
    MN_H_NOENERGYAMMOHERE,
    MN_H_NOROOMBSUITON,
    MN_H_NOROOMBSUITOFF,
    MN_H_ARMOURYBUILDTIMER,
    MN_H_DEADTOCLASS,
    MN_H_UNKNOWNSPAWNITEM,
    
    //human build
    MN_H_NOPOWERHERE,
    MN_H_NOBP,
    MN_H_NOTPOWERED,
    MN_H_NODCC,
    MN_H_ONEREACTOR,
    MN_H_RPTPOWERHERE,
} dynMenu_t;

// animations
typedef enum
{
    BOTH_DEATH1,
    BOTH_DEAD1,
    BOTH_DEATH2,
    BOTH_DEAD2,
    BOTH_DEATH3,
    BOTH_DEAD3,
    
    TORSO_GESTURE,
    
    TORSO_ATTACK,
    TORSO_ATTACK2,
    
    TORSO_DROP,
    TORSO_RAISE,
    
    TORSO_STAND,
    TORSO_STAND2,
    
    LEGS_WALKCR,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,
    
    LEGS_JUMP,
    LEGS_LAND,
    
    LEGS_JUMPB,
    LEGS_LANDB,
    
    LEGS_IDLE,
    LEGS_IDLECR,
    
    LEGS_TURN,
    
    TORSO_GETFLAG,
    TORSO_GUARDBASE,
    TORSO_PATROL,
    TORSO_FOLLOWME,
    TORSO_AFFIRMATIVE,
    TORSO_NEGATIVE,
    
    MAX_PLAYER_ANIMATIONS,
    
    LEGS_BACKCR,
    LEGS_BACKWALK,
    FLAG_RUN,
    FLAG_STAND,
    FLAG_STAND2RUN,
    
    MAX_PLAYER_TOTALANIMATIONS
} playerAnimNumber_t;

// nonsegmented animations
typedef enum
{
    NSPA_STAND,
    
    NSPA_GESTURE,
    
    NSPA_WALK,
    NSPA_RUN,
    NSPA_RUNBACK,
    NSPA_CHARGE,
    
    NSPA_RUNLEFT,
    NSPA_WALKLEFT,
    NSPA_RUNRIGHT,
    NSPA_WALKRIGHT,
    
    NSPA_SWIM,
    
    NSPA_JUMP,
    NSPA_LAND,
    NSPA_JUMPBACK,
    NSPA_LANDBACK,
    
    NSPA_TURN,
    
    NSPA_ATTACK1,
    NSPA_ATTACK2,
    NSPA_ATTACK3,
    
    NSPA_PAIN1,
    NSPA_PAIN2,
    
    NSPA_DEATH1,
    NSPA_DEAD1,
    NSPA_DEATH2,
    NSPA_DEAD2,
    NSPA_DEATH3,
    NSPA_DEAD3,
    
    MAX_NONSEG_PLAYER_ANIMATIONS,
    
    NSPA_WALKBACK,
    
    MAX_NONSEG_PLAYER_TOTALANIMATIONS
} nonSegPlayerAnimNumber_t;

// for buildable animations
typedef enum
{
    BANIM_NONE,
    
    BANIM_CONSTRUCT1,
    BANIM_CONSTRUCT2,
    
    BANIM_IDLE1,
    BANIM_IDLE2,
    BANIM_IDLE3,
    
    BANIM_ATTACK1,
    BANIM_ATTACK2,
    
    BANIM_SPAWN1,
    BANIM_SPAWN2,
    
    BANIM_PAIN1,
    BANIM_PAIN2,
    
    BANIM_DESTROY1,
    BANIM_DESTROY2,
    BANIM_DESTROYED,
    
    MAX_BUILDABLE_ANIMATIONS
} buildableAnimNumber_t;

typedef enum
{
    WANIM_NONE,
    
    WANIM_IDLE,
    
    WANIM_DROP,
    WANIM_RELOAD,
    WANIM_RAISE,
    
    WANIM_ATTACK1,
    WANIM_ATTACK2,
    WANIM_ATTACK3,
    WANIM_ATTACK4,
    WANIM_ATTACK5,
    WANIM_ATTACK6,
    WANIM_ATTACK7,
    WANIM_ATTACK8,
    
    MAX_WEAPON_ANIMATIONS
} weaponAnimNumber_t;

typedef struct animation_s
{
    S32 firstFrame;
    S32 numFrames;
    S32 loopFrames;     // 0 to numFrames
    S32 frameLerp;      // msec between frames
    S32 initialLerp;    // msec to get to first frame
    S32 reversed;     // true if animation is reversed
    S32 flipflop;     // true if animation should flipflop back to base
} animation_t;

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT    0x80
#define ANIM_FORCEBIT     0x40

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME   1000

// How many players on the overlay
#define TEAM_MAXOVERLAY   32

// player classes
typedef enum
{
    PCL_NONE,
    
    //builder classes
    PCL_ALIEN_BUILDER0,
    PCL_ALIEN_BUILDER0_UPG,
    
    //offensive classes
    PCL_ALIEN_LEVEL0,
    PCL_ALIEN_LEVEL1,
    PCL_ALIEN_LEVEL1_UPG,
    PCL_ALIEN_LEVEL2,
    PCL_ALIEN_LEVEL2_UPG,
    PCL_ALIEN_LEVEL3,
    PCL_ALIEN_LEVEL3_UPG,
    PCL_ALIEN_LEVEL4,
    
    //human class
    PCL_HUMAN,
    PCL_HUMAN_BSUIT,
    
    PCL_NUM_CLASSES
} class_t;

// spectator state
typedef enum
{
    SPECTATOR_NOT,
    SPECTATOR_FREE,
    SPECTATOR_LOCKED,
    SPECTATOR_FOLLOW,
    SPECTATOR_SCOREBOARD
} spectatorState_t;

// player teams
typedef enum
{
    TEAM_NONE,
    TEAM_ALIENS,
    TEAM_HUMANS,
    
    NUM_TEAMS
} team_t;


// means of death
typedef enum
{
    MOD_UNKNOWN,
    MOD_SHOTGUN,
    MOD_BLASTER,
    MOD_PAINSAW,
    MOD_MACHINEGUN,
    MOD_CHAINGUN,
    MOD_PRIFLE,
    MOD_MDRIVER,
    MOD_LASGUN,
    MOD_LCANNON,
    MOD_LCANNON_SPLASH,
    MOD_FLAMER,
    MOD_FLAMER_SPLASH,
    MOD_GRENADE,
    MOD_WATER,
    MOD_SLIME,
    MOD_LAVA,
    MOD_CRUSH,
    MOD_TELEFRAG,
    MOD_FALLING,
    MOD_SUICIDE,
    MOD_DECONSTRUCT,
    MOD_NOCREEP,
    MOD_TARGET_LASER,
    MOD_TRIGGER_HURT,
    
    MOD_ABUILDER_CLAW,
    MOD_LEVEL0_BITE,
    MOD_LEVEL1_CLAW,
    MOD_LEVEL1_PCLOUD,
    MOD_LEVEL3_CLAW,
    MOD_LEVEL3_POUNCE,
    MOD_LEVEL3_BOUNCEBALL,
    MOD_LEVEL2_CLAW,
    MOD_LEVEL2_ZAP,
    MOD_LEVEL4_CLAW,
    MOD_LEVEL4_TRAMPLE,
    MOD_LEVEL4_CRUSH,
    
    MOD_SLOWBLOB,
    MOD_POISON,
    MOD_SWARM,
    
    MOD_HSPAWN,
    MOD_TESLAGEN,
    MOD_MGTURRET,
    MOD_REACTOR,
    
    MOD_ASPAWN,
    MOD_ATUBE,
    MOD_OVERMIND
} meansOfDeath_t;


//---------------------------------------------------------

// player class record
typedef struct
{
    class_t number;
    
    UTF8* name;
    UTF8* info;
    
    S32 stages;
    
    S32 health;
    F32 fallDamage;
    F32 regenRate;
    
    S32 abilities;
    
    weapon_t startWeapon;
    
    F32 buildDist;
    
    S32 fov;
    F32 bob;
    F32 bobCycle;
    S32 steptime;
    
    F32 speed;
    F32 acceleration;
    F32 airAcceleration;
    F32 friction;
    F32 stopSpeed;
    F32 jumpMagnitude;
    F32 knockbackScale;
    
    S32 children[ 3 ];
    S32 cost;
    S32 value;
} classAttributes_t;

typedef struct
{
    UTF8 modelName[ MAX_QPATH ];
    F32 modelScale;
    UTF8 skinName[ MAX_QPATH ];
    F32 shadowScale;
    UTF8 hudName[ MAX_QPATH ];
    UTF8 humanName[ MAX_STRING_CHARS ];
    
    vec3_t mins;
    vec3_t maxs;
    vec3_t crouchMaxs;
    vec3_t deadMins;
    vec3_t deadMaxs;
    S32 viewheight;
    S32 crouchViewheight;
    F32 zOffset;
} classConfig_t;

//stages
typedef enum
{
    S1,
    S2,
    S3
} stage_t;

#define MAX_BUILDABLE_MODELS 4

// buildable item record
typedef struct
{
    buildable_t number;
    
    UTF8* name;
    UTF8* humanName;
    UTF8* info;
    UTF8* entityName;
    
    trType_t traj;
    F32 bounce;
    
    S32 buildPoints;
    S32 stages;
    
    S32 health;
    S32 regenRate;
    
    S32 splashDamage;
    S32 splashRadius;
    
    S32 meansOfDeath;
    
    team_t team;
    weapon_t buildWeapon;
    
    S32 idleAnim;
    
    S32 nextthink;
    S32 buildTime;
    bool usable;
    
    S32 turretRange;
    S32 turretFireSpeed;
    weapon_t turretProjType;
    
    F32 minNormal;
    bool invertNormal;
    
    bool creepTest;
    S32 creepSize;
    
    bool dccTest;
    bool transparentTest;
    bool uniqueTest;
    
    S32 value;
} buildableAttributes_t;

typedef struct
{
    UTF8 models[ MAX_BUILDABLE_MODELS ][ MAX_QPATH ];
    
    F32 modelScale;
    vec3_t mins;
    vec3_t maxs;
    F32 zOffset;
} buildableConfig_t;

// weapon record
typedef struct
{
    weapon_t  number;
    
    S32 price;
    S32 stages;
    
    S32 slots;
    
    UTF8* name;
    UTF8* humanName;
    UTF8* info;
    
    S32 maxAmmo;
    S32 maxClips;
    bool infiniteAmmo;
    bool usesEnergy;
    
    S32 repeatRate1;
    S32 repeatRate2;
    S32 repeatRate3;
    S32 reloadTime;
    F32 knockbackScale;
    
    bool hasAltMode;
    bool hasThirdMode;
    
    bool canZoom;
    F32 zoomFov;
    
    bool purchasable;
    bool longRanged;
    
    team_t team;
} weaponAttributes_t;

// upgrade record
typedef struct
{
    upgrade_t number;
    
    S32 price;
    S32 stages;
    
    S32 slots;
    
    UTF8* name;
    UTF8* humanName;
    UTF8* info;
    
    UTF8* icon;
    
    bool purchasable;
    bool usable;
    
    team_t    team;
} upgradeAttributes_t;

// content masks
#define MASK_ALL          (-1)
#define MASK_SOLID        (CONTENTS_SOLID)
#define MASK_PLAYERSOLID  (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define MASK_DEADSOLID    (CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define MASK_WATER        (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE       (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT         (CONTENTS_SOLID|CONTENTS_BODY)

#define ARENAS_PER_TIER 4
#define MAX_ARENAS      1024
#define MAX_ARENAS_TEXT 8192

#define MAX_BOTS        1024
#define MAX_BOTS_TEXT   8192

// Friendly Fire Flags
#define FFF_HUMANS         1
#define FFF_ALIENS         2
#define FFF_BUILDABLES     4

// bg_voice.c
#define MAX_VOICES                8
#define MAX_VOICE_NAME_LEN        16
#define MAX_VOICE_CMD_LEN         16
#define VOICE_ENTHUSIASM_DECAY    0.5f // enthusiasm lost per second

typedef enum
{
    VOICE_CHAN_ALL,
    VOICE_CHAN_TEAM ,
    VOICE_CHAN_LOCAL,
    
    VOICE_CHAN_NUM_CHANS
} voiceChannel_t;

typedef struct voiceTrack_s
{
#ifdef CGAMEDLL
    sfxHandle_t track;
    S32 duration;
#endif
    UTF8* text;
    S32 enthusiasm;
    S32 team;
    S32 _class;
    S32 weapon;
    struct voiceTrack_s* next;
} voiceTrack_t;


typedef struct voiceCmd_s
{
    UTF8 cmd[ MAX_VOICE_CMD_LEN ];
    voiceTrack_t* tracks;
    struct voiceCmd_s* next;
} voiceCmd_t;

typedef struct voice_s
{
    UTF8 name[ MAX_VOICE_NAME_LEN ];
    voiceCmd_t* cmds;
    struct voice_s* next;
} voice_t;

//
// bgGame
//
class bgGame
{
public:
    virtual const buildableAttributes_t* BuildableByName( StringEntry name ) = 0;
    virtual const buildableAttributes_t* BuildableByEntityName( StringEntry name ) = 0;
    virtual const buildableAttributes_t* Buildable( buildable_t buildable ) = 0;
    virtual bool BuildableAllowedInStage( buildable_t buildable, stage_t stage ) = 0;
    virtual buildableConfig_t* BuildableConfig( buildable_t buildable ) = 0;
    virtual void BuildableBoundingBox( buildable_t buildable, vec3_t mins, vec3_t maxs ) = 0;
    virtual bool ParseBuildableFile( StringEntry filename, buildableConfig_t* bc ) = 0;
    virtual void InitBuildableConfigs( void ) = 0;
    virtual const classAttributes_t* ClassByName( StringEntry name ) = 0;
    virtual const classAttributes_t* Class( class_t _class ) = 0;
    virtual bool ClassAllowedInStage( class_t _class, stage_t stage ) = 0;
    virtual classConfig_t* ClassConfig( class_t _class ) = 0;
    virtual void ClassBoundingBox( class_t _class, vec3_t mins, vec3_t maxs, vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs ) = 0;
    virtual bool ClassHasAbility( class_t _class, S32 ability ) = 0;
    virtual S32 ClassCanEvolveFromTo( class_t fclass, class_t tclass, S32 credits, S32 stage, S32 cost ) = 0;
    virtual bool AlienCanEvolve( class_t pclass, S32 credits, S32 stage ) = 0;
    virtual bool ParseClassFile( StringEntry filename, classConfig_t* cc ) = 0;
    virtual void InitClassConfigs( void ) = 0;
    virtual const weaponAttributes_t* WeaponByName( StringEntry name ) = 0;
    virtual const weaponAttributes_t* Weapon( weapon_t weapon ) = 0;
    virtual bool WeaponAllowedInStage( weapon_t weapon, stage_t stage ) = 0;
    virtual const upgradeAttributes_t* UpgradeByName( StringEntry name ) = 0;
    virtual const upgradeAttributes_t* Upgrade( upgrade_t upgrade ) = 0;
    virtual bool UpgradeAllowedInStage( upgrade_t upgrade, stage_t stage ) = 0;
    virtual void EvaluateTrajectory( const trajectory_t* tr, S32 atTime, vec3_t result ) = 0;
    virtual void EvaluateTrajectoryDelta( const trajectory_t* tr, S32 atTime, vec3_t result ) = 0;
    virtual StringEntry EventName( S32 num ) = 0;
    virtual void AddPredictableEventToPlayerstate( S32 newEvent, S32 eventParm, playerState_t* ps ) = 0;
    virtual void PlayerStateToEntityState( playerState_t* ps, entityState_t* s, bool snap ) = 0;
    virtual void PlayerStateToEntityStateExtraPolate( playerState_t* ps, entityState_t* s, S32 time, bool snap ) = 0;
    virtual bool WeaponIsFull( weapon_t weapon, S32 stats[], S32 ammo, S32 clips ) = 0;
    virtual bool InventoryContainsWeapon( S32 weapon, S32 stats[] ) = 0;
    virtual S32 CalculateSlotsForInventory( S32 stats[] ) = 0;
    virtual void AddUpgradeToInventory( S32 item, S32 stats[] ) = 0;
    virtual void RemoveUpgradeFromInventory( S32 item, S32 stats[] ) = 0;
    virtual bool InventoryContainsUpgrade( S32 item, S32 stats[] ) = 0;
    virtual void ActivateUpgrade( S32 item, S32 stats[] ) = 0;
    virtual void DeactivateUpgrade( S32 item, S32 stats[] ) = 0;
    virtual bool UpgradeIsActive( S32 item, S32 stats[] ) = 0;
    virtual bool RotateAxis( vec3_t surfNormal, vec3_t inAxis[3], vec3_t outAxis[3], bool inverse, bool ceiling ) = 0;
    virtual void GetClientNormal( const playerState_t* ps, vec3_t normal ) = 0;
    virtual void PositionBuildableRelativeToPlayer( const playerState_t* ps, const vec3_t mins, const vec3_t maxs, void( *trace )( trace_t*, const vec3_t, const vec3_t, const vec3_t, const vec3_t, S32, S32 ), vec3_t outOrigin, vec3_t outAngles, trace_t* tr ) = 0;
    virtual S32 GetValueOfPlayer( playerState_t* ps ) = 0;
    virtual S32 PlayerPoisonCloudTime( playerState_t* ps ) = 0;
    virtual weapon_t GetPlayerWeapon( playerState_t* ps ) = 0;
    virtual bool PlayerCanChangeWeapon( playerState_t* ps ) = 0;
    virtual F32 atof_neg( UTF8* token, bool allowNegative ) = 0;
    virtual S32 atoi_neg( UTF8* token, bool allowNegative ) = 0;
    virtual void ParseCSVEquipmentList( StringEntry string, weapon_t* weapons, S32 weaponsSize, upgrade_t* upgrades, S32 upgradesSize ) = 0;
    virtual void ParseCSVClassList( StringEntry string, class_t* classes, S32 classesSize ) = 0;
    virtual void ParseCSVBuildableList( StringEntry string, buildable_t* buildables, S32 buildablesSize ) = 0;
    virtual void InitAllowedGameElements( void ) = 0;
    virtual bool WeaponIsAllowed( weapon_t weapon ) = 0;
    virtual bool UpgradeIsAllowed( upgrade_t upgrade ) = 0;
    virtual bool ClassIsAllowed( class_t _class ) = 0;
    virtual bool BuildableIsAllowed( buildable_t buildable ) = 0;
    virtual bool ClientListTest( clientList_t* list, S32 clientNum ) = 0;
    virtual void ClientListAdd( clientList_t* list, S32 clientNum ) = 0;
    virtual void ClientListRemove( clientList_t* list, S32 clientNum ) = 0;
    virtual UTF8* ClientListString( clientList_t* list ) = 0;
    virtual void ClientListParse( clientList_t* list, StringEntry s ) = 0;
    virtual weapon_t PrimaryWeapon( S32 stats[] ) = 0;
    virtual S32 LoadEmoticons( UTF8 names[][MAX_EMOTICON_NAME_LEN], S32 widths[] ) = 0;
    virtual UTF8* TeamName( team_t team ) = 0;
    virtual bool SlideMove( bool gravity ) = 0;
    virtual void StepEvent( vec3_t from, vec3_t to, vec3_t normal ) = 0;
    virtual bool StepSlideMove( bool gravity, bool predictive ) = 0;
    virtual bool PredictStepMove( void ) = 0;
    virtual void AddEvent( S32 newEvent ) = 0;
    virtual void AddTouchEnt( S32 entityNum ) = 0;
    virtual void StartTorsoAnim( S32 anim ) = 0;
    virtual void StartLegsAnim( S32 anim ) = 0;
    virtual void ContinueLegsAnim( S32 anim ) = 0;
    virtual void ContinueTorsoAnim( S32 anim ) = 0;
    virtual void ForceLegsAnim( S32 anim ) = 0;
    virtual void ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, F32 overbounce ) = 0;
    virtual void Friction( void ) = 0;
    virtual void Accelerate( vec3_t wishdir, F32 wishspeed, F32 accel ) = 0;
    virtual F32 CmdScale( usercmd_t* cmd ) = 0;
    virtual void SetMovementDir( void ) = 0;
    virtual void CheckCharge( void ) = 0;
    virtual bool CheckPounce( void ) = 0;
    virtual bool CheckWallJump( void ) = 0;
    virtual bool CheckJump( void ) = 0;
    virtual bool CheckWaterJump( void ) = 0;
    virtual bool CheckDodge( void ) = 0;
    virtual void WaterJumpMove( void ) = 0;
    virtual void WaterMove( void ) = 0;
    virtual void JetPackMove( void ) = 0;
    virtual void FlyMove( void ) = 0;
    virtual void AirMove( void ) = 0;
    virtual void ClimbMove( void ) = 0;
    virtual void WalkMove( void ) = 0;
    virtual void LadderMove( void ) = 0;
    virtual void CheckLadder( void ) = 0;
    virtual void DeadMove( void ) = 0;
    virtual void NoclipMove( void ) = 0;
    virtual S32 FootstepForSurface( void ) = 0;
    virtual void CrashLand( void ) = 0;
    virtual S32 CorrectAllSolid( trace_t* trace ) = 0;
    virtual void GroundTraceMissed( void ) = 0;
    virtual void GroundClimbTrace( void ) = 0;
    virtual void GroundTrace( void ) = 0;
    virtual void SetWaterLevel( void ) = 0;
    virtual void SetViewheight( void ) = 0;
    virtual void CheckDuck( void ) = 0;
    virtual void Footsteps( void ) = 0;
    virtual void WaterEvents( void ) = 0;
    virtual void BeginWeaponChange( S32 weapon ) = 0;
    virtual void FinishWeaponChange( void ) = 0;
    virtual void TorsoAnimation( void ) = 0;
    virtual void Weapon( void ) = 0;
    virtual void Animate( void ) = 0;
    virtual void DropTimers( void ) = 0;
    virtual void UpdateViewAngles( playerState_t* ps, const usercmd_t* cmd ) = 0;
    virtual void PmoveSingle( pmove_t* pmove ) = 0;
    virtual void Pmove( pmove_t* pmove ) = 0;
    virtual void VoiceParseError( fileHandle_t handle, UTF8* err ) = 0;
    virtual voice_t* VoiceList( void ) = 0;
    virtual bool VoiceParseTrack( S32 handle, voiceTrack_t* voiceTrack ) = 0;
    virtual voiceTrack_t* VoiceParseCommand( S32 handle ) = 0;
    virtual voiceCmd_t* VoiceParse( UTF8* name ) = 0;
    virtual voice_t* VoiceInit( void ) = 0;
    virtual void PrintVoices( voice_t* voices, S32 debugLevel ) = 0;
    virtual voice_t* VoiceByName( voice_t* head, UTF8* name ) = 0;
    virtual voiceCmd_t* VoiceCmdFind( voiceCmd_t* head, UTF8* name, S32* cmdNum ) = 0;
    virtual voiceCmd_t* VoiceCmdByNum( voiceCmd_t* head, S32 num ) = 0;
    virtual voiceTrack_t* VoiceTrackByNum( voiceTrack_t* head, S32 num ) = 0;
    virtual voiceTrack_t* VoiceTrackFind( voiceTrack_t* head, team_t team, class_t _class, weapon_t weapon, S32 enthusiasm, S32* trackNum ) = 0;
    virtual void* Alloc( size_t size ) = 0;
    virtual void Free( void* ptr ) = 0;
    virtual void InitMemory( void ) = 0;
    virtual void DefragmentMemory( void ) = 0;
};

extern bgGame* bggame;

#endif //!__BG_API_H__
