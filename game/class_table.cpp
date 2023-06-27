// Table of classes

#include "class_table.h"

#include <data_struct/hashtable.h>
#include <data_struct/hashdict.h>

#define LINK_CLASS_INFO(classID, flags) { static_cast<unsigned short>(flags & 0xFFFF), static_cast<unsigned short>(classID & 0xFFFF) }

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

extra_class_info_t g_ExtraClassInfo[] =
{
	// position of info must match with the class id
	// Name    |    Hitbox index list (head, neck and chest)    |    List of sequences indicating that an entity is dead/inactive (sorted)
	// If vector of hitboxes indexes is empty, then will be used mid point of an entity's hull
	{ "Unknown", {  }, {  } },

	{ "Player", { 11, 10, 8 }, {  } },
	{ "Dead Player", {  }, {  } },

	{ "Respawn Point", {  }, { 0 } },

	// NPCs
	{ "Gordon Freeman", {  }, {  } }, // friend
	{ "Scientist", {  }, {  } }, // friend
	{ "Barney", {  }, {  } }, // friend
	{ "Otis", {  }, { 25, 26, 27, 28, 29, 30 } }, // friend
	{ "Headcrab", { 6, 6, 6 }, { 7 } },
	{ "Baby Headcrab", { 0, 0, 0 }, { 7 } },
	{ "Zombie", { 11, 10, 8 }, { 15, 16, 17, 18, 19, 31 } },
	{ "Zombie Soldier", { 12, 11, 9 }, { 15, 16, 17, 18, 19 } },
	{ "Bullsquid", { 17, 16, 0 }, { 13, 15, 16, 17 } },
	{ "Houndeye", { 9, 8, 6 }, { 6, 7, 8, 9 } },
	{ "Barnacle", {  }, { 6 } }, // no needed hitboxes
	{ "Vortigaunt", { 19, 10, 9 }, { 18, 19, 20, 21 } },
	{ "Human Grunt", { 23, 12, 11 }, { 35, 36, 37, 38, 39, 40, 78, 80 } },
	{ "Sniper", { 16, 7, 6 }, {  } },
	{ "Alient Grunt", { 11, 10, 8 }, { 22, 23, 24, 25, 26 } },
	{ "Tentacle", {  }, {  } }, // no needed hitboxes
	{ "Sentry", { 2, 2, 2 }, { 0, 5 } },
	{ "Turret", { 1, 1, 2 }, { 0, 5 } },
	{ "Leech", {  }, { 3, 6, 7 } }, // no needed hitboxes
	{ "G-Man", {  }, {  } }, // neutral
	{ "Female Assassin", { 13, 12, 11 }, { 10, 11, 12 } },
	{ "Male Assassin", { 14, 7, 7 }, { 32, 33, 34, 35, 36, 37, 50 } },
	{ "Male Sniper Assassin", {  }, {  } }, // enemy
	{ "Agent", { 11, 10, 8 }, { 55 } }, // enemy
	{ "Snark", {  }, {  } }, // no needed hitboxes
	{ "Chumtoad", {  }, {  } }, // no needed hitboxes
	{ "Alien Controller", { 22, 9, 8 }, { 8, 18 } },
	{ "Ichtyosaur", { 15, 3, 2 }, { 3, 4, 7 } },
	{ "Gargantua", { 18, 16, 0 }, { 14 } },
	{ "Baby Gargantua", {  }, { 14 } }, // enemy
	{ "Big Momma", { 12, 12, 12 }, { 4, 19 } },
	{ "Osprey", { 6, 6, 2 }, {  } },
	{ "Black Ops Osprey", { 6, 6, 2 }, {  } },
	{ "Destroyed Osprey", {  }, {  } }, // no needed hitboxes
	{ "Apache", { 2, 2, 0 }, {  } },
	{ "Nihilanth", { 3, 3, 3 }, { 12 } },
	{ "Boid", {  }, {  } }, // neutral
	{ "H.E.V.", {  }, {  } }, // some shit
	{ "Spore Ammo", {  }, {  } }, // no needed hitboxes
	{ "Human Grunt", {  }, {  } }, // friend
	{ "Torch | Human Grunt", {  }, {  } }, // friend
	{ "Medic | Human Grunt", {  }, {  } }, // friend
	{ "Gonome", { 15, 13 /* 14 */, 12}, { 11, 12, 13, 14, 15 } },
	{ "Pit Drone", { 2, 1, 0 }, { 15, 16, 17 } },
	{ "Shock Trooper", { 3, 2, 1 }, { 27, 28, 29, 30, 31, 32 } },
	{ "Voltigore", { 19, 18, 29 }, { 13, 14, 15 } },
	{ "Baby Voltigore", { 17, 24, 14 }, {  } },
	{ "Pit Worm", {  }, {  } }, // no needed hitboxes
	{ "Geneworm", {  }, {  } }, // no needed hitboxes
	{ "Shock Rifle", { 1, 0, 13 }, { 7 } },
	{ "Mortar", {  }, {  } }, // no needed hitboxes
	{ "Stukabat", {  }, { 5, 6, 14 } },
	{ "Kingpin", {  }, { 5 } },
	{ "Xen Commander", {  }, {  } },
	{ "Heavy Grunt", {  }, { 11, 12, 13 } }, // enemy
	{ "Robot Grunt", {  }, { 35, 36, 37, 38, 39, 40, 53 } }, // enemy
	{ "Zombie Bull", {  }, { 35, 36, 37, 38, 39, 40, 53 } },
	{ "The Hand", {  }, {  } },
	{ "Chicken", { 5, 4, 3 }, { 11 } }, // enemy
	{ "Sheriff", { 5, 4, 3 }, { 11 } }, // enemy
	{ "Cyber Franklin", { 20, 22, 10 }, { 11 } }, // enemy
	{ "Manta", {  }, {  } }, // enemy
	{ "Barnabus", { 11, 10, 9 }, { 11 } }, // enemy
	{ "Skeleton", {  }, {  } }, // enemy
	{ "Piranha", {  }, {  } }, // enemy?

	// Wouldn't add them tbh
	{ "Special Forces Grunt", {  }, {  } },
	{ "Barniel", {  }, {  } },
	{ "Archer", {  }, { 9, 10 } },
	{ "Panthereye", {  }, {  } },
	{ "Fiona", {  }, {  } },
	{ "Twitcher", {  }, {  } },
	{ "Spitter", {  }, {  } },
	{ "Handcrab", {  }, {  } },
	{ "Ghost", {  }, {  } },
	{ "Screamer", {  }, {  } },
	{ "Devourer", {  }, {  } },
	{ "Wheelchair", {  }, {  } },
	{ "Face", {  }, {  } },
	{ "Hellhound", {  }, {  } },
	{ "Addiction", {  }, {  } },
	{ "Shark", {  }, {  } },

	// Items
	{ "H.E.V.", {  }, {  } },
	{ "Medkit", {  }, {  } },
	{ "Suit Battery", {  }, {  } },
	{ "Glock Ammo", {  }, {  } },
	{ ".357 Ammo", {  }, {  } },
	{ "Shotgun Ammo", {  }, {  } },
	{ "UZI Ammo", {  }, {  } },
	{ "AR Ammo", {  }, {  } },
	{ "MP5 Ammo", {  }, {  } },
	{ "Chain Ammo", {  }, {  } },
	{ "Crossbow Ammo", {  }, {  } },
	{ "Gauss Ammo", {  }, {  } },
	{ "RPG Ammo", {  }, {  } },
	{ "Sniper Rifle Ammo", {  }, {  } },
	{ "Machine Gun Ammo", {  }, {  } },
	{ "Crowbar", {  }, {  } },
	{ "Wrench", {  }, {  } },
	{ "Knife", {  }, {  } },
	{ "Barnacle Grapple", {  }, {  } },
	{ "Glock", {  }, {  } },
	{ ".357", {  }, {  } },
	{ "Deagle", {  }, {  } },
	{ "Shotgun", {  }, {  } },
	{ "Uzi", {  }, {  } },
	{ "Double Uzis", {  }, {  } },
	{ "9mm AR", {  }, {  } },
	{ "M16", {  }, {  } },
	{ "Crossbow", {  }, {  } },
	{ "Gauss", {  }, {  } },
	{ "Gluon Gun", {  }, {  } },
	{ "RPG", {  }, {  } },
	{ "Hornet Gun", {  }, {  } },
	{ "Sniper Rifle", {  }, {  } },
	{ "Machine Gun", {  }, {  } },
	{ "Spore Launcher", {  }, {  } },
	{ "Displacer", {  }, {  } },
	{ "Minigun", {  }, {  } },
	{ "Snark Nest", {  }, {  } },
	{ "Grenade", {  }, {  } },
	{ "Satchel Charge", {  }, {  } },
	{ "AR Grenade", {  }, {  } },
	{ "Trip Mine", {  }, {  } },
	{ "Weapon Box", {  }, {  } },
	{ "Long Jump", {  }, {  } },
	{ "Health Charger", {  }, {  } },
	{ "H.E.V. Charger", {  }, {  } },
	{ "Barney Vest", {  }, {  } },
	{ "Barney Helmet", {  }, {  } },
	{ "Suit", {  }, {  } },
	{ "Spore", {  }, {  } },
	{ "Crossbow Bolt", {  }, {  } },
	{ "RPG Rocket", {  }, {  } },
	{ "HVR Rocket", {  }, {  } },
	{ "Mortar Shell", {  }, {  } },

	// Custom Weapons/Items
	{ "Tommy Gun Ammo", {  }, {  } },
	{ "Grease Gun Ammo", {  }, {  } },
	{ "M16 Ammo", {  }, {  } },
	{ "M14 Ammo", {  }, {  } },
	{ "Shovel", {  }, {  } },
	{ "Spanner", {  }, {  } },
	{ "Double Barrel", {  }, {  } },
	{ "Tommy Gun", {  }, {  } },
	{ "Grease Gun", {  }, {  } },
	{ "M14", {  }, {  } },
	{ "Tesla Gun", {  }, {  } },
};

CHashTable<uint32_t, class_info_t> g_ClassTable( 1023 );
CHashDict<class_info_t, true, false> g_ModelsTable( 1023 );

CClassTable g_AutoFillClassTable;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

class_info_t GetEntityClassInfo( const char *pszModelName )
{
	auto pClassEntry = g_ClassTable.Find( (uint32_t)pszModelName );

	if ( !pClassEntry )
	{
		const char *pszSlashLastOccur = strrchr( pszModelName, '/' );
		const char *pszModelNameSliced = pszModelName;

		if ( pszSlashLastOccur )
			pszModelNameSliced = pszSlashLastOccur + 1;

		// Result: "model/hlclassic/scientist.mdl" -> "scientist.mdl"

		class_info_t *pClassInfo = g_ModelsTable.Find( pszModelNameSliced );

		if ( pClassInfo == NULL )
		{
			g_ClassTable.Insert( (uint32_t)pszModelName, LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_NEUTRAL ) );
			return { CLASS_NONE, FL_CLASS_NEUTRAL };
		}

		g_ClassTable.Insert( (uint32_t)pszModelName, *pClassInfo );
		return *pClassInfo;
	}

	return *pClassEntry;
}

extra_class_info_t &GetExtraEntityClassInfo( eClassID id )
{
	return g_ExtraClassInfo[ id ];
}

//-----------------------------------------------------------------------------
// Initialize models on DLL load
//-----------------------------------------------------------------------------

CClassTable::CClassTable()
{
	g_ModelsTable.Insert( "lambda.mdl", LINK_CLASS_INFO( CLASS_OBJECT_CP, FL_CLASS_NEUTRAL ) );
	g_ModelsTable.Insert( "umbrella.mdl", LINK_CLASS_INFO( CLASS_OBJECT_CP, FL_CLASS_NEUTRAL ) );

	// NPCs
	g_ModelsTable.Insert( "gordon_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_GORDON_FREEMAN, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "gordon.mdl", LINK_CLASS_INFO( CLASS_NPC_GORDON_FREEMAN, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "scientist2.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "cleansuit_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "scientist_rosenberg.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "wheelchair_sci.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "civ_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "civ_paper_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "console_civ_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "sc2sci.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "scigun.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "civ_coat_scientist.mdl", LINK_CLASS_INFO( CLASS_NPC_SCIENTIST, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "hgruntf.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "sc2grunt.mdl", LINK_CLASS_INFO( CLASS_NPC_ALIEN_GRUNT, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "agruntf.mdl", LINK_CLASS_INFO( CLASS_NPC_ALIEN_GRUNT, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "barney.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNEY, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "intro_barney.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNEY, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "hungerbarney.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNEY, FL_CLASS_ENEMY | FL_CLASS_CORPSE ) ); // TH
	g_ModelsTable.Insert( "pilot.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNEY, FL_CLASS_FRIEND ) ); // TH

	g_ModelsTable.Insert( "headcrab.mdl", LINK_CLASS_INFO( CLASS_NPC_HEADCRAB, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "baby_headcrab.mdl", LINK_CLASS_INFO( CLASS_NPC_BABY_HEADCRAB, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hungercrab.mdl", LINK_CLASS_INFO( CLASS_NPC_HEADCRAB, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "otis.mdl", LINK_CLASS_INFO( CLASS_NPC_OTIS, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "intro_otis.mdl", LINK_CLASS_INFO( CLASS_NPC_OTIS, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "otisf.mdl", LINK_CLASS_INFO( CLASS_NPC_OTIS, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "hungerotis.mdl", LINK_CLASS_INFO( CLASS_NPC_OTIS, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "zombie.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "zombie_soldier.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE_SOLDIER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "zombie_barney.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hungerzombie.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "nursezombie.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "zombie2.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "zombie3.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "nurse.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "lpzombie.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "houndeye.mdl", LINK_CLASS_INFO( CLASS_NPC_HOUNDEYE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hungerhound.mdl", LINK_CLASS_INFO( CLASS_NPC_HOUNDEYE, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "bullsquid.mdl", LINK_CLASS_INFO( CLASS_NPC_BULLSQUID, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "barnacle.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNACLE, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "islave.mdl", LINK_CLASS_INFO( CLASS_NPC_VORTIGAUNT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "islavef.mdl", LINK_CLASS_INFO( CLASS_NPC_VORTIGAUNT, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "sslave.mdl", LINK_CLASS_INFO( CLASS_NPC_VORTIGAUNT, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "hungerslave.mdl", LINK_CLASS_INFO( CLASS_NPC_VORTIGAUNT, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "hgrunt.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT, FL_CLASS_ENEMY | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "zgrunt.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "hgrunt_sniper.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_SNIPER, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "agrunt.mdl", LINK_CLASS_INFO( CLASS_NPC_ALIEN_GRUNT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "zork.mdl", LINK_CLASS_INFO( CLASS_NPC_ALIEN_GRUNT, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "tentacle2.mdl", LINK_CLASS_INFO( CLASS_NPC_TENTACLE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "tentacle3.mdl", LINK_CLASS_INFO( CLASS_NPC_TENTACLE, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "sentry.mdl", LINK_CLASS_INFO( CLASS_NPC_SENTRY, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "turret.mdl", LINK_CLASS_INFO( CLASS_NPC_TURRET, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "miniturret.mdl", LINK_CLASS_INFO( CLASS_NPC_TURRET, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "leech.mdl", LINK_CLASS_INFO( CLASS_NPC_LEECH, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "gman.mdl", LINK_CLASS_INFO( CLASS_NPC_GMAN, FL_CLASS_NEUTRAL ) );

	g_ModelsTable.Insert( "hassassin.mdl", LINK_CLASS_INFO( CLASS_NPC_FEMALE_ASSASSIN, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hassassinf.mdl", LINK_CLASS_INFO( CLASS_NPC_FEMALE_ASSASSIN, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "w_squeak.mdl", LINK_CLASS_INFO( CLASS_NPC_SNARK, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "chubby.mdl", LINK_CLASS_INFO( CLASS_NPC_CHUMTOAD, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "chumtoad.mdl", LINK_CLASS_INFO( CLASS_NPC_CHUMTOAD, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "piranha.mdl", LINK_CLASS_INFO( CLASS_NPC_PIRANHA, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "zombierat.mdl", LINK_CLASS_INFO( CLASS_NPC_SNARK, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "controller.mdl", LINK_CLASS_INFO( CLASS_NPC_ALIEN_CONTROLLER, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "icky.mdl", LINK_CLASS_INFO( CLASS_NPC_ICHTYOSAUR, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "garg.mdl", LINK_CLASS_INFO( CLASS_NPC_GARGANTUA, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "babygarg.mdl", LINK_CLASS_INFO( CLASS_NPC_BABY_GARGANTUA, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "big_mom.mdl", LINK_CLASS_INFO( CLASS_NPC_BIG_MOMMA, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "osprey.mdl", LINK_CLASS_INFO( CLASS_NPC_OSPREY, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "blkop_osprey.mdl", LINK_CLASS_INFO( CLASS_NPC_BLACK_OPS_OSPREY, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "dead_osprey.mdl", LINK_CLASS_INFO( CLASS_NPC_DESTROYED_OSPREY, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "apache.mdl", LINK_CLASS_INFO( CLASS_NPC_APACHE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "boss.mdl", LINK_CLASS_INFO( CLASS_NPC_APACHE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "huey_apache.mdl", LINK_CLASS_INFO( CLASS_NPC_APACHE, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "apache2.mdl", LINK_CLASS_INFO( CLASS_NPC_APACHE, FL_CLASS_FRIEND ) ); // TH
	g_ModelsTable.Insert( "blkop_apache.mdl", LINK_CLASS_INFO( CLASS_NPC_APACHE, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "nihilanth.mdl", LINK_CLASS_INFO( CLASS_NPC_NIHILANTH, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "aflock.mdl", LINK_CLASS_INFO( CLASS_NPC_BOID, FL_CLASS_NEUTRAL ) );

	g_ModelsTable.Insert( "player.mdl", LINK_CLASS_INFO( CLASS_NPC_HEV, FL_CLASS_WORLD_ENTITY ) );

	g_ModelsTable.Insert( "spore_ammo.mdl", LINK_CLASS_INFO( CLASS_NPC_SPORE_AMMO, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "hgrunt_opforf.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "hgrunt_opfor.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "hgrunt_torchf.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR_TORCH, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "hgrunt_torch.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR_TORCH, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "hgrunt_medicf.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR_MEDIC, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );
	g_ModelsTable.Insert( "hgrunt_medic.mdl", LINK_CLASS_INFO( CLASS_NPC_HUMAN_GRUNT_OPFOR_MEDIC, FL_CLASS_FRIEND | FL_CLASS_CORPSE ) );

	g_ModelsTable.Insert( "massn.mdl", LINK_CLASS_INFO( CLASS_NPC_MALE_ASSASSIN, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "massnf.mdl", LINK_CLASS_INFO( CLASS_NPC_MALE_SNIPER_ASSASSIN, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "bgman.mdl", LINK_CLASS_INFO( CLASS_NPC_AGENT, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "gonome.mdl", LINK_CLASS_INFO( CLASS_NPC_GONOME, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hungergonome.mdl", LINK_CLASS_INFO( CLASS_NPC_GONOME, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "pit_drone.mdl", LINK_CLASS_INFO( CLASS_NPC_PIT_DRONE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "strooper.mdl", LINK_CLASS_INFO( CLASS_NPC_SHOCK_TROOPER, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "voltigore.mdl", LINK_CLASS_INFO( CLASS_NPC_VOLTIGORE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "baby_voltigore.mdl", LINK_CLASS_INFO( CLASS_NPC_BABY_VOLTIGORE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "pit_worm_up.mdl", LINK_CLASS_INFO( CLASS_NPC_PIT_WORM, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "geneworm.mdl", LINK_CLASS_INFO( CLASS_NPC_GENEWORM, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "w_shock_rifle.mdl", LINK_CLASS_INFO( CLASS_NPC_SHOCK_RIFLE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "mortar.mdl", LINK_CLASS_INFO( CLASS_NPC_MORTAR, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "stukabat.mdl", LINK_CLASS_INFO( CLASS_NPC_STUKABAT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "kingpin.mdl", LINK_CLASS_INFO( CLASS_NPC_KINGPIN, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "tor.mdl", LINK_CLASS_INFO( CLASS_NPC_TOR, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "torf.mdl", LINK_CLASS_INFO( CLASS_NPC_TOR, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "hwgrunt.mdl", LINK_CLASS_INFO( CLASS_NPC_HEAVY_GRUNT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hwgruntf.mdl", LINK_CLASS_INFO( CLASS_NPC_HEAVY_GRUNT, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "rgrunt.mdl", LINK_CLASS_INFO( CLASS_NPC_ROBOT_GRUNT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "rgruntf.mdl", LINK_CLASS_INFO( CLASS_NPC_ROBOT_GRUNT, FL_CLASS_FRIEND ) );

	g_ModelsTable.Insert( "zombiebull.mdl", LINK_CLASS_INFO( CLASS_NPC_ZOMBIE_BULL, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "thehand.mdl", LINK_CLASS_INFO( CLASS_NPC_THE_HAND, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "chicken.mdl", LINK_CLASS_INFO( CLASS_NPC_CHICKEN, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "sheriff.mdl", LINK_CLASS_INFO( CLASS_NPC_SHERIFF, FL_CLASS_ENEMY ) ); // TH
	g_ModelsTable.Insert( "franklin2.mdl", LINK_CLASS_INFO( CLASS_NPC_CYBERFRANKLIN, FL_CLASS_ENEMY ) ); // TH

	g_ModelsTable.Insert( "flyer.mdl", LINK_CLASS_INFO( CLASS_NPC_MANTA, FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "barnabus.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNABUS, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "skeleton.mdl", LINK_CLASS_INFO( CLASS_NPC_SKELETON, FL_CLASS_ENEMY ) );

	// Wouldn't add them tbh
	g_ModelsTable.Insert( "spforce.mdl", LINK_CLASS_INFO( CLASS_NPC_SPECFOR_GRUNT, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "barniel.mdl", LINK_CLASS_INFO( CLASS_NPC_BARNIEL, FL_CLASS_FRIEND ) );
	g_ModelsTable.Insert( "archer.mdl", LINK_CLASS_INFO( CLASS_NPC_ARCHER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "panther.mdl", LINK_CLASS_INFO( CLASS_NPC_PANTHEREYE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "fiona.mdl", LINK_CLASS_INFO( CLASS_NPC_FIONA, FL_CLASS_NEUTRAL ) );
	g_ModelsTable.Insert( "twitcher.mdl", LINK_CLASS_INFO( CLASS_NPC_TWITCHER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "twitcher2.mdl", LINK_CLASS_INFO( CLASS_NPC_TWITCHER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "twitcher3.mdl", LINK_CLASS_INFO( CLASS_NPC_TWITCHER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "twitcher4.mdl", LINK_CLASS_INFO( CLASS_NPC_TWITCHER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "spitter.mdl", LINK_CLASS_INFO( CLASS_NPC_SPITTER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "handcrab.mdl", LINK_CLASS_INFO( CLASS_NPC_HANDCRAB, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "ghost.mdl", LINK_CLASS_INFO( CLASS_NPC_GHOST, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "screamer.mdl", LINK_CLASS_INFO( CLASS_NPC_SCREAMER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "devourer.mdl", LINK_CLASS_INFO( CLASS_NPC_DEVOURER, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "wheelchair_new.mdl", LINK_CLASS_INFO( CLASS_NPC_WHEELCHAIR, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "face_new.mdl", LINK_CLASS_INFO( CLASS_NPC_FACE, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "hellhound.mdl", LINK_CLASS_INFO( CLASS_NPC_HELLHOUND, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "davidbad_cutscene.mdl", LINK_CLASS_INFO( CLASS_NPC_ADDICTION, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "davidbad_noaxe.mdl", LINK_CLASS_INFO( CLASS_NPC_ADDICTION, FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "shark.mdl", LINK_CLASS_INFO( CLASS_NPC_SHARK, FL_CLASS_ENEMY ) );

	// Items
	g_ModelsTable.Insert( "w_suit.mdl", LINK_CLASS_INFO( CLASS_ITEM_HEV, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_medkit.mdl", LINK_CLASS_INFO( CLASS_ITEM_MEDKIT, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "th_medkit.mdl", LINK_CLASS_INFO( CLASS_ITEM_MEDKIT, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_battery.mdl", LINK_CLASS_INFO( CLASS_ITEM_BATTERY, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_9mmclip.mdl", LINK_CLASS_INFO( CLASS_ITEM_GLOCK_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_1911_mag.mdl", LINK_CLASS_INFO( CLASS_ITEM_GLOCK_AMMO, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_357ammobox.mdl", LINK_CLASS_INFO( CLASS_ITEM_PYTHON_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_hunger357ammobox.mdl", LINK_CLASS_INFO( CLASS_ITEM_PYTHON_AMMO, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_shotbox.mdl", LINK_CLASS_INFO( CLASS_ITEM_SHOTGUN_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_uzi_clip.mdl", LINK_CLASS_INFO( CLASS_ITEM_UZI_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_9mmarclip.mdl", LINK_CLASS_INFO( CLASS_ITEM_MP5_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_mp5_clip.mdl", LINK_CLASS_INFO( CLASS_ITEM_MP5_AMMO2, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_chainammo.mdl", LINK_CLASS_INFO( CLASS_ITEM_CHAIN_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_crossbow_clip.mdl", LINK_CLASS_INFO( CLASS_ITEM_CROSSBOW_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_gaussammo.mdl", LINK_CLASS_INFO( CLASS_ITEM_GAUSS_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_rpgammo.mdl", LINK_CLASS_INFO( CLASS_ITEM_RPG_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m40a1clip.mdl", LINK_CLASS_INFO( CLASS_ITEM_SNIPER_RIFLE_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_saw_clip.mdl", LINK_CLASS_INFO( CLASS_ITEM_MACHINEGUN_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_crowbar.mdl", LINK_CLASS_INFO( CLASS_ITEM_CROWBAR, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_hungercrowbar.mdl", LINK_CLASS_INFO( CLASS_ITEM_CROWBAR, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_pipe_wrench.mdl", LINK_CLASS_INFO( CLASS_ITEM_WRENCH, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_knife.mdl", LINK_CLASS_INFO( CLASS_ITEM_KNIFE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_bgrap.mdl", LINK_CLASS_INFO( CLASS_ITEM_BARNACLE_GRAPPLE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_9mmhandgun.mdl", LINK_CLASS_INFO( CLASS_ITEM_GLOCK, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_1911.mdl", LINK_CLASS_INFO( CLASS_ITEM_GLOCK, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_357.mdl", LINK_CLASS_INFO( CLASS_ITEM_PYTHON, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_hunger357.mdl", LINK_CLASS_INFO( CLASS_ITEM_PYTHON, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_desert_eagle.mdl", LINK_CLASS_INFO( CLASS_ITEM_DEAGLE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_shotgun.mdl", LINK_CLASS_INFO( CLASS_ITEM_SHOTGUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_uzi.mdl", LINK_CLASS_INFO( CLASS_ITEM_UZI, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_2uzis.mdl", LINK_CLASS_INFO( CLASS_ITEM_2UZIS, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_9mmAR.mdl", LINK_CLASS_INFO( CLASS_ITEM_MP5, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m16.mdl", LINK_CLASS_INFO( CLASS_ITEM_M16, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_crossbow.mdl", LINK_CLASS_INFO( CLASS_ITEM_CROSSBOW, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_gauss.mdl", LINK_CLASS_INFO( CLASS_ITEM_GAUSS, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_egon.mdl", LINK_CLASS_INFO( CLASS_ITEM_EGON, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_rpg.mdl", LINK_CLASS_INFO( CLASS_ITEM_RPG, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_hgun.mdl", LINK_CLASS_INFO( CLASS_ITEM_HORNET_GUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m40a1.mdl", LINK_CLASS_INFO( CLASS_ITEM_SNIPER_RIFLE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_saw.mdl", LINK_CLASS_INFO( CLASS_ITEM_MACHINEGUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_spore_launcher.mdl", LINK_CLASS_INFO( CLASS_ITEM_SPORE_LAUNCHER, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_displacer.mdl", LINK_CLASS_INFO( CLASS_ITEM_DISPLACER, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_minigun.mdl", LINK_CLASS_INFO( CLASS_ITEM_MINIGUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_sqknest.mdl", LINK_CLASS_INFO( CLASS_ITEM_SNARK_NEST, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_grenade.mdl", LINK_CLASS_INFO( CLASS_ITEM_GRENADE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_hungergrenade.mdl", LINK_CLASS_INFO( CLASS_ITEM_GRENADE, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_tnt.mdl", LINK_CLASS_INFO( CLASS_ITEM_GRENADE, FL_CLASS_ITEM ) ); // TH
	g_ModelsTable.Insert( "w_satchel.mdl", LINK_CLASS_INFO( CLASS_ITEM_SATCHEL, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_argrenade.mdl", LINK_CLASS_INFO( CLASS_ITEM_ARGRENADE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_tripmine.mdl", LINK_CLASS_INFO( CLASS_ITEM_TRIPMINE, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_weaponbox.mdl", LINK_CLASS_INFO( CLASS_ITEM_WEAPON_BOX, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_longjump.mdl", LINK_CLASS_INFO( CLASS_ITEM_LONGJUMP, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "health_charger_body.mdl", LINK_CLASS_INFO( CLASS_ITEM_HEALTH_CHARGER, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "hev_glass.mdl", LINK_CLASS_INFO( CLASS_ITEM_HEV_CHARGER, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "barney_vest.mdl", LINK_CLASS_INFO( CLASS_ITEM_BARNEY_VEST, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "barney_helmet.mdl", LINK_CLASS_INFO( CLASS_ITEM_BARNEY_HELMET, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "suit2.mdl", LINK_CLASS_INFO( CLASS_ITEM_SUIT, FL_CLASS_ITEM ) );

	g_ModelsTable.Insert( "spore.mdl", LINK_CLASS_INFO( CLASS_ITEM_SPORE, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "grenade.mdl", LINK_CLASS_INFO( CLASS_ITEM_GRENADE, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "crossbow_bolt.mdl", LINK_CLASS_INFO( CLASS_ITEM_CROSSBOW_BOLT, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "rpgrocket.mdl", LINK_CLASS_INFO( CLASS_ITEM_RPG_ROCKET, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "HVR.mdl", LINK_CLASS_INFO( CLASS_ITEM_HVR_ROCKET, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );
	g_ModelsTable.Insert( "mortarshell.mdl", LINK_CLASS_INFO( CLASS_ITEM_MORTAR_SHELL, FL_CLASS_ITEM | FL_CLASS_ENEMY ) );

	g_ModelsTable.Insert( "w_tommygun_mag.mdl", LINK_CLASS_INFO( CLASS_ITEM_TOMMY_GUN_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_greasegun_mag.mdl", LINK_CLASS_INFO( CLASS_ITEM_GREASE_GUN_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m16_mag.mdl", LINK_CLASS_INFO( CLASS_ITEM_M16_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m14_mag.mdl", LINK_CLASS_INFO( CLASS_ITEM_M14_AMMO, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_shovel.mdl", LINK_CLASS_INFO( CLASS_ITEM_SHOVEL, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_spanner.mdl", LINK_CLASS_INFO( CLASS_ITEM_SPANNER, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_dbarrel.mdl", LINK_CLASS_INFO( CLASS_ITEM_DOUBLE_BARREL, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_tommygun.mdl", LINK_CLASS_INFO( CLASS_ITEM_TOMMY_GUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_greasegun.mdl", LINK_CLASS_INFO( CLASS_ITEM_GREASE_GUN, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_m14.mdl", LINK_CLASS_INFO( CLASS_ITEM_M14, FL_CLASS_ITEM ) );
	g_ModelsTable.Insert( "w_tesla.mdl", LINK_CLASS_INFO( CLASS_ITEM_TESLA_GUN, FL_CLASS_ITEM ) );

	// World entites that have at least one hitbox
	g_ModelsTable.Insert( "bigrat.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "w_syringebox.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "dead_islave.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "headless_zombie.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "dissected_headcrab.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tank_base.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "DecayCrystals.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "broken_tube_glass.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bs_experiment.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bs_glasstube.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "hornet.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tree.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "protozoa.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fungus.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fungus(small).mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fungus(large).mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "hair.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "rengine.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "sat_globe.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "w_pmedkit.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "roach.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "forklift.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "loader.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "construction.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "dead_barney.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "dead_scientist.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "ball.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "can.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "w_crossbow_clip.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "pit_drone_spike.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "crashed_osprey.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "baby_strooper.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "filecabinet.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "light.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tool_box.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "nuke_case.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "arc_xer_tree1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "chair.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tool_box_sm.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "w_flashlight.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "rip.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "base.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "base_flag.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "shell.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "stretcher.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "holo.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) ); // holo-.. Horo?
	g_ModelsTable.Insert( "egg.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "plant_01.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "health_charger_both.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "EYE_SCANNER.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "obj_chair.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "obj_pipe1_straight.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "pipe_1_straight.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "arc_xer_tree2.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "arc_bush.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "arc_flower.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "arc_fern.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "ouitz_tree1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "alec_tree1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bush1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bush2.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fern1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fern2.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "uplant1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "mbarrel.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tree1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "tree2.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "shrub1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "cross.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "grave.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "grave1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "grave2.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "grave3.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "grave4.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );

	// fucking gibs
	g_ModelsTable.Insert( "tech_crategibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "vgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "pit_drone_gibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "med_crategibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "mil_crategibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "strooper_gibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "concrete_gibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "chromegibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "catwalkgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "garbagegibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "metalplategibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "rockgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "office_gibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "hgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "webgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "osprey_bodygibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "woodgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "fleshgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "glassgibsw.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "metalplategibs_dark.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "metalgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bleachbones.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "bonegibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "glassgibs.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "gib_lung.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "gib_skull.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "gib_b_bone.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "ribcage.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
	g_ModelsTable.Insert( "riblet1.mdl", LINK_CLASS_INFO( CLASS_NONE, FL_CLASS_WORLD_ENTITY ) );
}