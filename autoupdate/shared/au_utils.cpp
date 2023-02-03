#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <random>

#include "au_utils.h"

#ifdef AU_CLIENT
#include <dbg.h>

std::vector<std::string> g_AUDevMsgQueue;
std::vector<AUColorMsg> g_AUColorMsgQueue;
#endif

#ifdef AU_SERVER
std::mutex print_mutex;
#endif

constexpr auto STRIP_CHARS = " \t\n";
constexpr auto STRIP_CHARS_LEN = sizeof(STRIP_CHARS) - 1;

CMutexAutoLock::CMutexAutoLock(std::mutex &mutex)
{
	m_mutex = &mutex;
	m_mutex->lock();
}

CMutexAutoLock::~CMutexAutoLock()
{
	m_mutex->unlock();
}

void AU_Printf(const char *pszMessageFormat, ...)
{
#ifdef AU_SERVER
	AUTO_LOCK( print_mutex );
#endif

	static char szFormattedMessage[2048] = { 0 };

	va_list args;
	va_start(args, pszMessageFormat);
	vsnprintf(szFormattedMessage, sizeof(szFormattedMessage) / sizeof(szFormattedMessage[0]), pszMessageFormat, args);
	va_end(args);

#ifdef AU_CLIENT
	g_AUDevMsgQueue.push_back( szFormattedMessage );
#else
	printf(szFormattedMessage);
#endif
}

#ifdef AU_CLIENT
void AU_Printf_Clr(const Color &clr, const char *pszMessageFormat, ...)
{
	static char szFormattedMessage[2048] = { 0 };

	va_list args;
	va_start(args, pszMessageFormat);
	vsnprintf(szFormattedMessage, sizeof(szFormattedMessage) / sizeof(szFormattedMessage[0]), pszMessageFormat, args);
	va_end(args);

	g_AUColorMsgQueue.push_back( { clr, szFormattedMessage } );
}
#endif

void AU_SetSeed(unsigned int seed)
{
	srand( seed );
}

int AU_RandomInt(int min, int max)
{
	return min + ( rand() % (max - min + 1) );
}

void EncryptData(unsigned char *data, unsigned char *key, int data_length, int key_length)
{
	int cycle = 0;

	for (int i = 0; i < data_length; i++)
	{
		if (cycle >= key_length)
			cycle = 0;

		data[i] ^= key[cycle++];
	}
}

// Same as EncryptData
void DecryptData(unsigned char *data, unsigned char *key, int data_length, int key_length)
{
	int cycle = 0;

	for (int i = 0; i < data_length; i++)
	{
		if (cycle >= key_length)
			cycle = 0;

		data[i] ^= key[cycle++];
	}
}

static inline bool ContainsChars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if ( chars[i] == ch )
			return true;
	}

	return false;
}

void ParseUtil_RemoveComment(char *str)
{
	char *comment = NULL;

	while (*str)
	{
		if (*str == '#')
		{
			if ( comment == NULL )
				comment = str;
		}

		str++;
	}

	if ( comment != NULL )
	{
		*comment = '\0';
	}
}

char *ParseUtil_LStrip(char *str)
{
	while (*str && ContainsChars(*str, STRIP_CHARS, STRIP_CHARS_LEN))
		++str;

	return str;
}

void ParseUtil_RStrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && ContainsChars(*end, STRIP_CHARS, STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

char *ParseUtil_Strip(char *str)
{
	char *result = ParseUtil_LStrip(str);
	ParseUtil_RStrip(result);

	return result;
}