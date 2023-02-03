// Skybox

#include <ISvenModAPI.h>
#include <IMemoryUtils.h>
#include <IEngineClient.h>

#include <convar.h>
#include <dbg.h>

#include <hl_sdk/pm_shared/pm_defs.h>

#include "skybox.h"

#include "../patterns.h"

#include "../config.h"
#include "../game/utils.h"

//-----------------------------------------------------------------------------
// Declare Hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(int, __cdecl, R_LoadSkyBoxInt, const char *);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CSkybox g_Skybox;

bool g_bMenuChangeSkybox = false;

static bool s_bLoadingSkybox = false;
static bool s_bSkyboxLoaded = false;

const char *g_szSkyboxes[] =
{
	"-",
	"desert",
	"2desert",
	"desnoon",
	"morning",
	"cliff",
	"dfcliff",
	"dustbowl",
	"sandstone",
	"sky_blu_",
	"sky16",
	"sky35",
	"sky45",
	"tornsky",
	"twildes",
	"crashsite",
	"doom1",
	"dusk",
	"fodrian",
	"night",
	"carnival",
	"theyh2",
	"theyh3",
	"thn",
	"forest512_",
	"tetris",
	"2vs",
	"ac_",
	"arcn",
	"black",
	"coliseum",
	"gmcity",
	"grassy",
	"toon",
	"parallax-errorlf256_",
	"necros-hell256_",
	"space",
	"hplanet",
	"vreality_sky",
	"neb1",
	"neb2b",
	"neb6",
	"neb7",
	"alien1",
	"alien2",
	"alien3",
	"xen8",
	"xen9",
	"xen10"
};

int g_iSkyboxesSize = int(sizeof(g_szSkyboxes) / sizeof(*g_szSkyboxes));

//-----------------------------------------------------------------------------
// ConCommands, CVars..
//-----------------------------------------------------------------------------

CON_COMMAND_EXTERN(sc_change_skybox, ConCommand_ChangeSkybox, "Change skybox by given name")
{
	if (g_bMenuChangeSkybox || args.ArgC() > 1)
	{
		if (g_bMenuChangeSkybox && g_Config.cvars.skybox == 0)
		{
			Msg("Choose skybox name first\n");
			return;
		}

		const char *pszSkyboxName = g_bMenuChangeSkybox ? g_szSkyboxes[g_Config.cvars.skybox] : args[1];

		if (*pszSkyboxName)
			g_Skybox.Replace(pszSkyboxName);
	}
	else
	{
		ConMsg("Usage:  sc_change_skybox <skybox name>\n");
	}
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_reset_skybox, ConCommand_ResetSkybox, "Reset skybox to the default")
{
	if (g_Config.cvars.skybox != 0)
		g_Config.cvars.skybox = 0;

	g_Skybox.Reset();
}

CON_COMMAND(sc_change_skybox_color, "hange skybox's color (RGB: color range from 0 to 1)")
{
	if (args.ArgC() > 3)
	{
		float r = strtof(args[1], NULL);
		float g = strtof(args[2], NULL);
		float b = strtof(args[3], NULL);

		g_pEngineClient->ChangeSkycolor(r, g, b);
	}
	else
	{
		ConMsg("Usage:  sc_change_skybox_color <red> <green> <blue>\n");
	}
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(int, __cdecl, HOOKED_R_LoadSkyBoxInt, const char *pszSkyboxName)
{
	int loaded = ORIG_R_LoadSkyBoxInt(pszSkyboxName);

	if (s_bLoadingSkybox)
	{
		s_bSkyboxLoaded = loaded;
	}
	else
	{
		g_Skybox.SaveOriginalSkybox(pszSkyboxName);
	}

	return loaded;
}

//-----------------------------------------------------------------------------
// Think function
//-----------------------------------------------------------------------------

void CSkybox::Think()
{
	if (m_bSkyboxReplaced && g_pPlayerMove->movevars)
	{
		if (m_flNextThinkTime > g_pEngineFuncs->GetClientTime())
			return;
		else
			m_flNextThinkTime = g_pEngineFuncs->GetClientTime() + 0.25f;

		if ( strcmp( m_szSkyboxName, m_szCurrentSkyboxName ) )
		{
			s_bLoadingSkybox = true;

			g_pEngineClient->ChangeSkymap(m_szSkyboxName);

			s_bLoadingSkybox = false;

			if (!s_bSkyboxLoaded)
			{
				m_bSkyboxReplaced = false;

				*m_szSkyboxName = 0;
				*m_szCurrentSkyboxName = 0;
				*m_szOriginalSkyboxName = 0;
			}
			else
			{
				strcpy_s(m_szOriginalSkyboxName, sizeof(movevars_s::skyName), g_pPlayerMove->movevars->skyName);
				strcpy_s(m_szCurrentSkyboxName, sizeof(m_szCurrentSkyboxName), m_szSkyboxName);
			}
		}
	}
}

void CSkybox::Replace(const char *pszSkyboxName)
{
	strcpy_s(m_szSkyboxName, sizeof(m_szSkyboxName), pszSkyboxName);
	
	*m_szCurrentSkyboxName = 0;
	m_bSkyboxReplaced = true;
}

void CSkybox::Reset()
{
	if (m_bSkyboxReplaced && g_pPlayerMove->movevars && *m_szOriginalSkyboxName)
	{
		g_pEngineClient->ChangeSkymap(m_szOriginalSkyboxName);
	}

	*m_szSkyboxName = 0;
	*m_szOriginalSkyboxName = 0;
	*m_szCurrentSkyboxName = 0;

	m_bSkyboxReplaced = false;
}

void CSkybox::SaveOriginalSkybox(const char *pszSkyboxName)
{
	strcpy_s(m_szOriginalSkyboxName, sizeof(m_szOriginalSkyboxName), pszSkyboxName);
	strcpy_s(m_szCurrentSkyboxName, sizeof(m_szCurrentSkyboxName), pszSkyboxName);
}

void CSkybox::OnVideoInit()
{
	m_flNextThinkTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------

void CSkybox::OnConfigLoad()
{
	if (g_Config.cvars.skybox > 0 && g_Config.cvars.skybox < g_iSkyboxesSize)
	{
		Replace(g_szSkyboxes[g_Config.cvars.skybox]);
	}
}

CSkybox::CSkybox()
{
	m_hR_LoadSkyboxInt = 0;

	m_bSkyboxReplaced = false;
	m_flNextThinkTime = -1.f;
}

bool CSkybox::Load()
{
	m_pfnR_LoadSkyboxInt = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::R_LoadSkyboxInt );

	if ( !m_pfnR_LoadSkyboxInt )
	{
		Warning("Couldn't find function R_LoadSkyboxInt\n");
		return false;
	}

	return true;
}

void CSkybox::PostLoad()
{
	m_hR_LoadSkyboxInt = DetoursAPI()->DetourFunction( m_pfnR_LoadSkyboxInt, HOOKED_R_LoadSkyBoxInt, GET_FUNC_PTR(ORIG_R_LoadSkyBoxInt) );
}

void CSkybox::Unload()
{
	DetoursAPI()->RemoveDetour( m_hR_LoadSkyboxInt );
}