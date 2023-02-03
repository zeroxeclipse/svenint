#ifndef __AUTOUPDATE_UTILS__H
#define __AUTOUPDATE_UTILS__H

#ifdef _WIN32
#pragma once
#endif

#include <mutex>

#ifdef AU_CLIENT
#include <vector>
#include <string>

#include <color.h>

struct AUColorMsg
{
	Color clr;
	std::string msg;
};

extern std::vector<std::string> g_AUDevMsgQueue;
extern std::vector<AUColorMsg> g_AUColorMsgQueue;
#endif

#define AUTO_LOCK(mutex) CMutexAutoLock __##mutex##__autolock(mutex)

class CMutexAutoLock
{
public:
	CMutexAutoLock(std::mutex &mutex);
	~CMutexAutoLock();

private:
	std::mutex *m_mutex;
};

void AU_Printf(const char *pszMessageFormat, ...);

#ifdef AU_CLIENT
void AU_Printf_Clr(const Color &clr, const char *pszMessageFormat, ...);
#endif

void AU_SetSeed(unsigned int seed);
int AU_RandomInt(int min, int max);

void EncryptData(unsigned char *data, unsigned char *key, int data_length, int key_length);
void DecryptData(unsigned char *data, unsigned char *key, int data_length, int key_length);

void ParseUtil_RemoveComment(char *str);
char *ParseUtil_LStrip(char *str);
void ParseUtil_RStrip(char *str);
char *ParseUtil_Strip(char *str);

#endif