#ifndef MENU_STYLES_H
#define MENU_STYLES_H

#ifdef _WIN32
#pragma once
#endif

#include <imgui.h>

void LoadMenuTheme();

void SaveCurrentStyle();
void LoadSavedStyle();

void InitImGuiStyles();
void ConvertToU32();
void RefreshCustomStyle();

#endif // MENU_STYLES_H