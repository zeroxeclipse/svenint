#ifndef SILLY_PATCHES_H
#define SILLY_PATCHES_H

#ifdef _WIN32
#pragma once
#endif

void EnableTertiaryAttackGlitch();
void DisableTertiaryAttackGlitch();
bool IsTertiaryAttackGlitchPatched();
bool IsTertiaryAttackGlitchInit();

void EnableTertiaryAttackGlitch_Server();
void DisableTertiaryAttackGlitch_Server();
bool IsTertiaryAttackGlitchPatched_Server();
bool IsTertiaryAttackGlitchInit_Server();

#endif // SILLY_PATCHES_H