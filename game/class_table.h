// Table of classes

#ifndef CLASS_TABLE_H
#define CLASS_TABLE_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/common/const.h>

#include <vector>

//-----------------------------------------------------------------------------
// Class flags
//-----------------------------------------------------------------------------

// 16 flags only
#define FL_CLASS_FRIEND (1 << 0)
#define FL_CLASS_ENEMY (1 << 1)
#define FL_CLASS_ITEM (1 << 2)
#define FL_CLASS_NEUTRAL (1 << 3)
#define FL_CLASS_WORLD_ENTITY (1 << 4)
#define FL_CLASS_CORPSE (1 << 5)
#define FL_CLASS_UNDEFINED_7 (1 << 6)
#define FL_CLASS_UNDEFINED_8 (1 << 7)
#define FL_CLASS_UNDEFINED_9 (1 << 8)
#define FL_CLASS_UNDEFINED_10 (1 << 9)
#define FL_CLASS_UNDEFINED_11 (1 << 10)
#define FL_CLASS_UNDEFINED_12 (1 << 11)
#define FL_CLASS_UNDEFINED_13 (1 << 12)
#define FL_CLASS_UNDEFINED_14 (1 << 13)
#define FL_CLASS_UNDEFINED_15 (1 << 14)
#define FL_CLASS_UNDEFINED_16 (1 << 15)

//-----------------------------------------------------------------------------
// Class ID
//-----------------------------------------------------------------------------

enum eClassID
{
	CLASS_NONE = 0,

	CLASS_PLAYER,

	CLASS_OBJECT_CP,

	// NPCs
	CLASS_NPC_GORDON_FREEMAN,
	CLASS_NPC_SCIENTIST,
	CLASS_NPC_BARNEY,
	CLASS_NPC_OTIS,
	CLASS_NPC_HEADCRAB,
	CLASS_NPC_BABY_HEADCRAB,
	CLASS_NPC_ZOMBIE,
	CLASS_NPC_ZOMBIE_SOLDIER,
	CLASS_NPC_BULLSQUID,
	CLASS_NPC_HOUNDEYE,
	CLASS_NPC_BARNACLE,
	CLASS_NPC_VORTIGAUNT,
	CLASS_NPC_HUMAN_GRUNT,
	CLASS_NPC_HUMAN_GRUNT_SNIPER,
	CLASS_NPC_ALIEN_GRUNT,
	CLASS_NPC_TENTACLE,
	CLASS_NPC_SENTRY,
	CLASS_NPC_TURRET,
	CLASS_NPC_LEECH,
	CLASS_NPC_GMAN,
	CLASS_NPC_FEMALE_ASSASSIN,
	CLASS_NPC_MALE_ASSASSIN,
	CLASS_NPC_MALE_SNIPER_ASSASSIN,
	CLASS_NPC_AGENT,
	CLASS_NPC_SNARK,
	CLASS_NPC_CHUMTOAD,
	CLASS_NPC_ALIEN_CONTROLLER,
	CLASS_NPC_ICHTYOSAUR,
	CLASS_NPC_GARGANTUA,
	CLASS_NPC_BABY_GARGANTUA,
	CLASS_NPC_BIG_MOMMA,
	CLASS_NPC_OSPREY,
	CLASS_NPC_BLACK_OPS_OSPREY,
	CLASS_NPC_DESTROYED_OSPREY,
	CLASS_NPC_APACHE,
	CLASS_NPC_NIHILANTH,
	CLASS_NPC_BOID,
	CLASS_NPC_HEV,
	CLASS_NPC_SPORE_AMMO,
	CLASS_NPC_HUMAN_GRUNT_OPFOR,
	CLASS_NPC_HUMAN_GRUNT_OPFOR_TORCH,
	CLASS_NPC_HUMAN_GRUNT_OPFOR_MEDIC,
	CLASS_NPC_GONOME,
	CLASS_NPC_PIT_DRONE,
	CLASS_NPC_SHOCK_TROOPER,
	CLASS_NPC_VOLTIGORE,
	CLASS_NPC_BABY_VOLTIGORE,
	CLASS_NPC_PIT_WORM,
	CLASS_NPC_GENEWORM,
	CLASS_NPC_SHOCK_RIFLE,
	CLASS_NPC_MORTAR,
	CLASS_NPC_STUKABAT,
	CLASS_NPC_KINGPIN,
	CLASS_NPC_TOR,
	CLASS_NPC_HEAVY_GRUNT,
	CLASS_NPC_ROBOT_GRUNT,
	CLASS_NPC_ZOMBIE_BULL,
	CLASS_NPC_THE_HAND,
	CLASS_NPC_CHICKEN,
	CLASS_NPC_SHERIFF,
	CLASS_NPC_CYBERFRANKLIN,
	CLASS_NPC_MANTA,
	CLASS_NPC_BARNABUS, // wtf is this?
	CLASS_NPC_SKELETON,
	CLASS_NPC_PIRANHA,

	// Wouldn't add them tbh
	CLASS_NPC_SPECFOR_GRUNT,
	CLASS_NPC_BARNIEL,
	CLASS_NPC_ARCHER,
	CLASS_NPC_PANTHEREYE,
	CLASS_NPC_FIONA,
	CLASS_NPC_TWITCHER,
	CLASS_NPC_SPITTER,
	CLASS_NPC_HANDCRAB,
	CLASS_NPC_GHOST,
	CLASS_NPC_SCREAMER,
	CLASS_NPC_DEVOURER,
	CLASS_NPC_WHEELCHAIR,
	CLASS_NPC_FACE,
	CLASS_NPC_HELLHOUND,
	CLASS_NPC_ADDICTION,
	CLASS_NPC_SHARK,

	// Items
	CLASS_ITEM_HEV,
	CLASS_ITEM_MEDKIT,
	CLASS_ITEM_BATTERY,
	CLASS_ITEM_GLOCK_AMMO,
	CLASS_ITEM_PYTHON_AMMO,
	CLASS_ITEM_SHOTGUN_AMMO,
	CLASS_ITEM_UZI_AMMO,
	CLASS_ITEM_MP5_AMMO,
	CLASS_ITEM_MP5_AMMO2,
	CLASS_ITEM_CHAIN_AMMO,
	CLASS_ITEM_CROSSBOW_AMMO,
	CLASS_ITEM_GAUSS_AMMO,
	CLASS_ITEM_RPG_AMMO,
	CLASS_ITEM_SNIPER_RIFLE_AMMO,
	CLASS_ITEM_MACHINEGUN_AMMO,
	CLASS_ITEM_CROWBAR,
	CLASS_ITEM_WRENCH,
	CLASS_ITEM_KNIFE,
	CLASS_ITEM_BARNACLE_GRAPPLE,
	CLASS_ITEM_GLOCK,
	CLASS_ITEM_PYTHON,
	CLASS_ITEM_DEAGLE,
	CLASS_ITEM_SHOTGUN,
	CLASS_ITEM_UZI,
	CLASS_ITEM_2UZIS,
	CLASS_ITEM_MP5,
	CLASS_ITEM_M16,
	CLASS_ITEM_CROSSBOW,
	CLASS_ITEM_GAUSS,
	CLASS_ITEM_EGON,
	CLASS_ITEM_RPG,
	CLASS_ITEM_HORNET_GUN,
	CLASS_ITEM_SNIPER_RIFLE,
	CLASS_ITEM_MACHINEGUN,
	CLASS_ITEM_SPORE_LAUNCHER,
	CLASS_ITEM_DISPLACER,
	CLASS_ITEM_MINIGUN,
	CLASS_ITEM_SNARK_NEST,
	CLASS_ITEM_GRENADE,
	CLASS_ITEM_SATCHEL,
	CLASS_ITEM_ARGRENADE,
	CLASS_ITEM_TRIPMINE,
	CLASS_ITEM_WEAPON_BOX,
	CLASS_ITEM_LONGJUMP,
	CLASS_ITEM_HEALTH_CHARGER,
	CLASS_ITEM_HEV_CHARGER,
	CLASS_ITEM_BARNEY_VEST,
	CLASS_ITEM_BARNEY_HELMET,
	CLASS_ITEM_SUIT,
	CLASS_ITEM_SPORE,
	CLASS_ITEM_CROSSBOW_BOLT,
	CLASS_ITEM_RPG_ROCKET,
	CLASS_ITEM_HVR_ROCKET,
	CLASS_ITEM_MORTAR_SHELL,

	// Custom Weapons/Items
	CLASS_ITEM_TOMMY_GUN_AMMO,
	CLASS_ITEM_GREASE_GUN_AMMO,
	CLASS_ITEM_M16_AMMO,
	CLASS_ITEM_M14_AMMO,
	CLASS_ITEM_SHOVEL,
	CLASS_ITEM_SPANNER,
	CLASS_ITEM_DOUBLE_BARREL,
	CLASS_ITEM_TOMMY_GUN,
	CLASS_ITEM_GREASE_GUN,
	CLASS_ITEM_M14,
	CLASS_ITEM_TESLA_GUN,

	CLASS_LAST = CLASS_ITEM_MORTAR_SHELL
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// Access to indexes of hitboxes
#define HITBOX_HEAD 0
#define HITBOX_NECK 1
#define HITBOX_CHEST 2

typedef struct class_info_s
{
	unsigned short flags;
	unsigned short id;

} class_info_t;

typedef struct extra_class_info_s
{
	//extra_class_info_s(const char *pszName, int hitbox_head, int hitbox_neck, int hitbox_chest, std::vector<unsigned char> sequence_dead);

	const char *name;
	std::vector<unsigned char> aimbot_hitboxes; // Head, neck, chest
	std::vector<unsigned char> sequence_dead;

} extra_class_info_t;

class CClassTable
{
public:
	CClassTable();
};

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

inline const char *GetEntityClassname(class_info_t &classInfo)
{
	extern extra_class_info_t g_ExtraClassInfo[];
	return g_ExtraClassInfo[classInfo.id].name;

	//extern const char *g_szClassName[];
	//return g_szClassName[classInfo.id];
}

inline bool IsEntityClassFriend(class_info_t &classInfo)
{
	return classInfo.flags & FL_CLASS_FRIEND;
}

inline bool IsEntityClassEnemy(class_info_t &classInfo)
{
	return classInfo.flags & FL_CLASS_ENEMY;
}

inline bool IsEntityClassNeutral(class_info_t &classInfo)
{
	return classInfo.flags & FL_CLASS_NEUTRAL;
}

inline bool IsEntityClassItem(class_info_t &classInfo)
{
	return classInfo.flags & FL_CLASS_ITEM;
}

inline bool IsEntityClassCorpse(class_info_t &classInfo, int iSolid)
{
	return iSolid == SOLID_BBOX && classInfo.flags & FL_CLASS_CORPSE;
}

inline bool IsEntityClassTrash(class_info_t &classInfo)
{
	return classInfo.flags & FL_CLASS_WORLD_ENTITY;
}

//-----------------------------------------------------------------------------
// Gets entity info from a model
//-----------------------------------------------------------------------------

class_info_t GetEntityClassInfo(const char *pszModelName);
extra_class_info_t &GetExtraEntityClassInfo(eClassID id);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern CClassTable g_AutoFillClassTable;
extern extra_class_info_t g_ExtraClassInfo[];

#endif // CLASS_TABLE_H