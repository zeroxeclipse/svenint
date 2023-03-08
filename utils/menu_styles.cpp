#include "menu_styles.h"
#include <imgui_internal.h>

#include "../config.h"

#pragma warning(disable : 418)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)

ImGuiStyle* style = nullptr;
ImVec4* colors = 0;

//-----------------------------------------------------------------------------
// SvenInt
//-----------------------------------------------------------------------------

static void StyleColors_SvenInt()
{
	colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 230);
	colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
	colors[ImGuiCol_Button] = ImColor(56, 58, 74, 255);
	colors[ImGuiCol_ButtonActive] = ImColor(101, 164, 212, 255);
	colors[ImGuiCol_ButtonHovered] = ImColor(114, 187, 242, 255);
	colors[ImGuiCol_FrameBg] = ImColor(56, 58, 74, 255);
	colors[ImGuiCol_FrameBgActive] = ImColor(94, 140, 166, 255);
	colors[ImGuiCol_FrameBgHovered] = ImColor(114, 187, 242, 255);
	colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
	colors[ImGuiCol_ChildBg] = ImColor(33, 34, 45, 255);
	colors[ImGuiCol_CheckMark] = ImColor(255, 255, 255, 255);
	colors[ImGuiCol_SliderGrab] = ImColor(101, 164, 212, 255);
	colors[ImGuiCol_SliderGrabActive] = ImColor(101, 164, 212, 255);
	colors[ImGuiCol_Header] = ImColor(101, 164, 212, 255);
	colors[ImGuiCol_HeaderHovered] = ImColor(114, 187, 242, 255);
	colors[ImGuiCol_HeaderActive] = ImColor(101, 164, 212, 255);
	colors[ImGuiCol_ResizeGripActive] = ImColor(20, 50, 66, 255);
	colors[ImGuiCol_SeparatorActive] = ImColor(20, 50, 66, 255);
	colors[ImGuiCol_TitleBgActive] = ImColor(20, 50, 66, 255);
	colors[ImGuiCol_Separator] = ImColor(47, 96, 133, 255);
}

//-----------------------------------------------------------------------------
// Dark
//-----------------------------------------------------------------------------

static void StyleColors_Dark()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
}

//-----------------------------------------------------------------------------
// Classic
//-----------------------------------------------------------------------------

static void StyleColors_Classic()
{
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
	colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
}

//-----------------------------------------------------------------------------
// Light
//-----------------------------------------------------------------------------

static void StyleColors_Light()
{
	colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
}

//-----------------------------------------------------------------------------
// Berserk c:
//-----------------------------------------------------------------------------

static void StyleColors_Berserk()
{
	colors[ImGuiCol_Text] = ImVec4(1.20f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.03f, 0.03f, 0.90f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(1.00f, 0.00f, 0.00f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.33f, 0.33f, 0.33f, 0.69f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.33f, 0.33f, 0.33f, 0.80f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.00f, 0.00f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 0.62f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.79f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.00f, 0.00f, 0.60f);
	colors[ImGuiCol_Header] = ImVec4(0.33f, 0.33f, 0.33f, 0.45f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.00f, 0.00f, 0.80f);
	colors[ImGuiCol_Separator] = ImVec4(1.00f, 0.00f, 0.00f, 0.50f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
}

//-----------------------------------------------------------------------------
// Deep dark
//-----------------------------------------------------------------------------

static void StyleColors_DeepDark()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
}

//-----------------------------------------------------------------------------
// Carbon
//-----------------------------------------------------------------------------

static void StyleColors_Carbon()
{
	colors[ImGuiCol_Text] = ImVec4(1.00, 1.00, 1.00, 1.00);
	colors[ImGuiCol_WindowBg] = ImVec4(0.07, 0.07, 0.07, 1.00);
	colors[ImGuiCol_ChildBg] = ImVec4(0.07, 0.07, 0.07, 1.00);
	colors[ImGuiCol_PopupBg] = ImVec4(0.07, 0.07, 0.07, 1.00);
	colors[ImGuiCol_Border] = ImVec4(0.25, 0.25, 0.26, 0.54);
	colors[ImGuiCol_FrameBg] = ImVec4(0.12, 0.12, 0.12, 1.00);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25, 0.25, 0.26, 1.00);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25, 0.25, 0.26, 1.00);
	colors[ImGuiCol_CheckMark] = ImVec4(1.00, 1.00, 1.00, 1.00);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.21, 0.20, 0.20, 1.00);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.21, 0.20, 0.20, 1.00);
	colors[ImGuiCol_Button] = ImVec4(0.12, 0.12, 0.12, 1.00);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.21, 0.20, 0.20, 1.00);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.41, 0.41, 0.41, 1.00);
	colors[ImGuiCol_Header] = ImVec4(0.12, 0.12, 0.12, 1.00);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.20, 0.20, 0.20, 1.00);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.47, 0.47, 0.47, 1.00);
	colors[ImGuiCol_Separator] = ImVec4(0.25, 0.25, 0.26, 0.54);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.12, 0.12, 0.12, 1.00);
}

//-----------------------------------------------------------------------------
// Corporate grey
//-----------------------------------------------------------------------------

static void StyleColors_CorporateGrey()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
}

//-----------------------------------------------------------------------------
// Grey 
//-----------------------------------------------------------------------------

static void StyleColors_Grey()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
}

//-----------------------------------------------------------------------------
// Gold & Black
//-----------------------------------------------------------------------------

static void StyleColors_GoldBlack()
{
	colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.93f, 0.65f, 0.14f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
}

//-----------------------------------------------------------------------------
// Monochrome
//-----------------------------------------------------------------------------

static void StyleColors_Monochrome()
{
	colors[ImGuiCol_Text] = ImVec4(0.00, 1.00, 1.00, 1.00);
	colors[ImGuiCol_WindowBg] = ImVec4(0.00, 0.00, 0.00, 1.00);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00, 0.00, 0.00, 0.00);
	colors[ImGuiCol_Border] = ImVec4(0.00, 1.00, 1.00, 0.65);
	colors[ImGuiCol_FrameBg] = ImVec4(0.44, 0.80, 0.80, 0.18);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44, 0.80, 0.80, 0.27);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.44, 0.81, 0.86, 0.66);
	colors[ImGuiCol_CheckMark] = ImVec4(0.00, 1.00, 1.00, 0.68);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.00, 1.00, 1.00, 0.36);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00, 1.00, 1.00, 0.76);
	colors[ImGuiCol_Button] = ImVec4(0.00, 0.65, 0.65, 0.46);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.01, 1.00, 1.00, 0.43);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.00, 1.00, 1.00, 0.62);
	colors[ImGuiCol_Header] = ImVec4(0.00, 1.00, 1.00, 0.33);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00, 1.00, 1.00, 0.42);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.00, 1.00, 1.00, 0.54);
	colors[ImGuiCol_Separator] = ImVec4(0.00, 1.00, 1.00, 0.65);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
}

//-----------------------------------------------------------------------------
// Dark light
//-----------------------------------------------------------------------------

static void StyleColors_DarkLight()
{
	colors[ImGuiCol_Text] = ImVec4(0.90, 0.90, 0.90, 1.00);
	colors[ImGuiCol_WindowBg] = ImVec4(0.00, 0.00, 0.00, 1.00);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00, 0.00, 0.00, 1.00);
	colors[ImGuiCol_PopupBg] = ImVec4(0.00, 0.00, 0.00, 1.00);
	colors[ImGuiCol_Border] = ImVec4(0.82, 0.77, 0.78, 1.00);
	colors[ImGuiCol_FrameBg] = ImVec4(1.00, 1.00, 1.00, 0.28);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.68, 0.68, 0.68, 0.67);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.79, 0.73, 0.73, 0.62);
	colors[ImGuiCol_CheckMark] = ImVec4(0.99, 0.99, 0.99, 0.52);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00, 1.00, 1.00, 0.42);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.76, 0.76, 0.76, 1.00);
	colors[ImGuiCol_Button] = ImVec4(0.51, 0.51, 0.51, 0.60);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.45, 0.45, 0.45, 1.00);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.67, 0.67, 0.67, 1.00);
	colors[ImGuiCol_Header] = ImVec4(0.72, 0.72, 0.72, 0.54);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.92, 0.92, 0.95, 0.77);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.82, 0.82, 0.82, 0.80);
	colors[ImGuiCol_Separator] = ImVec4(0.73, 0.73, 0.73, 1.00);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.74, 0.74, 0.74, 1.00);
}

//-----------------------------------------------------------------------------
// Soft dark
//-----------------------------------------------------------------------------

static void StyleColors_SoftDark()
{
	colors[ImGuiCol_Text] = ImVec4(0.95, 0.96, 0.98, 1.00);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11, 0.15, 0.17, 1.00);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15, 0.18, 0.22, 1.00);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08, 0.08, 0.08, 0.94);
	colors[ImGuiCol_Border] = ImVec4(0.43, 0.43, 0.50, 0.50);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20, 0.25, 0.29, 1.00);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12, 0.20, 0.28, 1.00);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.09, 0.12, 0.14, 1.00);
	colors[ImGuiCol_CheckMark] = ImVec4(0.28, 0.56, 1.00, 1.00);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.28, 0.56, 1.00, 1.00);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37, 0.61, 1.00, 1.00);
	colors[ImGuiCol_Button] = ImVec4(0.20, 0.25, 0.29, 1.00);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28, 0.56, 1.00, 1.00);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06, 0.53, 0.98, 1.00);
	colors[ImGuiCol_Header] = ImVec4(0.20, 0.25, 0.29, 0.55);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26, 0.59, 0.98, 0.80);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26, 0.59, 0.98, 1.00);
	colors[ImGuiCol_Separator] = ImVec4(0.43, 0.43, 0.50, 0.50);
}

//-----------------------------------------------------------------------------
// Pink
//-----------------------------------------------------------------------------

static void StyleColors_Pink()
{
	colors[ImGuiCol_Text] = ImVec4(0.00, 0.00, 0.00, 1.00);
	colors[ImGuiCol_WindowBg] = ImVec4(1.00, 1.00, 1.00, 0.71);
	colors[ImGuiCol_ChildBg] = ImVec4(0.92, 0.92, 0.92, 0.00);
	colors[ImGuiCol_PopupBg] = ImVec4(1.00, 1.00, 1.00, 0.94);
	colors[ImGuiCol_Border] = ImVec4(1.00, 1.00, 1.00, 0.50);
	colors[ImGuiCol_FrameBg] = ImVec4(0.77, 0.49, 0.66, 0.54);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00, 1.00, 1.00, 0.40);
	colors[ImGuiCol_FrameBgActive] = ImVec4(1.00, 1.00, 1.00, 0.67);
	colors[ImGuiCol_CheckMark] = ImVec4(1.00, 1.00, 1.00, 1.00);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.71, 0.39, 0.39, 1.00);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.76, 0.51, 0.66, 0.46);
	colors[ImGuiCol_Button] = ImVec4(0.78, 0.28, 0.58, 0.54);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.77, 0.52, 0.67, 0.54);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20, 0.20, 0.20, 0.50);
	colors[ImGuiCol_Header] = ImVec4(0.78, 0.28, 0.58, 0.54);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.78, 0.28, 0.58, 0.25);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.79, 0.04, 0.48, 0.63);
	colors[ImGuiCol_Separator] = ImVec4(1.00, 1.00, 1.00, 0.50);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.79, 0.17, 0.54, 0.77);
}

//-----------------------------------------------------------------------------
// Half-Life 1
//-----------------------------------------------------------------------------

static void StyleColors_HalfLife()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Button] = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Separator] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
}

//-----------------------------------------------------------------------------
// Sven-Cope
//-----------------------------------------------------------------------------

static void StyleColors_SvenCope()
{
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.47f, 0.49f, 0.59f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.22f, 0.23f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.23f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.86f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.49f, 0.59f, 0.50f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.31f, 0.37f, 0.50f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.21f, 0.27f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.46f, 0.53f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.21f, 0.22f, 0.50f);
	colors[ImGuiCol_Header] = ImVec4(0.47f, 0.49f, 0.59f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.23f, 0.29f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Separator] = ImVec4(0.47f, 0.49f, 0.59f, 0.50f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
}

void StyleColors_Custom()
{
	colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.WindowBgU32);
	colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.BorderU32);
	colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.ButtonU32);
	colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.ButtonActiveU32);
	colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.ButtonHoveredU32);
	colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.FrameBgU32);
	colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.FrameBgActiveU32);
	colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.FrameBgHoveredU32);
	colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.TextU32);
	colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.ChildBgU32);
	colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.CheckMarkU32);
	colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.SliderGrabU32);
	colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.SliderGrabActiveU32);
	colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.HeaderU32);
	colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.HeaderHoveredU32);
	colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.HeaderActiveU32);
	colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.ResizeGripActiveU32);
	colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.SeparatorActiveU32);
	colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.TitleBgActiveU32);
	colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(g_Config.cvars.SeparatorU32);
}

//-----------------------------------------------------------------------------
// Theme Functions
//-----------------------------------------------------------------------------

void InitImGuiStyles()
{
	style = &ImGui::GetStyle();
	colors = style->Colors;
}

void LoadMenuTheme()
{
	switch (g_Config.cvars.menu_theme)
	{
	case 0:
		StyleColors_SvenInt();
		break;

	case 1:
		ImGui::StyleColorsDark();
		break;

	case 2:
		ImGui::StyleColorsLight();
		break;

	case 3:
		ImGui::StyleColorsClassic();
		break;

	case 4:
		StyleColors_Berserk();
		break;

	case 5:
		StyleColors_DeepDark();
		break;

	case 6:
		StyleColors_Carbon();
		break;

	case 7:
		StyleColors_CorporateGrey();
		break;

	case 8:
		StyleColors_Grey();
		break;

	case 9:
		StyleColors_DarkLight();
		break;

	case 10:
		StyleColors_SoftDark();
		break;

	case 11:
		StyleColors_GoldBlack();
		break;

	case 12:
		StyleColors_Monochrome();
		break;

	case 13:
		StyleColors_Pink();
		break;

	case 14:
		StyleColors_HalfLife();
		break;

	case 15:
		StyleColors_SvenCope();
		break;

	case 16:
		StyleColors_Custom();
		break;
	}
}

//-----------------------------------------------------------------------------
// Save and load one initial style
//-----------------------------------------------------------------------------

static ImGuiStyle s_Style;

void SaveCurrentStyle() // call on initialization after StyleColorsDarkTheme
{
	memcpy(&s_Style, &ImGui::GetStyle(), sizeof(ImGuiStyle));
}

void LoadSavedStyle()
{
	memcpy((unsigned char*)&ImGui::GetStyle() + sizeof(ImGuiStyle::Alpha), // Skip opacity
		(unsigned char*)&s_Style + sizeof(ImGuiStyle::Alpha),
		sizeof(ImGuiStyle) - sizeof(ImGuiStyle::Alpha));
}

