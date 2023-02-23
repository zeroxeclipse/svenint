#ifndef PATTERNS_H
#define PATTERNS_H

#ifdef _WIN32
#pragma once
#endif

#include <memutils/patterns.h>

namespace Patterns
{
	namespace Hardware
	{
		EXTERN_PATTERN(flNextCmdTime);

		EXTERN_PATTERN(Netchan_CanPacket);
		EXTERN_PATTERN(Netchan_Transmit);

		EXTERN_PATTERN(Sys_InitializeGameDLL);

		EXTERN_PATTERN(GL_Bind);

		EXTERN_PATTERN(SCR_UpdateScreen);
		EXTERN_PATTERN(SCR_BeginLoadingPlaque);
		EXTERN_PATTERN(SCR_EndLoadingPlaque);

		EXTERN_PATTERN(V_RenderView);
		EXTERN_PATTERN(R_RenderScene);
		EXTERN_PATTERN(R_RenderScene_HOOKED);
		EXTERN_PATTERN(R_SetupFrame);
		EXTERN_PATTERN(R_LoadSkyboxInt);

		EXTERN_PATTERN(CL_ClearState);
		EXTERN_PATTERN(CL_ParseConsistencyInfo);
		EXTERN_PATTERN(CL_BatchResourceRequest);
		EXTERN_PATTERN(CL_PlayerFlashlight);

		EXTERN_PATTERN(ClientDLL_HudRedraw);

		EXTERN_PATTERN(IsSafeFileToDownload);

		EXTERN_PATTERN(CRC_MapFile);
		EXTERN_PATTERN(ResourceList_Offset);
		EXTERN_PATTERN(UserInfo_Offset);
		EXTERN_PATTERN(clc_buffer);
	}

	namespace Client
	{
		EXTERN_PATTERN(IN_Move);

		EXTERN_PATTERN(CVotePopup__MsgFunc_VoteMenu);
		EXTERN_PATTERN(READ_BYTE);
		EXTERN_PATTERN(READ_STRING);
		
		EXTERN_PATTERN(CHudBaseTextBlock__Print);

		EXTERN_PATTERN(CVoiceBanMgr__SaveState);
		EXTERN_PATTERN(CVoiceBanMgr__SetPlayerBan);
		EXTERN_PATTERN(CVoiceBanMgr__InternalFindPlayerSquelch);

		EXTERN_PATTERN(CVoiceStatus__IsPlayerBlocked);
		EXTERN_PATTERN(CVoiceStatus__SetPlayerBlockedState);
		EXTERN_PATTERN(CVoiceStatus__UpdateServerState);

		EXTERN_PATTERN(CClient_SoundEngine__PlayFMODSound);
		EXTERN_PATTERN(CClient_SoundEngine__Play2DSound);
		EXTERN_PATTERN(CClient_SoundEngine__LoadSoundList);

		EXTERN_PATTERN(EV_HLDM_PlayTextureSound);

		EXTERN_PATTERN(HACK_GetPlayerUniqueID);

		EXTERN_PATTERN(GetClientColor);

		EXTERN_PATTERN(ScaleColors);
		EXTERN_PATTERN(ScaleColors_RGBA);

		EXTERN_PATTERN(CHud__Think);

		EXTERN_PATTERN(V_PunchAxis);

		EXTERN_PATTERN(WeaponsResource__GetFirstPos);
	}

	namespace Server
	{
		EXTERN_PATTERN(gpGlobals);
	}

	namespace GameOverlay
	{
		EXTERN_PATTERN(SetCursorPos_Hook);
		EXTERN_PATTERN(ValveUnhookFunc);
	}
}

#endif // PATTERNS_H