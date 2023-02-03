// Game Utils

#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#ifdef _WIN32
#pragma once
#endif

#ifndef M_SQR
#define M_SQR(x) (x * x)
#endif

struct screen_info_s
{
	int width;
	int height;
};

extern float *g_flNextCmdTime;
extern double *g_dbGameSpeed;
extern double *dbRealtime;

extern screen_info_s g_ScreenInfo;

int UTIL_SharedRandomLong(unsigned int seed, int low, int high);
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high);
const wchar_t *UTIL_CStringToWideCString(const char *pszString);
bool UTIL_WorldToScreen(float *pflOrigin, float *pflVecScreen);
void UTIL_ScreenToWorld(float *pflNDC, float *pflWorldOrigin);
void UTIL_SetAnglesSilent(float *angles, struct usercmd_s *cmd);
bool UTIL_IsFiring(struct usercmd_s *cmd);

void UTIL_SetGameSpeed(double dbSpeed);
void UTIL_SendPacket(bool bSend);

#endif