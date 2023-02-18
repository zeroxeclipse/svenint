#include <vector>
#include <algorithm>

#include <sys.h>
#include <dbg.h>
#include <convar.h>
#include <ISvenModAPI.h>

#include "opengl.h"
#include "menu.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"

#include "../imgui_custom/imgui_custom.h"

#include "../features/models_manager.h"
#include "../features/thirdperson.h"

#include "../utils/xorstr.h"
#include "../utils/menu_styles.h"
#include "../utils/menu_fonts.hpp"
#include "../config.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define OBFUSCATE_STRINGS

#ifdef OBFUSCATE_STRINGS

#include "../utils/xorstr.h"

#else

#define xs(_str) (_str)

#endif

extern uint64 g_ullSteam64ID;
extern std::vector<uint64> g_Gods;

//-----------------------------------------------------------------------------
// Signatures
//-----------------------------------------------------------------------------

DECLARE_HOOK(BOOL, APIENTRY, wglSwapBuffers, HDC);
DECLARE_HOOK(BOOL, WINAPI, SetCursorPos, int, int);

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

static CCommand s_DummyCommand;

static HWND hGameWnd = NULL;
static WNDPROC hGameWndProc = NULL;

CMenuModule g_MenuModule;

int g_iMenuState = 0;

bool g_bMenuEnabled = false;
bool g_bMenuClosed = true;

float g_bMenuOpenTime = -1.f;
float g_bMenuCloseTime = -1.f;
float g_bMenuOpenTimePrev = -1.f;
float g_bMenuCloseTimePrev = -1.f;

//ImFont *g_pImFont = NULL;

ImGuiStyle* style;
CImGuiCustom ImGuiCustom;

ImFont* cool_font = NULL;
ImFont* cool_font_big = NULL;
ImFont* cool_font_small = NULL;

int sven_int_width = 0;
int sven_int_height = 0;
GLuint sven_int_logo = 0;

int menu_image_width = 0;
int menu_image_height = 0;
GLuint menu_image = 0;

bool m_Image = true;
float Red = 0.0f, Green = 0.01f, Blue = 0.0f;

int selectedTab = 0, selectedSubTab0 = 0, selectedSubTab1 = 0, selectedSubTab2 = 0, selectedSubTab3 = 0, selectedSubTab4 = 0;

// Tabs Strings Vars

std::string VisualsSubTabs[] = { "Render", "ESP", "Chams", "Glow", "Flashlight", "Wallhack", "BSP", "Models Manager", "Shaders", "Misc" };

std::string HUDSubTabs[] = { "General", "Speedometer", "Radar", "Chat Colors", "Custom Vote Popup" };

std::string UtilitySubTabs[] = { "Player", "Color Pulsator", "Fake Lag", "Anti-AFK", "Spammer", "Speedrun Tools", "Misc" };

std::string ConfigsSubTab[] = { "SvenInt" };

std::string SettingsSubTab[] = { "Menu", "Game" };

// Features Strings Vars

static const char* no_weap_anim_items[] = { "0 - Off", "1 - All Animations", "2 - Take Animations" };

static const char* draw_entities_items[] = { "0 - Default", "1 - Draw Bones", "2 - Draw Hitboxes", "3 - Draw Model & Hitboxes", "4 - Draw Hulls", "5 - Draw Players Bones", "6 - Draw Players Hitboxes" };

static const char* esp_style[] = { "0 - Default", "1 - SAMP", "2 - Left 4 Dead" };
static const char* esp_process_items[] = { "0 - Everyone", "1 - Entities", "2 - Players" };
static const char* esp_box_items[] = { "0 - Off", "1 - Default", "2 - Coal", "3 - Corner" };

static const char* chams_items[] = { "0 - Disable", "1 - Flat", "2 - Texture", "3 - Material" };

static const char* glow_items[] = { "0 - Disable", "1 - Glow Outline", "2 - Glow Shell", "3 - Ghost" };

static const char* ca_types[] = { "0 - Default", "1 - Barrel Distortion", "2 - Linear Barrel Distortion" };

float v = g_Config.cvars.shaders_chromatic_aberration_shift;

static const char* dof_interps[] = { "0 - Linear", "1 - Simple Spline", "2 - Parabolic", "3 - Parabolic Inverted", "4 - Cubic" };

static const char* strafe_dir_items[] = { "0 - To the left", "1 - To the right", "2 - Best strafe", "3 - View angles" };
static const char* strafe_type_items[] = { "0 - Max. acceleration", "1 - Max. angle", "2 - Max. deceleration", "3 - Const speed" };

static const char* fakelag_type_items[] = { "0 - Dynamic", "1 - Maximum", "2 - Jitter", "3 - Break Lag Compensation" };
static const char* fakelag_move_items[] = { "0 - Everytime", "1 - On Land", "2 - On Move", "3 - In Air" };

static const char* antiafk_items[] =
{
	"0 - Off", "1 - Step Forward & Back", "2 - Spam Gibme",
	"3 - Spam Kill", "4 - Walk Around & Spam Inputs",
	"5 - Walk Around", "6 - Go Right"
};

static const char* trace_type[] =
{
	"0 - Trace Line", "1 - Trace Hull"
};

static const char* radar_type[] = { "0 - Round", "1 - Square" };

static const char* theme_items[] =
{
	"SvenInt",
	"Dark",
	"Light",
	"Classic",
	"Berserk",
	"Deep Dark",
	"Carbon",
	"Corporate Grey",
	"Grey",
	"Dark Light",
	"Soft Dark",
	"Gold & Black",
	"Monochrome",
	"Pink",
	"Half-Life",
	"Sven-Cope",
};

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

static void RainbowCycle()
{
	int isFrames = g_pEngineFuncs->Sys_FloatTime();

	ImVec4 isRGB = ImVec4(Red, Green, Blue, 1.0f);

	if (isFrames % g_Config.cvars.rainbow_speed == 1)
	{

		if (Green == 0.01f && Blue == 0.0f)
		{
			Red += 0.01f;

		}

		if (Red > 0.99f && Blue == 0.0f)
		{
			Red = 1.0f;

			Green += 0.01f;

		}

		if (Green > 0.99f && Blue == 0.0f)
		{
			Green = 1.0f;

			Red -= 0.01f;

		}

		if (Red < 0.01f && Green == 1.0f)
		{
			Red = 0.0f;

			Blue += 0.01f;

		}

		if (Blue > 0.99f && Red == 0.0f)
		{
			Blue = 1.0f;

			Green -= 0.01f;

		}

		if (Green < 0.01f && Blue == 1.0f)
		{
			Green = 0.0f;

			Red += 0.01f;

		}

		if (Red > 0.99f && Green == 0.0f)
		{
			Red = 1.0f;

			Blue -= 0.01f;

		}

		if (Blue < 0.01f && Green == 0.0f)
		{
			Blue = 0.0f;

			Red += 0.01f;

			if (Red < 0.01f)
				Green = 0.01f;
		}
	}
}

// Simple helper function to load an image into a OpenGL texture with common settings
static bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}

static void LoadMenuImage()
{
	std::string bPath = SvenModAPI()->GetBaseDirectory();

	std::string sint_image = "\\sven_internal\\images\\menu_image.png";
	std::string sint_image_fPath = bPath + sint_image;

	bool sven_image = LoadTextureFromFile(sint_image_fPath.c_str(), &menu_image, &menu_image_width, &menu_image_height);

	if ((!sven_image) || !(menu_image_width == 130 && menu_image_height == 248))
	{
		Warning("[SvenInt] Cannot load SvenInt menu image, make sure size is 130x248\n");
		m_Image = false;
	}

}

static void LoadFontAndTextures()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Unless we make a config for the same font
	// with different size the icons will not work wtf (?)
	ImFontConfig CoolFontBigCfg;
	CoolFontBigCfg.FontDataOwnedByAtlas = false; // if this is set to true it will try to free memory and crash 
	cool_font_big = io.Fonts->AddFontFromMemoryTTF((void*)CoolFont, sizeof(CoolFont), 24, &CoolFontBigCfg);

	ImFontConfig CoolFontSmallCfg;
	CoolFontSmallCfg.FontDataOwnedByAtlas = false;
	cool_font_small = io.Fonts->AddFontFromMemoryTTF((void*)CoolFont, sizeof(CoolFont), 17, &CoolFontSmallCfg);

	ImFontConfig CoolFontCfg;
	CoolFontCfg.FontDataOwnedByAtlas = false;
	cool_font = io.Fonts->AddFontFromMemoryTTF((void*)CoolFont, sizeof(CoolFont), 18, &CoolFontCfg);

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.FontDataOwnedByAtlas = false;
	io.Fonts->AddFontFromMemoryTTF((void*)fontAwesome, sizeof(fontAwesome), 18, &icons_config, icons_ranges);

	// This is a mess... but im too stupid to figure out another way
	std::string bPath = SvenModAPI()->GetBaseDirectory();

	std::string sint_logo = "\\sven_internal\\images\\logo.png";
	std::string sint_logo_fPath = bPath + sint_logo;

	bool sven_logo = LoadTextureFromFile(sint_logo_fPath.c_str(), &sven_int_logo, &sven_int_width, &sven_int_height);

	if (!sven_logo)
	{
		Warning("[SvenInt] Failed to load ImGui logo\n");
	}

	LoadMenuImage();
}

// Restores window style
void WindowStyle()
{
	style->WindowRounding = 5;
	style->ChildRounding = 8;
	style->FrameRounding = 5;
	style->GrabRounding = 5;
	style->PopupRounding = 5;
	style->FrameRounding = 5;
}

// This is needed to align icons and text on the buttons 
// due to different icons taking up different space
// yes it's dumb thing from dumb person 
static float ForIcon(int i)
{
	switch (i)
	{
	case 0:
		return 0.1;
	case 1:
		return 0.08;
	case 2:
		return 0.09;
	case 3:
		return 0.115;
	case 4:
		return 0.09;
	default:
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Menu
//-----------------------------------------------------------------------------

void CMenuModule::Draw()
{
	if ( !m_bThemeLoaded )
	{
		LoadMenuTheme();
		WindowStyle();

		m_bThemeLoaded = true;
	}

	ImGui::GetIO().MouseDrawCursor = g_bMenuEnabled;
	ImGui::GetStyle().Alpha = g_Config.cvars.menu_opacity;

	if (g_bMenuEnabled)
	{
		// Main Window

		ImGui::SetNextWindowSize({ 800, 600 });
		ImGui::Begin("Main", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

		if (g_Config.cvars.rainbow[0] || g_Config.cvars.rainbow[1])
		{
			RainbowCycle();
		}

		ImGui::SetCursorPosY(0);

		ImGuiCustom.Columns(2, nullptr, true, g_Config.cvars.rainbow[1] ? ImVec4(Red, Green, Blue, 150) : ImGui::GetStyleColorVec4(ImGuiCol_Separator));
		ImGui::SetColumnOffset(1, 130);

		// Left Side Column

		DrawLogo();

		DrawMainTabs(); // (Visuals, HUD, Utility, Configs, Settings)

		DrawMenuImage();

		DrawStats(); // Frames

		// Next Coloumn
	
		ImGui::NextColumn();

		ImGui::SetCursorPosX(134);
		ImGui::SetCursorPosY(6);

		// Sub Tabs Wrapper

		ImGui::BeginChild("subtabs-wrapper", ImVec2(880, 39), true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		{
			ImGui::SetCursorPosY(8);

			ImGui::PushFont(cool_font_small);

			switch (selectedTab)
			{
			case 0:
			{
				DrawVisualsSubTabs();
				break;
			}
			case 1:
			{
				DrawHUDSubTabs();
				break;
			}
			case 2:
			{
				DrawUtilitySubTabs();
				break;
			}
			case 3:
			{
				DrawConfigsSubTabs();
				break;
			}
			case 4:
			{
				DrawSettingsSubTabs();
				break;
			}
			}
		}

		ImGui::EndChild();

		ImGui::Spacing();

		ImGui::SetCursorPosX(134);
		ImGui::SetCursorPosY(50);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
		ImGui::BeginChild("content-wrapper", ImVec2(ImGui::GetContentRegionAvail().x + 7, ImGui::GetContentRegionAvail().y + 5), false);
		ImGui::PopStyleColor();

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnOffset(1, 328.5);

		ImGui::SetCursorPosX(0);

		switch (selectedTab)
		{
		case 0:
		{
			DrawVisualsTabContent();
			break;
		}
		case 1:
		{
			DrawHUDTabContent();
			break;
		}
		case 2:
		{
			DrawUtilityTabContent();
			break;
		}
		case 3:
		{
			DrawTabConfigsContent();
			break;
		}
		case 4:
		{
			DrawSettingsTabContent();
			break;
		}
		}
		ImGui::EndChild();
	}
	ImGuiCustom.End();
}


// Left Column 

void CMenuModule::DrawLogo()
{
	ImGui::SetCursorPosY(10);
	ImGui::SetCursorPosX(9);

	ImGui::Image((void*)(intptr_t)sven_int_logo, ImVec2(sven_int_width, sven_int_height), ImVec2(0, 0), ImVec2(1, 1), g_Config.cvars.rainbow[0] ? ImVec4(Red, Green, Blue, 255) : ImVec4(g_Config.cvars.logo_color[0], g_Config.cvars.logo_color[1], g_Config.cvars.logo_color[2], 255));

	ImGui::SameLine();

	ImGui::SetCursorPosY(12);
	ImGui::SetCursorPosX(50);

	ImGui::PushFont(cool_font_big);

	ImGui::TextUnformatted("SvenInt");

	ImGui::PopFont();

	ImGui::Spacing();
}

void CMenuModule::DrawMainTabs()
{

	ImGui::SetCursorPosX(4);

	ImGui::BeginChild("tabs", ImVec2(123, 232), true);

	std::string TabNames[] = { ICON_FA_EYE "  Visuals", ICON_FA_CROSSHAIRS "  HUD", ICON_FA_GLOBE "  Utility", ICON_FA_SAVE "  Configs", ICON_FA_COG "  Settings" };

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);

	ImGui::PushFont(cool_font);

	for (int i = 0; i < ARRAYSIZE(TabNames); i++)
	{
		std::string it = TabNames[i];
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(ForIcon(i), 0.5));
		ImGui::PushStyleColor(ImGuiCol_Button, selectedTab == i ? style->Colors[ImGuiCol_ButtonActive] : ImVec4(0, 0, 0, 0));
		ImGui::SetCursorPosX(7);
		if (ImGui::Button(it.c_str(), ImVec2(140, 40))) selectedTab = i;
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(2);
	}

	ImGui::PopStyleVar();
	ImGui::PopFont();

	ImGui::EndChild();
}

void CMenuModule::DrawMenuImage()
{
	if (m_Image)
	{
		ImGui::SetCursorPosX(0);
		ImGui::Image((void*)(intptr_t)menu_image, ImVec2(menu_image_width, menu_image_height), ImVec2(0, 0), ImVec2(1, 1));
	}
	else
	{
		ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 55 - style->ItemSpacing.y));
	}
}

void CMenuModule::DrawStats()
{
	ImGui::SetCursorPosX(4);
	ImGui::BeginChild("Frames", ImVec2(123, 58), true);

	ImGui::PushFont(cool_font);

	ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

	ImGui::PopFont();

	ImGui::EndChild();
}


// Sub Tabs 

void CMenuModule::DrawVisualsSubTabs()
{
	for (int i = 0; i < ARRAYSIZE(VisualsSubTabs); i++)
	{
		std::string it = VisualsSubTabs[i];

		ImGui::PushStyleColor(ImGuiCol_Button, selectedSubTab0 == i ? style->Colors[ImGuiCol_ButtonActive] : style->Colors[ImGuiCol_Button]);
		if (ImGui::Button(it.c_str(), ImVec2(ImGui::CalcTextSize(it.c_str()).x + 10, 25)))
		{
			selectedSubTab0 = i;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}
}

void CMenuModule::DrawHUDSubTabs()
{
	for (int i = 0; i < ARRAYSIZE(HUDSubTabs); i++)
	{
		std::string it = HUDSubTabs[i];

		ImGui::PushStyleColor(ImGuiCol_Button, selectedSubTab1 == i ? style->Colors[ImGuiCol_ButtonActive] : style->Colors[ImGuiCol_Button]);
		if (ImGui::Button(it.c_str(), ImVec2(ImGui::CalcTextSize(it.c_str()).x + 10, 25)))
		{
			selectedSubTab1 = i;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}
}

void CMenuModule::DrawUtilitySubTabs()
{
	for (int i = 0; i < ARRAYSIZE(UtilitySubTabs); i++)
	{
		std::string it = UtilitySubTabs[i];

		ImGui::PushStyleColor(ImGuiCol_Button, selectedSubTab2 == i ? style->Colors[ImGuiCol_ButtonActive] : style->Colors[ImGuiCol_Button]);
		if (ImGui::Button(it.c_str(), ImVec2(ImGui::CalcTextSize(it.c_str()).x + 10, 25)))
		{
			selectedSubTab2 = i;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}
}

void CMenuModule::DrawConfigsSubTabs()
{
	for (int i = 0; i < ARRAYSIZE(ConfigsSubTab); i++)
	{
		std::string it = ConfigsSubTab[i];

		ImGui::PushStyleColor(ImGuiCol_Button, selectedSubTab3 == i ? style->Colors[ImGuiCol_ButtonActive] : style->Colors[ImGuiCol_Button]);
		if (ImGui::Button(it.c_str(), ImVec2(ImGui::CalcTextSize(it.c_str()).x + 10, 25)))
		{
			selectedSubTab3 = i;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}
}

void CMenuModule::DrawSettingsSubTabs()
{
	for (int i = 0; i < ARRAYSIZE(SettingsSubTab); i++)
	{
		std::string it = SettingsSubTab[i];

		ImGui::PushStyleColor(ImGuiCol_Button, selectedSubTab4 == i ? style->Colors[ImGuiCol_ButtonActive] : style->Colors[ImGuiCol_Button]);
		if (ImGui::Button(it.c_str(), ImVec2(ImGui::CalcTextSize(it.c_str()).x + 10, 25)))
		{
			selectedSubTab4 = i;
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}
}


// Sub Tabs Content

void CMenuModule::DrawVisualsTabContent()
{
	switch (selectedSubTab0)
	{
	case 0: // Render
	{
		ImGui::BeginChild(xs("render"), ImVec2(328, 430), true);

		ImGui::PushItemWidth(180);

		ImGui::Text(xs("Game"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("No Shake"), &g_Config.cvars.no_shake); ImGui::SameLine();
		ImGui::Checkbox(xs("No Fade"), &g_Config.cvars.no_fade); ImGui::SameLine();
		ImGui::Checkbox(xs("Remove FOV Cap"), &g_Config.cvars.remove_fov_cap);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Draw Entities"));

		ImGuiCustom.Spacing(4);

		ImGui::Combo(" ", &g_Config.cvars.draw_entities, draw_entities_items, IM_ARRAYSIZE(draw_entities_items));

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Light Map"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Override Lightmap"), &g_Config.cvars.lightmap_override);

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Lightmap Brightness"), &g_Config.cvars.lightmap_brightness, 0.0f, 1.0f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Lightmap Color"), g_Config.cvars.lightmap_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("No Weapon Animations"));

		ImGuiCustom.Spacing(4);

		ImGui::Combo(xs("##no_weap_anims"), &g_Config.cvars.no_weapon_anim, no_weap_anim_items, IM_ARRAYSIZE(no_weap_anim_items));

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Skip Frames"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Skip Frames"), &g_Config.cvars.skip_frames);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Skip Frames Count"), &g_Config.cvars.skip_frames_count, 1, 60);

		ImGui::EndTabItem();

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::PopItemWidth();
		break;
	}
	case 1: // ESP
	{
		ImGui::BeginChild(xs("esp"), ImVec2(328, 490), true);

		ImGui::PushItemWidth(180);

		ImGui::Checkbox(xs("Enable ESP"), &g_Config.cvars.esp); ImGui::SameLine();
		ImGui::Checkbox(xs("Debug##esp"), &g_Config.cvars.esp_debug);
		ImGui::Checkbox(xs("Optimize##esp"), &g_Config.cvars.esp_optimize); ImGui::SameLine();
		ImGui::Checkbox(xs("Snap Lines##esp"), &g_Config.cvars.esp_snaplines); ImGui::SameLine();
		ImGui::Checkbox(xs("Outline Box"), &g_Config.cvars.esp_box_outline);
		ImGui::Checkbox(xs("Show Items"), &g_Config.cvars.esp_show_items); ImGui::SameLine();
		ImGui::Checkbox(xs("Ignore Unknown Entities"), &g_Config.cvars.esp_ignore_unknown_ents);
		ImGui::Checkbox(xs("Draw Entity Index"), &g_Config.cvars.esp_box_index); ImGui::SameLine();
		ImGui::Checkbox(xs("Draw Distance"), &g_Config.cvars.esp_box_distance);
		ImGui::Checkbox(xs("Show Only Visible Players"), &g_Config.cvars.esp_show_visible_players);
		ImGui::Checkbox(xs("Draw Player Health"), &g_Config.cvars.esp_box_player_health);
		ImGui::Checkbox(xs("Draw Player Armor"), &g_Config.cvars.esp_box_player_armor);
		ImGui::Checkbox(xs("Draw Nicknames"), &g_Config.cvars.esp_box_player_name);
		ImGui::Checkbox(xs("Draw Entity Name"), &g_Config.cvars.esp_box_entity_name); ImGui::SameLine();
		ImGui::Checkbox(xs("Draw Skeleton"), &g_Config.cvars.esp_skeleton);
		ImGui::Checkbox(xs("Draw Names of Bones"), &g_Config.cvars.esp_bones_name);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Colors"));

		ImGuiCustom.Spacing(4);

		ImGui::ColorEdit3(xs("Friend Player Color"), g_Config.cvars.esp_friend_player_color);

		ImGui::ColorEdit3(xs("Enemy Player Color"), g_Config.cvars.esp_enemy_player_color);

		ImGui::ColorEdit3(xs("Friend Color"), g_Config.cvars.esp_friend_color);

		ImGui::ColorEdit3(xs("Enemy Color"), g_Config.cvars.esp_enemy_color);

		ImGui::ColorEdit3(xs("Neutral Color"), g_Config.cvars.esp_neutral_color);

		ImGui::ColorEdit3(xs("Item Color"), g_Config.cvars.esp_item_color);

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("esp-2"), ImVec2(328.5, 340), true);

		ImGui::PushItemWidth(150);

		ImGui::Text(xs("ESP Misc."));

		ImGuiCustom.Spacing(4);

		if (ImGui::Combo(xs("Player Style##style"), &g_Config.cvars.esp_player_style, esp_style, IM_ARRAYSIZE(esp_style)))
		{
			if (g_Config.cvars.esp_player_style == 0)
			{
				g_Config.cvars.esp_friend_player_color[0] = 0.f;
				g_Config.cvars.esp_friend_player_color[1] = 1.f;
				g_Config.cvars.esp_friend_player_color[2] = 0.f;
			}
			else if (g_Config.cvars.esp_player_style == 1)
			{
				g_Config.cvars.esp_box_player_health = true;
				g_Config.cvars.esp_box_player_armor = true;

				g_Config.cvars.esp_show_visible_players = true;
				g_Config.cvars.esp_box_distance = false;
				g_Config.cvars.esp_box_index = true;

				g_Config.cvars.esp_friend_player_color[0] = 1.f;
				g_Config.cvars.esp_friend_player_color[1] = 1.f;
				g_Config.cvars.esp_friend_player_color[2] = 1.f;
			}
			else if (g_Config.cvars.esp_player_style == 2)
			{
				g_Config.cvars.esp_box_player_health = false;
				g_Config.cvars.esp_box_player_armor = false;

				g_Config.cvars.esp_box_distance = false;
				g_Config.cvars.esp_box_index = false;

				g_Config.cvars.esp_friend_player_color[0] = 0.6f;
				g_Config.cvars.esp_friend_player_color[1] = 0.75f;
				g_Config.cvars.esp_friend_player_color[2] = 1.f;
			}
		}

		ImGui::Spacing();

		ImGui::Combo(xs("Entity Style##style2"), &g_Config.cvars.esp_entity_style, esp_style, IM_ARRAYSIZE(esp_style));

		ImGui::Spacing();

		ImGui::Combo(xs("Targets##esp"), &g_Config.cvars.esp_targets, esp_process_items, IM_ARRAYSIZE(esp_process_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Draw Distance Mode##esp"), &g_Config.cvars.esp_distance_mode, esp_process_items, IM_ARRAYSIZE(esp_process_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Draw Skeleton Mode##esp"), &g_Config.cvars.esp_skeleton_type, esp_process_items, IM_ARRAYSIZE(esp_process_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Draw Box Targets##esp"), &g_Config.cvars.esp_box_targets, esp_process_items, IM_ARRAYSIZE(esp_process_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Draw Box Type##esp"), &g_Config.cvars.esp_box, esp_box_items, IM_ARRAYSIZE(esp_box_items));

		ImGuiCustom.Spacing(8);

		ImGui::SliderFloat(xs("Distance##esp"), &g_Config.cvars.esp_distance, 1.0f, 8192.0f);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Box Fill Alpha##esp"), &g_Config.cvars.esp_box_fill, 0, 255);

		ImGui::EndChild();

		ImGui::PopItemWidth();
		break;
	}
	case 2: // Chams
	{
		ImGui::BeginChild(xs("chams"), ImVec2(328, 360), true);

		ImGui::PushItemWidth(150);

		ImGui::Checkbox(xs("Enable Chams"), &g_Config.cvars.chams);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Players"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Chams Players Behind Wall"), &g_Config.cvars.chams_players_wall);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Chams Players"), &g_Config.cvars.chams_players, 0, 3, chams_items[g_Config.cvars.chams_players]);

		ImGui::ColorEdit3(xs("Chams Players Color"), g_Config.cvars.chams_players_color);

		ImGui::ColorEdit3(xs("Chams Players Wall Color"), g_Config.cvars.chams_players_wall_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Entities"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Chams Entities Behind Wall"), &g_Config.cvars.chams_entities_wall);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Chams Entities"), &g_Config.cvars.chams_entities, 0, 3, chams_items[g_Config.cvars.chams_entities]);

		ImGui::ColorEdit3(xs("Chams Entities Color"), g_Config.cvars.chams_entities_color);

		ImGui::ColorEdit3(xs("Chams Entities Wall Color"), g_Config.cvars.chams_entities_wall_color);

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("chams-2"), ImVec2(328.5, 165), true);

		ImGui::PushItemWidth(160);

		ImGui::Text(xs("Items"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Chams Items Behind Wall"), &g_Config.cvars.chams_items_wall);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Chams Items"), &g_Config.cvars.chams_items, 0, 3, chams_items[g_Config.cvars.chams_items]);

		ImGui::ColorEdit3(xs("Chams Items Color"), g_Config.cvars.chams_items_color);

		ImGui::ColorEdit3(xs("Chams Items Wall Color"), g_Config.cvars.chams_items_wall_color);

		ImGui::EndChild();

		ImGui::PopItemWidth();
		break;
	}
	case 3: // Glow (Incl. Dynamic)
	{
		ImGui::BeginChild(xs("glow"), ImVec2(328, 520), true);

		ImGui::PushItemWidth(180);

		ImGui::Checkbox(xs("Enable Glow"), &g_Config.cvars.glow);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Optimize Glow Behind Wall"), &g_Config.cvars.glow_optimize);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Players"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Glow Players Behind Wall"), &g_Config.cvars.glow_players_wall);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Glow Players"), &g_Config.cvars.glow_players, 0, 3, glow_items[g_Config.cvars.glow_players]);

		ImGui::SliderInt(xs("Glow Players Width"), &g_Config.cvars.glow_players_width, 0, 30);

		ImGui::ColorEdit3(xs("Glow Players Color"), g_Config.cvars.glow_players_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Entities"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Glow Entities Behind Wall"), &g_Config.cvars.glow_entities_wall);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Glow Entities"), &g_Config.cvars.glow_entities, 0, 3, glow_items[g_Config.cvars.glow_entities]);

		ImGui::SliderInt(xs("Glow Entities Width"), &g_Config.cvars.glow_entities_width, 0, 30);

		ImGui::ColorEdit3(xs("Glow Entities Color"), g_Config.cvars.glow_entities_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Items"));

		ImGuiCustom.Spacing(8);

		ImGui::Checkbox(xs("Glow Items Behind Wall"), &g_Config.cvars.glow_items_wall);

		ImGuiCustom.Spacing(8);

		ImGui::SliderInt(xs("Glow Items"), &g_Config.cvars.glow_items, 0, 3, glow_items[g_Config.cvars.glow_items]);

		ImGui::SliderInt(xs("Glow Items Width"), &g_Config.cvars.glow_items_width, 0, 30);

		ImGui::ColorEdit3(xs("Glow Items Color"), g_Config.cvars.glow_items_color);

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("glow-dynamic"), ImVec2(328.5, 530), true);

		ImGui::PushItemWidth(140);

		ImGui::Text(xs("Dynamic Glow"));

		ImGui::SameLine();

		ImGui::Checkbox(xs("Dyn. Glow Attach To Targets"), &g_Config.cvars.dyn_glow_attach);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Self"));

		ImGui::SameLine();

		ImGui::Checkbox(xs("Dyn. Glow Self"), &g_Config.cvars.dyn_glow_self);

		ImGui::SliderFloat(xs("Dyn. Glow Self Radius"), &g_Config.cvars.dyn_glow_self_radius, 0.f, 4096.f);

		ImGui::SliderFloat(xs("Dyn. Glow Self Decay"), &g_Config.cvars.dyn_glow_self_decay, 0.f, 4096.f);

		ImGui::ColorEdit3(xs("Dyn. Glow Self Color"), g_Config.cvars.dyn_glow_self_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Players"));

		ImGui::SameLine();

		ImGui::Checkbox(xs("Dyn. Glow Players"), &g_Config.cvars.dyn_glow_players);

		ImGui::SliderFloat(xs("Dyn. Glow Players Radius"), &g_Config.cvars.dyn_glow_players_radius, 0.f, 4096.f);

		ImGui::SliderFloat(xs("Dyn. Glow Players Decay"), &g_Config.cvars.dyn_glow_players_decay, 0.f, 4096.f);

		ImGui::ColorEdit3(xs("Dyn. Glow Players Color"), g_Config.cvars.dyn_glow_players_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Entities"));

		ImGui::SameLine();

		ImGui::Checkbox(xs("Dyn. Glow Entities"), &g_Config.cvars.dyn_glow_entities);

		ImGui::SliderFloat(xs("Dyn. Glow Entities Radius"), &g_Config.cvars.dyn_glow_entities_radius, 0.f, 4096.f);

		ImGui::SliderFloat(xs("Dyn. Glow Entities Decay"), &g_Config.cvars.dyn_glow_entities_decay, 0.f, 4096.f);

		ImGui::ColorEdit3(xs("Dyn. Glow Entities Color"), g_Config.cvars.dyn_glow_entities_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Items"));

		ImGui::SameLine();

		ImGui::Checkbox(xs("Dyn. Glow Items"), &g_Config.cvars.dyn_glow_items);

		ImGui::SliderFloat(xs("Dyn. Glow Items Radius"), &g_Config.cvars.dyn_glow_items_radius, 0.f, 4096.f);

		ImGui::SliderFloat(xs("Dyn. Glow Items Decay"), &g_Config.cvars.dyn_glow_items_decay, 0.f, 4096.f);

		ImGui::ColorEdit3(xs("Dyn. Glow Items Color"), g_Config.cvars.dyn_glow_items_color);

		ImGui::EndChild();

		ImGui::PopItemWidth();
		break;
	}
	case 4: // Flashlight
	{
		ImGui::BeginChild(xs("esp"), ImVec2(328, 410), true);

		ImGui::PushItemWidth(150);

		ImGui::Checkbox(xs("Custom Flashlight"), &g_Config.cvars.custom_flashlight);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Local Player Flashlight"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Flashlight##localp"), &g_Config.cvars.flashlight_localplayer);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Distance##localp"), &g_Config.cvars.flashlight_localplayer_flashlight_distance, 1.f, 8192.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Falloff Distance##localp"), &g_Config.cvars.flashlight_localplayer_falloff_distance, 1.f, 8192.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Radius##localp"), &g_Config.cvars.flashlight_localplayer_radius, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Flashlight Color##localp"), g_Config.cvars.flashlight_localplayer_color);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Other Players Flashlight"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Flashlight##plr"), &g_Config.cvars.flashlight_players);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Distance##plr"), &g_Config.cvars.flashlight_players_flashlight_distance, 1.f, 8192.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Falloff Distance##plr"), &g_Config.cvars.flashlight_players_falloff_distance, 1.f, 8192.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Radius##plr"), &g_Config.cvars.flashlight_players_radius, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Flashlight Color##plr"), g_Config.cvars.flashlight_players_color);

		ImGui::PopItemWidth();

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("ESP2"), ImVec2(328.5, 315), true);

		ImGui::PushItemWidth(140);

		ImGui::Text(xs("Local Player Flashlight Lighting"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Flashlight Lighting##llocalp"), &g_Config.cvars.flashlight_lighting_localplayer);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Lighting Distance##llocalp"), &g_Config.cvars.flashlight_lighting_localplayer_distance, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Lighting Radius##llocalp"), &g_Config.cvars.flashlight_lighting_localplayer_radius, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Flashlight Lighting Color##llocalp"), g_Config.cvars.flashlight_lighting_localplayer_color);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Other Players Flashlight Lighting"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Flashlight Lighting##lplr"), &g_Config.cvars.flashlight_lighting_players);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Lighting Distance##lplr"), &g_Config.cvars.flashlight_lighting_players_distance, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Flashlight Lighting Radius##lplr"), &g_Config.cvars.flashlight_lighting_players_radius, 1.f, 1024.f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Flashlight Lighting Color##lplr"), g_Config.cvars.flashlight_lighting_players_color);

		ImGui::PopItemWidth();
		break;
	}
	case 5: // Wallhack
	{
		ImGui::BeginChild(xs("wallhack"), ImVec2(328, 170), true);

		ImGui::PushItemWidth(150);

		ImGui::Checkbox(xs("Simple Wallhack"), &g_Config.cvars.wallhack); ImGui::SameLine();
		ImGui::Checkbox(xs("Lambert Wallhack"), &g_Config.cvars.wallhack_white_walls);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Wireframe World"), &g_Config.cvars.wallhack_wireframe); ImGui::SameLine();
		ImGui::Checkbox(xs("Wireframe Models"), &g_Config.cvars.wallhack_wireframe_models);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Negative"), &g_Config.cvars.wallhack_negative);

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Wireframe Line Width"), &g_Config.cvars.wh_wireframe_width, 0.0f, 10.0f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Wireframe Color"), g_Config.cvars.wh_wireframe_color);

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 6: // BSP
	{
		ImGui::BeginChild(xs("bsp"), ImVec2(328, 160), true);

		ImGui::PushItemWidth(250);

		ImGui::Text(xs("BSP Info"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Spawns"), &g_Config.cvars.show_spawns);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Enable Triggers"), &g_Config.cvars.show_triggers);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Enable Triggers Info"), &g_Config.cvars.show_triggers_info);

		ImGui::Spacing();
		
		if (ImGui::BeginCombo("", xs("Triggers To Show"), ImGuiComboFlags_HeightLargest))
		{
			ImGui::Checkbox(xs("Show Trigger Once"), &g_Config.cvars.show_trigger_once);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Once Color"), g_Config.cvars.trigger_once_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Multiple"), &g_Config.cvars.show_trigger_multiple);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Multiple Color"), g_Config.cvars.trigger_multiple_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Hurt"), &g_Config.cvars.show_trigger_hurt);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Hurt Color"), g_Config.cvars.trigger_hurt_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Hurt (Heal)"), &g_Config.cvars.show_trigger_hurt_heal);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Hurt (Heal) Color"), g_Config.cvars.trigger_hurt_heal_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Push"), &g_Config.cvars.show_trigger_push);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Push Color"), g_Config.cvars.trigger_push_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Teleport"), &g_Config.cvars.show_trigger_teleport);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Teleport Color"), g_Config.cvars.trigger_teleport_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Trigger Changelevel"), &g_Config.cvars.show_trigger_changelevel);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Trigger Changelevel Color"), g_Config.cvars.trigger_changelevel_color);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Show Anti-Rush Trigger"), &g_Config.cvars.show_trigger_antirush);

			ImGui::Spacing();

			ImGui::ColorEdit4(xs("Anti-Rush Trigger Color"), g_Config.cvars.trigger_antirush_color);

			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 7: // Models Manager
	{
		ImGui::BeginChild(xs("models-manager"), ImVec2(328, 380), true);

		ImGui::PushItemWidth(150);

		ImGui::Text(xs("Replace Models of All Players"));

		ImGuiCustom.Spacing(4);

		if (ImGui::Checkbox(xs("Replace Models of All Players"), &g_Config.cvars.replace_players_models))
		{
			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetPlayersInfo();
		}
		if (ImGui::Checkbox(xs("Replace Model on Self"), &g_Config.cvars.replace_model_on_self))
		{
			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetLocalPlayerInfo();
		}

		ImGui::Spacing();

		ImGui::InputText(xs("Model to Replace"), g_szReplacePlayerModelBuffer, IM_ARRAYSIZE(g_szReplacePlayerModelBuffer));

		if (ImGui::Button(xs("Change Model##mm")))
		{
			g_ReplacePlayerModel = g_szReplacePlayerModelBuffer;

			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetPlayersInfo();
		}

		ImGuiCustom.Spacing(4);

		if (ImGui::Checkbox(xs("Replace All Players Models With Random Ones"), &g_Config.cvars.replace_players_models_with_randoms))
		{
			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetPlayersInfo();
		}

		ImGui::Spacing();

		if (ImGui::Button(xs("Reload List of Random Models")))
		{
			g_ModelsManager.ReloadRandomModels();
		}

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Replace Models of Specified Players"));

		ImGuiCustom.Spacing(4);

		if (ImGui::Checkbox(xs("Replace Models of Specified Players"), &g_Config.cvars.replace_specified_players_models))
		{
			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetPlayersInfo();
		}
		if (ImGui::Checkbox(xs("Don't Replace Models of Specified Players"), &g_Config.cvars.dont_replace_specified_players_models))
		{
			if (SvenModAPI()->GetClientState() == CLS_ACTIVE)
				g_ModelsManager.ResetPlayersInfo();
		}

		ImGui::Spacing();

		if (ImGui::Button(xs("Reload List of Players##mm")))
		{
			g_ModelsManager.ReloadTargetPlayers();
		}

		ImGui::Spacing();

		if (ImGui::Button(xs("Reload List of Ignored Players##mm")))
		{
			g_ModelsManager.ReloadIgnoredPlayers();
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 8: // Shaders
	{
		ImGui::BeginChild(xs("shaders"), ImVec2(328, 355), true);

		ImGui::Checkbox(xs("Enable##shaders"), &g_Config.cvars.shaders);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Depth Buffer"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Depth Buffer##shaders"), &g_Config.cvars.shaders_show_depth_buffer);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Z Near##depth"), &g_Config.cvars.shaders_depth_buffer_znear, 0.01f, 64.f);
		
		ImGui::Spacing();

		ImGui::SliderFloat(xs("Z Far##depth"), &g_Config.cvars.shaders_depth_buffer_zfar, 0.01f, 4096.f);
		
		ImGui::Spacing();

		ImGui::SliderFloat(xs("Brightness##depth"), &g_Config.cvars.shaders_depth_buffer_brightness, 0.f, 1.f);

		ImGui::Spacing();

		ImGui::Text(xs("Misc."));

		ImGui::Spacing();

		if (ImGui::BeginCombo("", xs("SSAO")), ImGuiComboFlags_HeightLargest)
		{
			ImGui::Text(xs("Screen-Space Ambient Occlusion"));

			ImGui::Spacing();

			ImGui::Checkbox(xs("Enable SSAO##shader"), &g_Config.cvars.shaders_ssao);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Only Ambient Occlusion##ssao"), &g_Config.cvars.shaders_ssao_onlyAO);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Z Near##ssao"), &g_Config.cvars.shaders_ssao_znear, 0.01f, 64.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Z Far##ssao"), &g_Config.cvars.shaders_ssao_zfar, 0.01f, 4096.f);

			ImGui::Spacing();

			ImGui::SliderInt(xs("Quality##ssao"), &g_Config.cvars.shaders_ssao_samples, 1, 64);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Strength##ssao"), &g_Config.cvars.shaders_ssao_strength, 0.0f, 10.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Radius##ssao"), &g_Config.cvars.shaders_ssao_radius, 0.0f, 50.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Depth Clamp##ssao"), &g_Config.cvars.shaders_ssao_aoclamp, 0.0f, 5.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Self-Shadowing Reduction##ssao"), &g_Config.cvars.shaders_ssao_diffarea, 0.0f, 5.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Gauss Bell Center##ssao"), &g_Config.cvars.shaders_ssao_gdisplace, 0.0f, 5.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Luminance Affection##ssao"), &g_Config.cvars.shaders_ssao_lumInfluence, 0.0f, 5.f);

			ImGuiCustom.Spacing(4);

			ImGui::Text(xs("Noise"));

			ImGui::Spacing();

			ImGui::Checkbox(xs("Use Noise Instead of Patterns##ssao"), &g_Config.cvars.shaders_ssao_noise);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Dithering Amount##ssao"), &g_Config.cvars.shaders_ssao_noiseamount, 0.0f, 0.1f, "%.5f");

			ImGuiCustom.Spacing(4);

			ImGui::Text(xs("Mist"));

			ImGui::Spacing();

			ImGui::Checkbox(xs("Enable Mist##ssao"), &g_Config.cvars.shaders_ssao_mist);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Start##ssao"), &g_Config.cvars.shaders_ssao_miststart, 0.f, 2048.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("End##ssao"), &g_Config.cvars.shaders_ssao_mistend, 0.01f, 4096.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo(" ", xs("Color Correction"), ImGuiComboFlags_HeightLargest))
		{
			ImGui::Checkbox(xs("Enable Color Correction##shader"), &g_Config.cvars.shaders_color_correction);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Grain##clrcor"), &g_Config.cvars.shaders_cc_grain, 0.0f, 512.f);

			ImGui::Spacing();

			//ImGui::SliderFloat(xs("Sharpness##clrcor"), &g_Config.cvars.shaders_cc_sharpness, 0.f, 100.f);
			//
			//ImGui::Spacing();

			ImGui::SliderFloat(xs("Gamma##clrcor"), &g_Config.cvars.shaders_cc_target_gamma, 0.01f, 4.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Monitor Gamma##clrcor"), &g_Config.cvars.shaders_cc_monitor_gamma, 0.01f, 4.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Hue Adjustment##clrcor"), &g_Config.cvars.shaders_cc_hue_offset, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Saturation##clrcor"), &g_Config.cvars.shaders_cc_saturation, 0.f, 10.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Contrast##clrcor"), &g_Config.cvars.shaders_cc_contrast, 0.f, 10.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Luminance##clrcor"), &g_Config.cvars.shaders_cc_luminance, 0.f, 10.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Bright Boost##clrcor"), &g_Config.cvars.shaders_cc_bright_boost, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Black Level##clrcor"), &g_Config.cvars.shaders_cc_black_level, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Red Channel Level##clrcor"), &g_Config.cvars.shaders_cc_R, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Green Channel Level##clrcor"), &g_Config.cvars.shaders_cc_G, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Blue Channel Level##clrcor"), &g_Config.cvars.shaders_cc_B, 0.f, 1.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("  ", xs("Chromatic Aberration"), ImGuiComboFlags_HeightLarge))
		{
			ImGui::Checkbox(xs("Enable Chromatic Aberration##shaders"), &g_Config.cvars.shaders_chromatic_aberration);

			ImGui::Spacing();

			ImGui::SliderInt(xs("Type##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_type, 0, 2, ca_types[g_Config.cvars.shaders_chromatic_aberration_type]);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Direction X##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_dir_x, -50.f, 50.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Direction Y##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_dir_y, -50.f, 50.f);

			ImGui::Spacing();

			if (g_Config.cvars.shaders_chromatic_aberration_type == 0)
			{
				ImGui::SliderFloat(xs("Shift##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_shift, 0.f, 50.f);
			}
			else
			{
				g_Config.cvars.shaders_chromatic_aberration_shift = clamp(v, 0.f, 1.f);

				ImGui::SliderFloat(xs("Shift##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_shift, 0.f, 1.f);
			}

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Strength##aberrat"), &g_Config.cvars.shaders_chromatic_aberration_strength, 0.f, 10.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("   ", xs("Vignette"), 0))
		{
			ImGui::Checkbox(xs("Enable Vignette##shaders"), &g_Config.cvars.shaders_vignette);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Falloff##vignette"), &g_Config.cvars.shaders_vignette_falloff, 0.f, 1.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Amount##vignette"), &g_Config.cvars.shaders_vignette_amount, 0.f, 5.f);

			ImGui::EndCombo();
		}

		ImGui::EndChild();


		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("shaders-blur"), ImVec2(328.5, 240), true);

		ImGui::Text(xs("Blur"));

		ImGuiCustom.Spacing(4);

		if (ImGui::BeginCombo("", xs("Depth of Field Blur"), ImGuiComboFlags_HeightLarge))
		{
			ImGui::Checkbox(xs("Enable DoF Blur##shader"), &g_Config.cvars.shaders_dof_blur);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Min. Range##dof"), &g_Config.cvars.shaders_dof_blur_min_range, 0.01f, 4096.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Max. Range##dof"), &g_Config.cvars.shaders_dof_blur_max_range, 0.01f, 4096.f);

			ImGui::Spacing();

			ImGui::SliderInt(xs("Interpolation Type"), &g_Config.cvars.shaders_dof_blur_interp_type, 0, 4, dof_interps[g_Config.cvars.shaders_dof_blur_interp_type]);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Bluriness Range##dof"), &g_Config.cvars.shaders_dof_blur_bluriness_range, 0.0f, 150.f);

			ImGui::Spacing();

			ImGui::SliderInt(xs("Quality##dof"), &g_Config.cvars.shaders_dof_blur_quality, 1, 50);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Bokeh Coefficient##dof"), &g_Config.cvars.shaders_dof_blur_bokeh, 0.f, 1.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo(" ", xs("Motion Blur"), 0))
		{
			ImGui::Checkbox(xs("Enable Motion Blur##shader"), &g_Config.cvars.shaders_motion_blur);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Min. Speed##motblur"), &g_Config.cvars.shaders_motion_blur_min_speed, 0.01f, 2000.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Max. Speed##motblur"), &g_Config.cvars.shaders_motion_blur_max_speed, 0.01f, 2000.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Strength##motblur"), &g_Config.cvars.shaders_motion_blur_strength, 0.0f, 50.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("  ", xs("Radial Blur"), 0))
		{
			ImGui::Checkbox(xs("Enable Radial Blur##shader"), &g_Config.cvars.shaders_radial_blur);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Distance##radblur"), &g_Config.cvars.shaders_radial_blur_distance, 0.0f, 10.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Strength##radblur"), &g_Config.cvars.shaders_radial_blur_strength, 0.0f, 50.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("   ", xs("Bokeh Blur"), 0))
		{
			ImGui::Checkbox(xs("Enable Bokeh Blur##shader"), &g_Config.cvars.shaders_bokeh_blur);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Bluriness Radius##bokeh"), &g_Config.cvars.shaders_bokeh_blur_radius, 0.0f, 150.f);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Bokeh Coefficient##bokeh"), &g_Config.cvars.shaders_bokeh_blur_coeff, 0.0f, 1.f);

			ImGui::Spacing();

			ImGui::SliderInt(xs("Quality##bokeh"), &g_Config.cvars.shaders_bokeh_blur_samples, 1, 50);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("    ", xs("Gaussian Blur"), 0))
		{
			ImGui::Checkbox(xs("Enable Gaussian Blur##shader"), &g_Config.cvars.shaders_gaussian_blur);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Radius##gaussian"), &g_Config.cvars.shaders_gaussian_blur_radius, 0.0f, 150.f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("     ", xs("Gaussian Blur Fast"), 0))
		{
			ImGui::Checkbox(xs("Enable Gaussian Blur Fast##shader"), &g_Config.cvars.shaders_gaussian_blur_fast);

			ImGui::Spacing();

			ImGui::SliderFloat(xs("Radius##gaussianfast"), &g_Config.cvars.shaders_gaussian_blur_fast_radius, 0.0f, 15.f);

			ImGui::EndCombo();
		}

		ImGui::EndChild();
		break;
	}
	case 9: // Misc
	{

		ImGui::BeginChild(xs("misc"), ImVec2(328, 410), true);

		ImGui::PushItemWidth(160);

		extern void ConCommand_ChangeSkybox(const CCommand & args);
		extern void ConCommand_ResetSkybox();

		extern const char* g_szSkyboxes[];
		extern int g_iSkyboxesSize;
		extern bool g_bMenuChangeSkybox;

		ImGui::Text(xs("Skybox Changer"));

		ImGuiCustom.Spacing(4);

		if (ImGui::Combo(xs("Skybox Name"), &g_Config.cvars.skybox, g_szSkyboxes, g_iSkyboxesSize))
		{
			g_bMenuChangeSkybox = true;

			ConCommand_ChangeSkybox(s_DummyCommand);

			g_bMenuChangeSkybox = false;
		}

		ImGuiCustom.Spacing(4);

		if (ImGui::Button(xs("Reset Skybox")))
		{
			ConCommand_ResetSkybox();
		}

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Player's Push Direction"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Player's Push Direction"), &g_Config.cvars.show_players_push_direction);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Push Direction Length"), &g_Config.cvars.push_direction_length, 0.0f, 256.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Push Direction Width"), &g_Config.cvars.push_direction_width, 0.01f, 100.0f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Push Direction Color"), g_Config.cvars.push_direction_color);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Player's Sight Direction"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Players Sight Direction"), &g_Config.cvars.show_players_sight_direction);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Sight Direction Length"), &g_Config.cvars.players_sight_direction_length, 0.0f, 256.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Sight Direction Width"), &g_Config.cvars.players_sight_direction_width, 0.01f, 100.0f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Sight Direction Color"), g_Config.cvars.players_sight_direction_color);

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("misc2"), ImVec2(328.5, 235), true);

		ImGui::PushItemWidth(160);

		ImGui::Text(xs("Fog"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Enable Fog"), &g_Config.cvars.fog);

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Fog Skybox"), &g_Config.cvars.fog_skybox);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Disable Water Fog"), &g_Config.cvars.remove_water_fog);

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Fog Start"), &g_Config.cvars.fog_start, 0.0f, 10000.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Fog End"), &g_Config.cvars.fog_end, 0.0f, 10000.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Density"), &g_Config.cvars.fog_density, 0.0f, 10.0f);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Color"), g_Config.cvars.fog_color);

		ImGui::EndChild();
		ImGui::PopItemWidth();
		break;
	}
	}
}

void CMenuModule::DrawHUDTabContent()
{
	switch (selectedSubTab1)
	{
	case 0: // General (HUD Color Remap, Crosshair)
	{
		ImGui::BeginChild(xs("general"), ImVec2(328, 420), true);

		ImGui::Text(xs("Custom HUD Color"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Change HUD Color"), &g_Config.cvars.remap_hud_color); g_Config.cvars.tooltips ? ImGui::SameLine(), ImGuiCustom.ToolTip(xs("Let's you change your HUD color")) : void(nullptr);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("HUD Color"), g_Config.cvars.hud_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Crosshair"));

		ImGui::PushItemWidth(130);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Draw Crosshair"), &g_Config.cvars.draw_crosshair);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Draw Crosshair Dot"), &g_Config.cvars.draw_crosshair_dot);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Draw Crosshair Outline"), &g_Config.cvars.draw_crosshair_outline);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Crosshair Size"), &g_Config.cvars.crosshair_size, 1, 50);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Crosshair Gap"), &g_Config.cvars.crosshair_gap, 0, 50);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Crosshair Thickness"), &g_Config.cvars.crosshair_thickness, 1, 50);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Crosshair Outline Thickness"), &g_Config.cvars.crosshair_outline_thickness, 1, 50);

		ImGuiCustom.Spacing(4);

		ImGui::PopItemWidth();

		ImGui::PushItemWidth(160);

		ImGui::ColorEdit4(xs("Crosshair Color"), g_Config.cvars.crosshair_color);

		ImGui::Spacing();

		ImGui::ColorEdit4(xs("Crosshair Outline Color"), g_Config.cvars.crosshair_outline_color);

		ImGui::PopItemWidth();

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("ESP2"), ImVec2(328.5, 195), true);

		ImGui::PushItemWidth(160);

		ImGui::Text(xs("Grenade's Timer"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Grenade's Timer"), &g_Config.cvars.grenade_timer); g_Config.cvars.tooltips ? ImGui::SameLine(), ImGuiCustom.ToolTip(xs("Shows a timer that indicates the relative time left to a player held grenade explosion")) : void(nullptr);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Timer Color##nade_t"), g_Config.cvars.grenade_timer_color);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Explosion Time Color##nade_t"), g_Config.cvars.grenade_explosive_time_color);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Width Fraction##nade_t"), &g_Config.cvars.grenade_timer_width_frac, 0.f, 1.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Height Fraction##nade_t"), &g_Config.cvars.grenade_timer_height_frac, 0.f, 1.f);

		ImGui::EndChild();
		ImGui::PopItemWidth();
		break;
	}
	case 1: // Speedometer
	{
		ImGui::BeginChild(xs("speedometer"), ImVec2(328, 470), true);

		ImGui::PushItemWidth(120);

		ImGui::Checkbox(xs("Show Speedometer"), &g_Config.cvars.show_speed);

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Show Jump's Speed"), &g_Config.cvars.show_jumpspeed);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Store Vertical Speed"), &g_Config.cvars.show_vertical_speed);

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Jump's Speed: Fade Duration"), &g_Config.cvars.jumpspeed_fade_duration, 0.1f, 2.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Speed Width Fraction"), &g_Config.cvars.speed_width_fraction, 0.0f, 1.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Speed Height Fraction"), &g_Config.cvars.speed_height_fraction, 0.0f, 1.0f);

		ImGui::PopItemWidth();
		ImGuiCustom.Spacing(4);
		ImGui::PushItemWidth(200);

		ImGui::ColorEdit3(xs("Speed Color"), g_Config.cvars.speed_color);

		ImGui::Spacing();

		if (ImGui::Button(xs("Reset Color of Speedometer")))
		{
			g_Config.cvars.speed_color[0] = 100.f / 255.f;
			g_Config.cvars.speed_color[1] = 130.f / 255.f;
			g_Config.cvars.speed_color[2] = 200.f / 255.f;
		}

		ImGui::PopItemWidth();
		ImGui::PushItemWidth(120);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Legacy Speedometer"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Show Speedometer (Legacy)"), &g_Config.cvars.show_speed_legacy);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Store Vertical Speed (Legacy)"), &g_Config.cvars.show_vertical_speed_legacy);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Speed Width Fraction (Legacy)"), &g_Config.cvars.speed_width_fraction_legacy, 0.0f, 1.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Speed Height Fraction (Legacy)"), &g_Config.cvars.speed_height_fraction_legacy, 0.0f, 1.0f);

		ImGui::PopItemWidth();
		ImGuiCustom.Spacing(4);
		ImGui::PushItemWidth(170);

		ImGui::ColorEdit3(xs("Speed Color (Legacy)"), g_Config.cvars.speed_color_legacy);

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 2: // Radar
	{
		ImGui::BeginChild(xs("radar"), ImVec2(328, 265), true);

		ImGui::PushItemWidth(180);

		ImGui::Checkbox(xs("Enable Radar"), &g_Config.cvars.radar);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Player Name##radar"), &g_Config.cvars.radar_show_player_name);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Entity Name##radar"), &g_Config.cvars.radar_show_entity_name);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Type##rdr"), &g_Config.cvars.radar_type, 0, 1, radar_type[g_Config.cvars.radar_type]);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Size##rdr"), &g_Config.cvars.radar_size, 1, 1000);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Distance##rdr"), &g_Config.cvars.radar_distance, 1.f, 16384.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Width Fraction##rdr"), &g_Config.cvars.radar_width_frac, 0.f, 1.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Height Fraction##rdr"), &g_Config.cvars.radar_height_frac, 0.f, 1.f);

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 3: // Chat Colors
	{
		ImGui::BeginChild(xs("chat-colors"), ImVec2(328, 525), true);

		extern void ConCommand_ChatColorsLoadPlayers();

		ImGui::Checkbox(xs("Enable Chat Colors"), &g_Config.cvars.enable_chat_colors);

		ImGuiCustom.Spacing(4);

		if (ImGui::Button(xs("Load Players List")))
		{
			ConCommand_ChatColorsLoadPlayers();
		}

		ImGui::Spacing();

		if (ImGui::Button(xs("Reset Default Player Color")))
		{
			g_Config.cvars.player_name_color[0] = 0.6f;
			g_Config.cvars.player_name_color[1] = 0.75f;
			g_Config.cvars.player_name_color[2] = 1.0f;
		}

		ImGuiCustom.Spacing(4);

		ImGui::PushItemWidth(170);

		ImGui::ColorEdit3(xs("Default Player Color"), g_Config.cvars.player_name_color);

		ImGui::PopItemWidth();

		ImGuiCustom.Spacing(4);

		ImGui::PushItemWidth(150);

		ImGui::Text(xs("Rainbow Names"));

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Rainbow Update Delay"), &g_Config.cvars.chat_rainbow_update_delay, 0.0f, 0.5f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Rainbow Hue Delta"), &g_Config.cvars.chat_rainbow_hue_delta, 0.0f, 0.5f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Rainbow Saturation"), &g_Config.cvars.chat_rainbow_saturation, 0.0f, 1.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Rainbow Lightness"), &g_Config.cvars.chat_rainbow_lightness, 0.0f, 1.0f);

		ImGuiCustom.Spacing(8);

		ImGui::PopItemWidth();
		ImGui::PushItemWidth(185);

		ImGui::Text(xs("Custom Colors"));

		ImGuiCustom.Spacing(4);

		ImGui::ColorEdit3(xs("Custom Color #1"), g_Config.cvars.chat_color_one);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Custom Color #2"), g_Config.cvars.chat_color_two);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Custom Color #3"), g_Config.cvars.chat_color_three);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Custom Color #4"), g_Config.cvars.chat_color_four);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Custom Color #5"), g_Config.cvars.chat_color_five);

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("Custom Color #6"), g_Config.cvars.chat_color_six);

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 4: // Custom Vote Popup 
	{
		ImGui::BeginChild(xs("custom-vote-popup"), ImVec2(328, 325), true);

		ImGui::PushItemWidth(180);

		ImGui::Checkbox(xs("Enable Custom Vote Popup"), &g_Config.cvars.vote_popup);

		ImGui::Spacing();

		ImGui::InputInt(xs("Yes Key##votepopup"), &g_Config.cvars.vote_popup_yes_key);

		ImGui::Spacing();

		ImGui::InputInt(xs("No Key##votepopup"), &g_Config.cvars.vote_popup_no_key);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Width Size##cvp"), &g_Config.cvars.vote_popup_width_size, 0, 1000);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Height Size##cvp"), &g_Config.cvars.vote_popup_height_size, 0, 1000);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Width Border Pixels##cvp"), &g_Config.cvars.vote_popup_w_border_pix, 0, 100);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Height Border Pixels##cvp"), &g_Config.cvars.vote_popup_h_border_pix, 0, 100);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Width Fraction##cvp"), &g_Config.cvars.vote_popup_width_frac, 0.0f, 1.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Height Fraction##cvp"), &g_Config.cvars.vote_popup_height_frac, 0.0f, 1.0f);

		ImGuiCustom.Spacing(4);

		if (ImGui::Button(xs("Restore Default Values")))
		{
			g_Config.cvars.vote_popup_width_size = 250;
			g_Config.cvars.vote_popup_height_size = 125;
			g_Config.cvars.vote_popup_w_border_pix = 12;
			g_Config.cvars.vote_popup_h_border_pix = 7;
			g_Config.cvars.vote_popup_width_frac = 0.015f;
			g_Config.cvars.vote_popup_height_frac = 0.37f;
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	}
}

void CMenuModule::DrawUtilityTabContent()
{
	switch (selectedSubTab2)
	{
	case 0: // Player (Generals (Sink,Freeze,Autojump, etc..), Spinner, Aimbot, No Recoil)
	{
		ImGui::BeginChild(xs("player"), ImVec2(328, 435), true);

		extern void ConCommand_AutoSelfSink();
		extern void ConCommand_Freeze();
		extern void ConCommand_Freeze2();
		extern void ConCommand_DropEmptyWeapon();

		ImGui::Text(xs("General"));

		ImGui::Spacing();

		if (ImGui::Button(xs("Selfsink")))
			ConCommand_AutoSelfSink();

		ImGui::SameLine();

		if (ImGui::Button(xs("Drop Empty Weapon")))
			ConCommand_DropEmptyWeapon();

		ImGui::Spacing();

		if (ImGui::Button(xs("Freeze")))
			ConCommand_Freeze();

		ImGui::SameLine();

		if (ImGui::Button(xs("Freeze #2")))
			ConCommand_Freeze2();

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Autojump"), &g_Config.cvars.autojump);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Jumpbug"), &g_Config.cvars.jumpbug);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Edgejump"), &g_Config.cvars.edgejump);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Doubleduck"), &g_Config.cvars.ducktap);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Fastrun"), &g_Config.cvars.fastrun);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Auto Ceil-Clipping"), &g_Config.cvars.auto_ceil_clipping); ImGui::SameLine();
		ImGui::Checkbox(xs("Tertiary Attack Glitch"), &g_Config.cvars.tertiary_attack_glitch);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Rotate Dead Body"), &g_Config.cvars.rotate_dead_body); ImGui::SameLine();
		ImGui::Checkbox(xs("Quake Guns"), &g_Config.cvars.quake_guns);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Revert Pitch"), &g_Config.cvars.revert_pitch); ImGui::SameLine();
		ImGui::Checkbox(xs("Revert Yaw"), &g_Config.cvars.revert_yaw);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Auto Wallstrafing"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Enable Auto Wallstrafing"), &g_Config.cvars.auto_wallstrafing);

		ImGuiCustom.Spacing(4);

		ImGui::PushItemWidth(170);

		ImGui::SliderFloat(xs("Angle (~6.5 is perfect)"), &g_Config.cvars.wallstrafing_angle, 0.f, 45.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Max Distance to Wall"), &g_Config.cvars.wallstrafing_dist, 1.f, 128.f);

		ImGui::PopItemWidth();

		ImGuiCustom.Spacing(8);

		if (ImGui::BeginCombo("", xs("Spinner & View Angles"), ImGuiComboFlags_HeightLarge))
		{
			ImGui::Text(xs("Spinner"));

			ImGui::Spacing();

			ImGui::Checkbox(xs("Inclined Rotation"), &g_Config.cvars.spin_pitch_angle);

			ImGui::SliderFloat(xs("Inclined Rotation: Angle"), &g_Config.cvars.spin_pitch_rotation_angle, -10.0f, 10.0f);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Spin Yaw"), &g_Config.cvars.spin_yaw_angle);

			ImGui::SliderFloat(xs("Spin Yaw: Angle"), &g_Config.cvars.spin_yaw_rotation_angle, -10.0f, 10.0f);

			ImGuiCustom.Spacing(8);

			ImGui::Text(xs("Lock View Angles"));

			ImGui::Spacing();

			ImGui::Checkbox(xs("Lock Pitch"), &g_Config.cvars.lock_pitch);

			ImGui::SliderFloat(xs("Lock Pitch: Angle"), &g_Config.cvars.lock_pitch_angle, -179.999f, 180.0f);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Lock Yaw"), &g_Config.cvars.lock_yaw);

			ImGui::SliderFloat(xs("Lock Yaw: Angle"), &g_Config.cvars.lock_yaw_angle, 0.0f, 360.0f);

			ImGui::EndCombo();
		}

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("player2"), ImVec2(328.5, 500), true);

		ImGui::Text(xs("Strafer"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Enable Strafer"), &g_Config.cvars.strafe);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Ignore Ground"), &g_Config.cvars.strafe_ignore_ground);

		ImGuiCustom.Spacing(4);

		ImGui::Combo(xs("Strafe Direction"), &g_Config.cvars.strafe_dir, strafe_dir_items, IM_ARRAYSIZE(strafe_dir_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Strafe Type"), &g_Config.cvars.strafe_type, strafe_type_items, IM_ARRAYSIZE(strafe_type_items));

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Aimbot"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Aimbot"), &g_Config.cvars.aimbot);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Silent Aimbot"), &g_Config.cvars.silent_aimbot);

		ImGui::SameLine();

		ImGui::Checkbox(xs("Ragebot"), &g_Config.cvars.ragebot);

		ImGuiCustom.Spacing(4);

		if (ImGui::BeginCombo("", xs("Aim Type"), 0))
		{
			ImGui::Checkbox(xs("Aim to Hitboxes"), &g_Config.cvars.aimbot_aim_hitboxes);
			ImGui::Checkbox(xs("Aim to Head"), &g_Config.cvars.aimbot_aim_head);
			ImGui::Checkbox(xs("Aim to Neck"), &g_Config.cvars.aimbot_aim_neck);
			ImGui::Checkbox(xs("Aim to Chest"), &g_Config.cvars.aimbot_aim_chest);
			ImGui::Checkbox(xs("Aim to Unknown Entities"), &g_Config.cvars.aimbot_aim_unknown_ents);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		ImGui::Checkbox(xs("Ignore Glass"), &g_Config.cvars.aimbot_ignore_glass);
		ImGui::Checkbox(xs("Ignore Studio Models"), &g_Config.cvars.aimbot_ignore_blockers);

		ImGui::Spacing();

		ImGui::PushItemWidth(180);

		ImGui::Checkbox(xs("Consider FOV"), &g_Config.cvars.aimbot_consider_fov);

		ImGui::SliderFloat(xs("Aimbot FOV"), &g_Config.cvars.aimbot_fov, 0.0f, 180.0f);

		ImGui::SliderFloat(xs("Aimbot Distance"), &g_Config.cvars.aimbot_distance, 0.0f, 8192.0f);

		ImGui::PopItemWidth();

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Recoil & Spread"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("No Recoil"), &g_Config.cvars.no_recoil);

		ImGui::Checkbox(xs("No Recoil [Visual]"), &g_Config.cvars.no_recoil_visual);

		ImGui::EndChild();
		break;
	}
	case 1: // Color Pulsator 
	{
		extern void ConCommand_ResetColors();
		extern void ConCommand_SyncColors();

		ImGui::BeginChild(xs("color-pulsator"), ImVec2(328, 200), true);

		ImGui::Checkbox(xs("Enable Pulsator"), &g_Config.cvars.color_pulsator);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Change Top Color"), &g_Config.cvars.color_pulsator_top);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Change Bottom Color"), &g_Config.cvars.color_pulsator_bottom);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Change Color Delay"));

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat("  ", &g_Config.cvars.color_pulsator_delay, 0.1f, 2.5f);

		ImGui::Spacing();

		if (ImGui::Button(xs("Reset Colors")))
			ConCommand_ResetColors();

		ImGui::SameLine();

		if (ImGui::Button(xs("Sync. Colors")))
			ConCommand_SyncColors();

		ImGui::EndChild();
		break;
	}
	case 2: // Fake Lag 
	{
		ImGui::BeginChild(xs("fake-lag"), ImVec2(328, 210), true);

		ImGui::Checkbox(xs("Enable Fake Lag"), &g_Config.cvars.fakelag);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Adaptive Ex Interp"), &g_Config.cvars.fakelag_adaptive_ex_interp);

		ImGuiCustom.Spacing(4);

		ImGui::SliderInt(xs("Limit"), &g_Config.cvars.fakelag_limit, 0, 256);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Variance"), &g_Config.cvars.fakelag_variance, 0.0f, 100.0f);

		ImGui::Spacing();

		ImGui::Combo(xs("Fake Lag Type"), &g_Config.cvars.fakelag_type, fakelag_type_items, IM_ARRAYSIZE(fakelag_type_items));

		ImGui::Spacing();

		ImGui::Combo(xs("Fake Move Type"), &g_Config.cvars.fakelag_move, fakelag_move_items, IM_ARRAYSIZE(fakelag_move_items));

		ImGui::EndChild();
		break;
	}
	case 3: // Anti-AFK
	{
		ImGui::BeginChild(xs("anti-afk"), ImVec2(328, 240), true);

		ImGui::Combo(xs("Mode"), &g_Config.cvars.antiafk, antiafk_items, IM_ARRAYSIZE(antiafk_items));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Anti-AFK Rotate Camera"), &g_Config.cvars.antiafk_rotate_camera);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Anti-AFK Stay Within Range"), &g_Config.cvars.antiafk_stay_within_range);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Anti-AFK Reset Stay Position"), &g_Config.cvars.antiafk_reset_stay_pos);

		ImGuiCustom.Spacing(4);

		ImGui::PushItemWidth(150);

		ImGui::SliderFloat(xs("Rotation Angle"), &g_Config.cvars.antiafk_rotation_angle, -7.0f, 7.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Stay Within Radius"), &g_Config.cvars.antiafk_stay_radius, 10.0f, 500.0f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Stay Within Spread Angle"), &g_Config.cvars.antiafk_stay_radius_offset_angle, 0.0f, 89.0f);

		ImGui::PopItemWidth();

		ImGui::EndChild();
		break;
	}
	case 4: // Spammer
	{
		ImGui::BeginChild(xs("spammer"), ImVec2(328, 270), true);

		extern void ConCommand_PrintSpamKeyWords(void);
		extern void ConCommand_PrintSpamTasks(void);

		ImGui::Text(xs("Key Spammer"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Hold Mode"), &g_Config.cvars.keyspam_hold_mode);

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Spam E"), &g_Config.cvars.keyspam_e); ImGui::SameLine();
		ImGui::Checkbox(xs("Spam Q"), &g_Config.cvars.keyspam_q);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Spam W"), &g_Config.cvars.keyspam_w); ImGui::SameLine();
		ImGui::Checkbox(xs("Spam S"), &g_Config.cvars.keyspam_s);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Spam CTRL"), &g_Config.cvars.keyspam_ctrl);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Message Spammer"));

		ImGuiCustom.Spacing(4);

		if (ImGui::Button(xs("Show Spam Tasks")))
			ConCommand_PrintSpamTasks();

		ImGui::Spacing();

		if (ImGui::Button(xs("Show Spam Keywords")))
			ConCommand_PrintSpamKeyWords();

		ImGui::EndChild();
		break;
	}
	case 5: // Speedrun Tools
	{
		ImGui::BeginChild(xs("speedrun-tools"), ImVec2(328, 325), true);

		ImGui::Text(xs("Timer"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Enable Timer##st"), &g_Config.cvars.st_timer);

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Width Fraction##st"), &g_Config.cvars.st_timer_width_frac, 0.f, 1.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Height Fraction##st"), &g_Config.cvars.st_timer_height_frac, 0.f, 1.f);

		ImGuiCustom.Spacing(4);

		ImGui::ColorEdit3(xs("Timer Color##st"), g_Config.cvars.st_timer_color);

		ImGuiCustom.Spacing(4);

		ImGui::Text(xs("Player Hulls"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Show Hulls of Players##st"), &g_Config.cvars.st_player_hulls); 
		ImGui::Checkbox(xs("Show Server's Hulls of Players##st"), &g_Config.cvars.st_server_player_hulls);

		ImGuiCustom.Spacing(4);

		ImGui::ColorEdit4(xs("Hull Color##st"), g_Config.cvars.st_player_hulls_color);

		ImGui::Spacing();

		ImGui::ColorEdit4(xs("Hull Dead Color##st"), g_Config.cvars.st_player_hulls_dead_color);


		ImGui::EndChild();


		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("speedrun-tools2"), ImVec2(328.5, 260), true);

		ImGui::Text(xs("HUD Info"));

		ImGui::Spacing();

		ImGui::ColorEdit3(xs("HUD Color##st"), g_Config.cvars.st_hud_color);

		ImGui::Spacing();

		if (ImGui::BeginCombo("", xs("View Angles"), 0))
		{
			ImGui::Checkbox(xs("Show View Angles##st"), &g_Config.cvars.st_show_view_angles);

			ImGui::SliderFloat(xs("Width Fraction##st_va"), &g_Config.cvars.st_show_view_angles_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_va"), &g_Config.cvars.st_show_view_angles_height_frac, 0.0f, 1.0f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo(" ", xs("Position"), 0))
		{
			ImGui::Checkbox(xs("Show Position##st"), &g_Config.cvars.st_show_pos); ImGui::SameLine();

			ImGui::Checkbox(xs("Use View Origin##st_pos"), &g_Config.cvars.st_show_pos_view_origin);

			ImGui::SliderFloat(xs("Width Fraction##st_pos"), &g_Config.cvars.st_show_pos_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_pos"), &g_Config.cvars.st_show_pos_height_frac, 0.0f, 1.0f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("  ", xs("Velocity"), 0))
		{
			ImGui::Checkbox(xs("Show Velocity##st"), &g_Config.cvars.st_show_velocity);

			ImGui::SliderFloat(xs("Width Fraction##st_vel"), &g_Config.cvars.st_show_velocity_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_vel"), &g_Config.cvars.st_show_velocity_height_frac, 0.0f, 1.0f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("   ", xs("Gauss Boost"), 0))
		{
			ImGui::Checkbox(xs("Show Gauss Boost Info##st"), &g_Config.cvars.st_show_gauss_boost_info);

			ImGui::SliderFloat(xs("Width Fraction##st_gaussboost"), &g_Config.cvars.st_show_gauss_boost_info_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_gaussboost"), &g_Config.cvars.st_show_gauss_boost_info_height_frac, 0.0f, 1.0f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("    ", xs("Selfgauss"), 0))
		{
			ImGui::Checkbox(xs("Show Selfgauss Info##st"), &g_Config.cvars.st_show_selfgauss_info);

			ImGui::SliderFloat(xs("Width Fraction##st_selfgauss"), &g_Config.cvars.st_show_selfgauss_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_selfgauss"), &g_Config.cvars.st_show_selfgauss_height_frac, 0.0f, 1.0f);

			ImGui::EndCombo();
		}

		ImGui::Spacing();

		if (ImGui::BeginCombo("     ", xs("Entity"), 0))
		{
			ImGui::Checkbox(xs("Show Entity Info##st"), &g_Config.cvars.st_show_entity_info); ImGui::SameLine();
			ImGui::Checkbox(xs("Check Players##st_entinfo"), &g_Config.cvars.st_show_entity_info_check_players);

			ImGui::SliderFloat(xs("Width Fraction##st_ent"), &g_Config.cvars.st_show_entity_info_width_frac, 0.0f, 1.0f);
			ImGui::SliderFloat(xs("Height Fraction##st_ent"), &g_Config.cvars.st_show_entity_info_height_frac, 0.0f, 1.0f);
			ImGui::EndCombo();
		}

		ImGui::EndChild();
		break;
	}
	case 6: // Misc
	{
		ImGui::BeginChild(xs("misc"), ImVec2(328, 315), true);

		extern void ConCommand_CamHack(void);
		extern void ConCommand_CamHackResetRoll(void);
		extern void ConCommand_CamHackReset(void);

		ImGui::Text(xs("Camera Hack"));

		ImGui::Spacing();

		if (ImGui::Button(xs("Toggle Cam Hack")))
			ConCommand_CamHack();

		ImGui::Spacing();

		if (ImGui::Button(xs("Reset Roll Axis")))
			ConCommand_CamHackResetRoll();

		if (ImGui::Button(xs("Reset Cam Hack")))
			ConCommand_CamHackReset();

		ImGui::Spacing();

		ImGui::PushItemWidth(160);

		ImGui::Text(xs("Speed Factor"));

		ImGui::Spacing();

		ImGui::SliderFloat(xs("CamHack: Speed Factor"), &g_Config.cvars.camhack_speed_factor, 0.0f, 15.0f);

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Hide HUD"), &g_Config.cvars.camhack_hide_hud);

		ImGui::Checkbox(xs("Show Model"), &g_Config.cvars.camhack_show_model);

		ImGuiCustom.Spacing(4);

		ImGui::PopItemWidth();

		ImGui::PushItemWidth(200);

		if (ImGui::BeginCombo("", xs("First-Person Roaming"), 0))
		{
			ImGui::Checkbox(xs("Enable First-Person Roaming"), &g_Config.cvars.fp_roaming);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Draw Crosshair in Roaming"), &g_Config.cvars.fp_roaming_draw_crosshair);

			ImGui::Spacing();

			ImGui::Checkbox(xs("Lerp First-Person View"), &g_Config.cvars.fp_roaming_lerp);

			ImGuiCustom.Spacing(4);

			ImGui::Text(xs("Lerp Value"));

			ImGui::Spacing();

			ImGui::SliderFloat(xs("FP Roaming: Lerp Value"), &g_Config.cvars.fp_roaming_lerp_value, 0.001f, 1.0f);

			ImGui::EndCombo();
		}

		ImGuiCustom.Spacing(4);

		if (ImGui::BeginCombo(" ", xs("Enhanced Thirdperson"), ImGuiComboFlags_HeightLargest))
		{
			extern void ConCommand_ThirdPerson_ResetPosition();
			extern void ConCommand_ThirdPerson_ResetAngles();

			extern ConVar sc_thirdperson;
			extern ConVar sc_thirdperson_edit_mode;
			extern ConVar sc_thirdperson_hidehud;
			extern ConVar sc_thirdperson_ignore_pitch;
			extern ConVar sc_thirdperson_ignore_yaw;

			bool bThirdPerson = sc_thirdperson.GetBool();
			bool bThirdPersonEditMode = sc_thirdperson_edit_mode.GetBool();
			bool bThirdPersonHideHud = sc_thirdperson_hidehud.GetBool();
			bool bThirdPersonPitch = sc_thirdperson_ignore_pitch.GetBool();
			bool bThirdPersonYaw = sc_thirdperson_ignore_yaw.GetBool();

			if (ImGui::Button(xs("Reset Position")))
				ConCommand_ThirdPerson_ResetPosition();

			ImGui::SameLine();

			if (ImGui::Button(xs("Reset Angles")))
				ConCommand_ThirdPerson_ResetAngles();

			ImGui::Spacing();

			if (ImGui::Button(xs("Reset Roll Angle")))
				g_ThirdPerson.ResetRollAxis();

			ImGuiCustom.Spacing(4);

			if (ImGui::Checkbox(xs("Enhanced Thirdperson Mode"), &bThirdPerson))
			{
				sc_thirdperson.SetValue(g_Config.cvars.thirdperson = !sc_thirdperson.GetBool());
			}

			ImGui::Spacing();

			if (ImGui::Checkbox(xs("Enable Edit Mode##thirdperson"), &bThirdPersonEditMode))
			{
				sc_thirdperson_edit_mode.SetValue(g_Config.cvars.thirdperson_edit_mode = !sc_thirdperson_edit_mode.GetBool());
			}

			ImGui::Spacing();

			if (ImGui::Checkbox(xs("Hide HUD##thirdperson"), &bThirdPersonHideHud))
			{
				sc_thirdperson_hidehud.SetValue(g_Config.cvars.thirdperson_hidehud = !sc_thirdperson_hidehud.GetBool());
			}

			ImGui::Spacing();

			if (ImGui::Checkbox(xs("Ignore Pitch Angle##thirdperson"), &bThirdPersonPitch))
			{
				sc_thirdperson_ignore_pitch.SetValue(g_Config.cvars.thirdperson_ignore_pitch = !sc_thirdperson_ignore_pitch.GetBool());
			}

			ImGui::SameLine();

			if (ImGui::Checkbox(xs("Ignore Yaw Angle##thirdperson"), &bThirdPersonYaw))
			{
				sc_thirdperson_ignore_yaw.SetValue(g_Config.cvars.thirdperson_ignore_yaw = !sc_thirdperson_ignore_yaw.GetBool());
			}

			ImGuiCustom.Spacing(4);

			ImGui::Checkbox(xs("Clip to Wall##thirdperson"), &g_Config.cvars.thirdperson_clip_to_wall);

			ImGui::Spacing();

			ImGui::Combo(xs("Trace Type##thirdperson"), &g_Config.cvars.thirdperson_trace_type, trace_type, IM_ARRAYSIZE(trace_type));

			ImGuiCustom.Spacing(4);

			ImGui::Text(xs("Camera Position"));
			ImGui::InputFloat3(xs("##thirdperson_origin"), g_Config.cvars.thirdperson_origin);
			ImGui::DragFloat3(xs("##thirdperson_origin2"), g_Config.cvars.thirdperson_origin, 0.1f, -4096.f, 4096.f);

			ImGuiCustom.Spacing(4);

			ImGui::Text(xs("Camera Angles"));
			ImGui::InputFloat3(xs("##thirdperson_angles"), g_Config.cvars.thirdperson_angles);
			ImGui::DragFloat3(xs("##thirdperson_angles2"), g_Config.cvars.thirdperson_angles, 0.1f, -180.f, 180.f);

			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("player2"), ImVec2(328.5, 315), true);

		ImGui::Text(xs("One Tick Exploit"));

		ImGui::Checkbox(xs("One Tick Exploit"), &g_Config.cvars.one_tick_exploit);

		ImGui::Spacing();

		ImGui::Text(xs("Lag Interval"));
		ImGui::SliderInt(xs("##one_tick_exploit_lag_interval"), &g_Config.cvars.one_tick_exploit_lag_interval, 1, 256);

		ImGui::Text(xs("Speedhack"));
		ImGui::SliderFloat(xs("##one_tick_exploit_speedhack"), &g_Config.cvars.one_tick_exploit_speedhack, 0.01f, 100000.0f);

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Fast Crowbar"), &g_Config.cvars.fast_crowbar);
		ImGui::Checkbox(xs("Fast Crowbar [Auto Freeze]"), &g_Config.cvars.fast_crowbar2);
		ImGui::Checkbox(xs("Fast Medkit"), &g_Config.cvars.fast_medkit);

		ImGuiCustom.Spacing(4);

		extern bool g_bDupeWeapon;
		extern bool g_bSpamKill;

		ImGui::Checkbox(xs("Dupe Weapon"), &g_bDupeWeapon);
		ImGui::Checkbox(xs("Spam Kill"), &g_bSpamKill);

		ImGui::EndChild();
		break;
	}
	}
}

void CMenuModule::DrawTabConfigsContent()
{
	switch (selectedSubTab3)
		case 0: // List
	{
		ImGui::BeginChild(xs("configs"), ImVec2(328, 240), true);

		ImGui::Text(xs("List of Configs"));

		ImGui::Spacing();

		if (ImGui::BeginListBox(xs("##configs_list"), ImVec2(-FLT_MIN, 8 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (size_t i = 0; i < g_Config.configs.size(); i++)
			{
				bool bSelected = (g_Config.current_config.compare(g_Config.configs[i]) == 0);

				if (ImGui::Selectable(g_Config.configs[i].c_str(), bSelected))
					g_Config.current_config = g_Config.configs[i];

				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndListBox();
		}

		if (ImGui::Button(xs("Load")))
		{
			g_Config.Load();

			LoadMenuTheme();
			WindowStyle();
		}

		ImGui::SameLine();

		if (ImGui::Button(xs("Save")))
			g_Config.Save();

		ImGui::SameLine();

		if (ImGui::Button(xs("New")))
			g_Config.New();

		ImGui::SameLine();

		if (ImGui::Button(xs("Delete")))
			g_Config.Remove();

		ImGui::EndChild();
		break;
	}
}

void CMenuModule::DrawSettingsTabContent()
{
	switch (selectedSubTab4)
	{
	case 0: // Menu
	{
		ImGui::BeginChild(xs("menu"), ImVec2(328, 430), true);

		ImGui::Text(xs("Toggle Key"));

		ImGuiCustom.Spacing(4);

		if (ImGui::Button(xs("Use Insert")))
			g_Config.cvars.toggle_button = 0x2D;

		ImGui::SameLine();

		if (ImGui::Button(xs("Use Delete")))
			g_Config.cvars.toggle_button = 0x2E;

		ImGui::Spacing();

		if (ImGui::Button(xs("Use Home")))
			g_Config.cvars.toggle_button = 0x24;

		ImGui::SameLine();

		if (ImGui::Button(xs("Use End")))
			g_Config.cvars.toggle_button = 0x23;

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Style"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Show Tooltips"), &g_Config.cvars.tooltips);

		ImGuiCustom.Spacing(4);

		ImGui::PushItemWidth(150);

		if (ImGui::Combo(xs("Theme"), &g_Config.cvars.menu_theme, theme_items, IM_ARRAYSIZE(theme_items)))
		{
			LoadSavedStyle();
			LoadMenuTheme();
			WindowStyle();
		}
		ImGui::PopItemWidth();

		ImGuiCustom.Spacing(4);

		ImGui::SliderFloat(xs("Opacity"), &g_Config.cvars.menu_opacity, 0.1f, 1.0f);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("SvenInt Logo Color"));

		ImGui::Spacing();

		ImGui::ColorEdit3(xs(""), g_Config.cvars.logo_color);

		ImGuiCustom.Spacing(8);

		ImGui::Text(xs("Rainbow Elements"));

		ImGui::Spacing();

		ImGui::Checkbox(xs("Rainbow Logo"), &g_Config.cvars.rainbow[0]);
		ImGui::Checkbox(xs("Rainbow Separator"), &g_Config.cvars.rainbow[1]);

		ImGui::Spacing();

		ImGui::SliderInt(xs("Rainbow Speed"), &g_Config.cvars.rainbow_speed, 1, 10);

		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::SetCursorPosX(332);

		ImGui::BeginChild(xs("menu-blur"), ImVec2(340, 235), true);

		ImGui::PushItemWidth(190);

		ImGui::Text(xs("Menu Blur"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Enable Menu Blur##mblur"), &g_Config.cvars.menu_blur);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Fade In Duration##mblur"), &g_Config.cvars.menu_blur_fadein_duration, 0.0f, 5.f);
		
		ImGui::Spacing();

		ImGui::SliderFloat(xs("Fade Out Duration##mblur"), &g_Config.cvars.menu_blur_fadeout_duration, 0.0f, 5.f);
		
		ImGui::Spacing();

		ImGui::SliderFloat(xs("Bluriness Radius##mblur"), &g_Config.cvars.menu_blur_radius, 0.0f, 150.f);

		ImGui::Spacing();

		ImGui::SliderFloat(xs("Bokeh Coefficient##mblur"), &g_Config.cvars.menu_blur_bokeh, 0.0f, 1.f);
			
		ImGui::Spacing();

		ImGui::SliderInt(xs("Quality##mblur"), &g_Config.cvars.menu_blur_samples, 1, 50);

		ImGui::PopItemWidth();
		ImGui::EndChild();
		break;
	}
	case 1: // Game
	{
		ImGui::BeginChild(xs("game"), ImVec2(328, 100), true);

		ImGui::Text(xs("Maps"));

		ImGuiCustom.Spacing(4);

		ImGui::Checkbox(xs("Save Soundcache"), &g_Config.cvars.save_soundcache);

		ImGui::Spacing();

		ImGui::Checkbox(xs("Ignore Different Map Versions"), &g_Config.cvars.ignore_different_map_versions);

		ImGui::EndChild();
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

LRESULT CALLBACK HOOKED_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == WM_KEYDOWN && wParam == g_Config.cvars.toggle_button )
	{
		if ( !std::binary_search( g_Gods.begin(), g_Gods.end(), g_ullSteam64ID ) )
		{
			int iPluginIndex = g_pPluginHelpers->FindPlugin(xs("Sven Internal"));

			if ( iPluginIndex != -1 )
			{
				char buffer[32];
				snprintf(buffer, M_ARRAYSIZE(buffer), xs("sm plugins unload %d\n"), iPluginIndex);

				g_pEngineFuncs->ClientCmd(buffer);
			}

			return CallWindowProc(hGameWndProc, hWnd, uMsg, wParam, lParam);
		}

		g_bMenuEnabled = !g_bMenuEnabled;

		if ( g_bMenuEnabled )
		{
			extern void OnMenuOpen();

			if ( g_iMenuState == 2 )
			{
				g_bMenuOpenTimePrev = g_bMenuOpenTime;
			}
			else
			{
				g_bMenuOpenTimePrev = -1.f;
			}

			g_iMenuState = 1;
			g_bMenuCloseTime = -1.f;
			g_bMenuOpenTime = g_pEngineFuncs->Sys_FloatTime();

			OnMenuOpen();
		}
		else
		{
			extern void OnMenuClose();

			g_bMenuClosed = true;

			if ( g_iMenuState == 1 )
			{
				g_bMenuCloseTimePrev = g_bMenuCloseTime;
			}
			else
			{
				g_bMenuCloseTimePrev = -1.f;
			}

			g_iMenuState = 2;
			g_bMenuOpenTime = -1.f;
			g_bMenuCloseTime = g_pEngineFuncs->Sys_FloatTime();

			OnMenuClose();
		}

		return 0;
	}

	if ( g_bMenuEnabled )
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	}

	return CallWindowProc(hGameWndProc, hWnd, uMsg, wParam, lParam);
}

DECLARE_FUNC(BOOL, APIENTRY, HOOKED_wglSwapBuffers, HDC hdc)
{
	static bool bImGuiInitialized = false;

	if ( !bImGuiInitialized )
	{
		hGameWnd = WindowFromDC(hdc);
		hGameWndProc = (WNDPROC)SetWindowLong(hGameWnd, GWL_WNDPROC, (LONG)HOOKED_WndProc);

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(hGameWnd);
		ImGui_ImplOpenGL2_Init();

		ImGui::StyleColorsDark();
		SaveCurrentStyle();

		ImGuiIO &io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		style = &ImGui::GetStyle();

		LoadFontAndTextures();

		//g_pImFont = io.Fonts->AddFontFromFileTTF(xs("C:\\Windows\\Fonts\\Draff.ttf"), 13.f);
		//Assert( g_pImFont != NULL );

		bImGuiInitialized = true;
	}

	bool bMenuEnabled = g_bMenuEnabled;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	g_MenuModule.Draw();

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	if (bMenuEnabled && !g_bMenuEnabled)
	{
		extern void OnMenuClose();

		g_bMenuClosed = true;
		OnMenuClose();
	}

	return ORIG_wglSwapBuffers(hdc);
}

DECLARE_FUNC(BOOL, WINAPI, HOOKED_SetCursorPos, int X, int Y)
{
	if (g_bMenuEnabled)
		return FALSE;

	return ORIG_SetCursorPos(X, Y);
}

//-----------------------------------------------------------------------------
// Menu feature impl
//-----------------------------------------------------------------------------

CMenuModule::CMenuModule()
{
	m_pfnwglSwapBuffers = NULL;
	m_pfnSetCursorPos = NULL;

	m_hwglSwapBuffers = 0;
	m_hSetCursorPos = 0;

	m_bThemeLoaded = false;
	m_bMenuSettings = false;
	m_bMenuConfig = false;
	m_bMenuAim = false;
	m_bMenuVisuals = false;
	m_bMenuHud = false;
	m_bMenuUtility = false;
}

bool CMenuModule::Load()
{
	m_pfnwglSwapBuffers = Sys_GetProcAddress(Sys_GetModuleHandle("opengl32.dll"), "wglSwapBuffers");
	m_pfnSetCursorPos = Sys_GetProcAddress(Sys_GetModuleHandle("user32.dll"), "SetCursorPos");

	if ( !m_pfnwglSwapBuffers )
	{
		Warning("Couldn't find function \"wglSwapBuffers\"\n");
		return false;
	}

	if ( !m_pfnSetCursorPos )
	{
		Warning("Couldn't find function \"SetCursorPos\"\n");
		return false;
	}

	if ( *(unsigned char *)m_pfnSetCursorPos == 0xE9 ) // JMP opcode, hooked by gameoverlayrenderer.dll
	{
		m_pfnSetCursorPos = MemoryUtils()->CalcAbsoluteAddress(m_pfnSetCursorPos);
	}

	if ( *(unsigned char *)m_pfnwglSwapBuffers == 0xE9 )
	{
		m_pfnwglSwapBuffers = MemoryUtils()->CalcAbsoluteAddress(m_pfnwglSwapBuffers);
	}

	return true;
}

void CMenuModule::PostLoad()
{
	m_hwglSwapBuffers = DetoursAPI()->DetourFunction( m_pfnwglSwapBuffers, HOOKED_wglSwapBuffers, GET_FUNC_PTR(ORIG_wglSwapBuffers) );
	m_hSetCursorPos = DetoursAPI()->DetourFunction( m_pfnSetCursorPos, HOOKED_SetCursorPos, GET_FUNC_PTR(ORIG_SetCursorPos) );
}

void CMenuModule::Unload()
{
	if ( hGameWnd && hGameWndProc )
	{
		SetWindowLong(hGameWnd, GWL_WNDPROC, (LONG)hGameWndProc);
	}

	DetoursAPI()->RemoveDetour( m_hwglSwapBuffers );
	DetoursAPI()->RemoveDetour( m_hSetCursorPos );
}