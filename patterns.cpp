#include "patterns.h"

namespace Patterns
{
	namespace Hardware
	{
		DEFINE_PATTERN(flNextCmdTime, "D9 1D ? ? ? ? 75 0A A1");

		DEFINE_PATTERN(Netchan_CanPacket, "D9 05 ? ? ? ? D9 EE DA E9 DF E0 F6 C4 44 8B 44 24 04");
		DEFINE_PATTERN(Netchan_Transmit, "B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 84 24 ? ? ? ? 53 55");

		DEFINE_PATTERN(Sys_InitializeGameDLL, "E8 ? ? ? ? 33 C0 83 3D ? ? ? ? ? 0F 9F C0 50 E8 ? ? ? ? 83 C4");

		DEFINE_PATTERN(GL_Bind, "8B 44 24 04 39 05 ? ? ? ? 74 11 50 68");

		DEFINE_PATTERN(SCR_UpdateScreen, "83 EC 44 A1 ? ? ? ? 33 C4 89 44 24 40 83 3D ? ? ? ? ? 0F 85 ? ? ? ? C7");
		DEFINE_PATTERN(SCR_BeginLoadingPlaque, "6A ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 83 F8");
		DEFINE_PATTERN(SCR_EndLoadingPlaque, "C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? C7 05");

		DEFINE_PATTERN(V_RenderView, "81 EC ? ? 00 00 A1 ? ? ? ? 33 C4 89 84 24 ? ? 00 00 D9 EE D9 15");
		DEFINE_PATTERN(R_RenderScene, "83 EC ? E8 ? ? ? ? 85 C0 74");
		DEFINE_PATTERN(R_RenderScene_HOOKED, "E9 ? ? ? ? ? ? ? 85 C0 74");
		DEFINE_PATTERN(R_SetupFrame, "83 EC 24 A1 ? ? ? ? 33 C4 89 44 24 20 33 C0 83 3D ? ? ? ? 01 0F 9F C0 50 E8 ? ? ? ? E8");
		DEFINE_PATTERN(R_LoadSkyboxInt, "81 EC ? ? 00 00 A1 ? ? ? ? 33 C4 89 84 24 ? ? 00 00 53 8B 9C 24 ? ? 00 00 55 56 57 89 5C 24 24");

		DEFINE_PATTERN(CL_ClearState, "E8 ? ? ? ? 85 C0 75 ? FF 74 24 04");
		DEFINE_PATTERN(CL_ParseConsistencyInfo, "83 EC ? A1 ? ? ? ? 33 C4 89 44 24 24 D9 ? ? ? ? ? D9 ? DA");
		DEFINE_PATTERN(CL_BatchResourceRequest, "55 8B EC 83 E4 ? B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 1C 01 04 00");
		DEFINE_PATTERN(CL_PlayerFlashlight, "81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 BC 00 00 00 A1");

		DEFINE_PATTERN(ClientDLL_HudRedraw, "E8 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0");

		DEFINE_PATTERN(IsSafeFileToDownload, "81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 04 01 00 00 8B 84 24 0C 01 00 00 56 85 C0");
		
		DEFINE_PATTERN(CRC_MapFile, "81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 84 04 00 00");
		DEFINE_PATTERN(ResourceList_Offset, "A1 ? ? ? ? 85 C0 74 ? 83 3D ? ? ? ? ? 74 ? 89 86 84 00 00 00");
		DEFINE_PATTERN(UserInfo_Offset, "8D B8 ? ? ? ? 75 20 8B 0D");
		DEFINE_PATTERN(clc_buffer, "68 ? ? ? ? 85 F6 74 ? 0F AE E8");
	}

	namespace Client
	{
		DEFINE_PATTERN(IN_Move, "83 3D ? ? ? ? 00 75 21 83 3D ? ? ? ? 00 74 18 FF 74 24 08 F3 0F 10 44 24 08 51 F3 0F 11 04 24");

		DEFINE_PATTERN(CVotePopup__MsgFunc_VoteMenu, "56 57 FF 74 24 10 8B F1 FF 74 24 18 E8 ? ? ? ? E8 ? ? ? ? 89 86 ? ? 00 00 E8");
		DEFINE_PATTERN(READ_BYTE, "8B 0D ? ? ? ? 8D 51 01 3B 15 ? ? ? ? 7E 0E C7 05 ? ? ? ? 01 00 00 00 83 C8 FF C3 A1 ? ? ? ? 89 15 ? ? ? ? 0F B6 04 08 C3");
		DEFINE_PATTERN(READ_STRING, "8B 15 ? ? ? ? 33 C0 53 55 8B 2D ? ? ? ? 56 57 8B 3D ? ? ? ? C6 05 ? ? ? ? 00 90");
		
		DEFINE_PATTERN(CHudBaseTextBlock__Print, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B D9 8B 0D");

		DEFINE_PATTERN(CVoiceBanMgr__SaveState, "81 EC ? ? 00 00 A1 ? ? ? ? 33 C4 89 84 24 ? ? 00 00 8B 84 24 ? ? 00 00 53 57 FF 35 ? ? ? ? 8B F9 50 8D 44 24 14");
		DEFINE_PATTERN(CVoiceBanMgr__SetPlayerBan, "56 FF 74 24 08 8B F1 E8 ? ? ? ? 80 7C 24 0C 00 74 13 85 C0 75 32 FF 74 24 08 8B CE E8");
		DEFINE_PATTERN(CVoiceBanMgr__InternalFindPlayerSquelch, "53 55 8B 6C 24 0C 56 57 0F 10 4D 00 0F 28 C1 66 0F 73 D8 08 66 0F FC C8 0F 10 C1 66 0F 73 D8 04");

		DEFINE_PATTERN(CVoiceStatus__IsPlayerBlocked, "83 EC 14 A1 ? ? ? ? 33 C4 89 44 24 10 56 8D 44 24 04 8B F1 50 FF 74 24 20 FF 15");
		DEFINE_PATTERN(CVoiceStatus__SetPlayerBlockedState, "81 EC ? ? 00 00 A1 ? ? ? ? 33 C4 89 84 24 ? ? 00 00 53 68 ? ? ? ? 8B D9 FF 15 ? ? ? ? D9 5C 24 08");
		DEFINE_PATTERN(CVoiceStatus__UpdateServerState, "81 EC ? ? 00 00 A1 ? ? ? ? 33 C4 89 84 24 ? ? 00 00 53 8B D9 89 5C 24 08");

		DEFINE_PATTERN(CClient_SoundEngine__PlayFMODSound, "55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 B4 00 00 00 8B 45 18");
		DEFINE_PATTERN(CClient_SoundEngine__Play2DSound, "83 EC ? F3 0F 10 44 24 14");
		DEFINE_PATTERN(CClient_SoundEngine__LoadSoundList, "81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 A8 08 00 00");

		DEFINE_PATTERN(EV_HLDM_PlayTextureSound, "81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 AC 00 00 00 8B 84 24 B8 00 00 00");

		DEFINE_PATTERN(HACK_GetPlayerUniqueID, "FF 74 24 08 FF 74 24 08 FF 15 ? ? ? ? 83 C4 08 85 C0 0F 95 C0 C3");

		DEFINE_PATTERN(GetClientColor, "8B 4C 24 04 85 C9 7E 35 6B C1 ? 0F BF 80 ? ? ? ? 48 83 F8 03");

		DEFINE_PATTERN(ScaleColors, "66 0F 6E 4C 24 10 8B 4C 24 04 0F 5B C9");
		DEFINE_PATTERN(ScaleColors_RGBA, "8B 4C 24 04 0F B6 41 03 66 0F 6E C8 0F B6 01 0F 5B C9");

		DEFINE_PATTERN(CHud__Think, "83 EC 08 53 56 57 8B F9 E8 ? ? ? ? 8B 37 D9 5C 24");

		DEFINE_PATTERN(V_PunchAxis, "8B 44 24 04 F3 0F 10 44 24 08 F3 0F 11 04 85");

		DEFINE_PATTERN(WeaponsResource__GetFirstPos, "6B 54 24 04 68 56 57 33 F6 8B F9 81 C2 ? ? ? ? 8B 02 85 C0 74");
	}

	namespace Server
	{
		DEFINE_PATTERN(gpGlobals, "A1 ? ? ? ? 8B 80 98 00 00 00 03 86 80 00 00 00 50 68 ? ? ? ? 6A ? FF 15 ? ? ? ? 83 C4");
	}

	namespace GameOverlay
	{
		DEFINE_PATTERN(SetCursorPos_Hook, "55 8B EC 80 3D ? ? ? ? 00 74 19 8B 45 08 A3 ? ? ? ? 8B 45 0C A3 ? ? ? ? B8 01");
		DEFINE_PATTERN(ValveUnhookFunc, "55 8B EC C7");
		//DEFINE_PATTERN(ValveUnhookFunc, "55 8B ? 64 ? ? ? ? ? 6A ? 68 ? ? ? ? 50 64 ? ? ? ? ? ? 81 ? ? ? ? ? 56 8B ? ? 85");
	}
}