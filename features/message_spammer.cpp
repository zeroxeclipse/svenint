// Message Spammer

#include <regex>

#include <hl_sdk/engine/APIProxy.h>

#include <convar.h>
#include <dbg.h>

#include "message_spammer.h"

#include "../game/utils.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

static int ms_contains_chars(char ch, const char *chars, size_t length);

static char *ms_lstrip(char *str);
static void ms_rstrip(char *str);
static char *ms_strip(char *str);
static void ms_strip_extension(const char **pszString);

static void ms_remove_comment(char *str);

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CMessageSpammer g_MessageSpammer;

//-----------------------------------------------------------------------------
// ConCommands & ConVars
//-----------------------------------------------------------------------------

CON_COMMAND_EXTERN(sc_ms_add, ConCommand_AddSpamTask, "Add spam task from file ../sven_internal/message_spammer/<taskname>.txt")
{
	if (args.ArgC() >= 2)
	{
		const char *pszTask = args[1];

		ms_strip_extension(&pszTask);
		g_MessageSpammer.AddTask(pszTask);
	}
	else
	{
		ConMsg("Usage:  sc_ms_add <taskname>\n");
	}
}

CON_COMMAND_EXTERN(sc_ms_remove, ConCommand_RemoveSpamTask, "Remove spam task by name")
{
	if (args.ArgC() >= 2)
	{
		const char *pszTask = args[1];

		ms_strip_extension(&pszTask);
		g_MessageSpammer.RemoveTask(pszTask);
	}
	else
	{
		ConMsg("Usage:  sc_ms_remove <filename>\n");
	}
}

CON_COMMAND_EXTERN(sc_ms_reload, ConCommand_ReloadSpamTask, " Reload spam task by name")
{
	if (args.ArgC() >= 2)
	{
		const char *pszTask = args[1];

		ms_strip_extension(&pszTask);
		g_MessageSpammer.ReloadTask(pszTask);
	}
	else
	{
		ConMsg("Usage:  sc_ms_reload <filename>\n");
	}
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_ms_keywords, ConCommand_PrintSpamKeyWords, "sc_ms_keywords - Prints all keywords")
{
	Msg("[Message Spammer] Keywords:\n");
	Msg("loop | must be defined at the beginning\n");
	Msg("send [message] | send a given message to the game chat\n");
	Msg("sleep [delay] | pause a running task for a given delay\n");
}

CON_COMMAND_EXTERN_NO_WRAPPER(sc_ms_print, ConCommand_PrintSpamTasks, "sc_ms_print - Print all spam tasks")
{
	g_MessageSpammer.PrintTasks();
}

ConVar sc_ms_debug("sc_ms_debug", "0", FCVAR_CLIENTDLL, "sc_ms_debug [0/1] - Enable debugging for Message Spammer");

//-----------------------------------------------------------------------------
// CMessageSpammer
//-----------------------------------------------------------------------------

CMessageSpammer::CMessageSpammer()
{
}

CMessageSpammer::~CMessageSpammer()
{
	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		CSpamTask *pTask = m_tasks[i];
		delete pTask;
	}

	m_tasks.clear();
}

void CMessageSpammer::CreateMove(float frametime, struct usercmd_s *cmd, int active)
{
	RunTasks();
}

void CMessageSpammer::RunTasks()
{
	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		CSpamTask *pTask = m_tasks[i];

		if (!pTask->Run())
		{
			m_tasks.erase(m_tasks.begin() + i);
			delete pTask;
			--i;
		}
	}
}

void CMessageSpammer::PrintTasks()
{
	Msg("[Message Spammer] Spam tasks list:\n");

	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		CSpamTask *pTask = m_tasks[i];

		Msg("%d: %s (loop: %d, waiting: %d)\n", i, pTask->GetName(), pTask->IsLooped(), pTask->IsWaiting());
	}
}

bool CMessageSpammer::AddTask(const char *pszTaskName)
{
	if (GetTask(pszTaskName))
		return ReloadTask(pszTaskName);

	static char szBuffer[512];

	sprintf_s(szBuffer, sizeof(szBuffer), "sven_internal/message_spammer/%s.txt", pszTaskName);

	FILE *file = fopen(szBuffer, "r");

	if (file)
	{
		int nLine = 0;

		bool bLoopVarFound = false;
		bool bParsingOperators = false;
		bool bDebug = sc_ms_debug.GetBool();

		std::regex regex_loop("^loop[\n]{0,1}$");
		std::regex regex_send("^send (.+)[\n]{0,1}$");
		std::regex regex_sleep("^sleep ([0-9.]+)[\n]{0,1}$");

		CSpamTask *pTask = new CSpamTask(pszTaskName);

		if (bDebug)
			Msg("< Parsing task: %s >\n", pszTaskName);

		while (fgets(szBuffer, sizeof(szBuffer), file))
		{
			std::cmatch match;
			nLine++;

			char *buffer = ms_lstrip(szBuffer);
			ms_remove_comment(buffer);
			ms_rstrip(buffer);

			if (!*buffer) // empty
				continue;

			if (!bParsingOperators && !bLoopVarFound)
			{
				if (std::regex_search(buffer, match, regex_loop))
				{
					if (bDebug)
						Msg("[%d] Found action | loop\n", nLine);

					bLoopVarFound = true;
					continue;
				}
			}

			if (std::regex_search(buffer, match, regex_send))
			{
				if (bDebug)
					Msg("[%d] Found action | send %s\n", nLine, match[1].str().c_str());

				CSpamOperatorSend *pOperator = new CSpamOperatorSend();

				pOperator->SetOperand(match[1].str().c_str());
				pTask->AddOperator(reinterpret_cast<ISpamOperator *>(pOperator));

				bParsingOperators = true;
			}
			else if (std::regex_search(buffer, match, regex_sleep))
			{
				if (bDebug)
					Msg("[%d] Found action | sleep %s\n", nLine, match[1].str().c_str());

				CSpamOperatorSleep *pOperator = new CSpamOperatorSleep();

				pOperator->SetOperand(strtof(match[1].str().c_str(), NULL));
				pTask->AddOperator(reinterpret_cast<ISpamOperator *>(pOperator));

				bParsingOperators = true;
			}
			else
			{
				Msg("[MS] Unrecognized expression '%s' at line %d\n", buffer, nLine);
			}
		}

		if (bDebug)
			Msg("< Parsing finished >\n");

		pTask->SetLoop(bLoopVarFound);
		m_tasks.push_back(pTask);

		Msg("[Message Spammer] Spam task %s successfully parsed\n", pszTaskName);

		fclose(file);
		return true;
	}
	else
	{
		Msg("[Message Spammer] Failed to open file called %s.txt\n", pszTaskName);
	}

	return false;
}

bool CMessageSpammer::ReloadTask(const char *pszTaskName)
{
	bool bReloaded = RemoveTask(pszTaskName) && AddTask(pszTaskName);

	if (bReloaded)
		Msg("[Message Spammer] Spam task %s has been reloaded\n", pszTaskName);
	else
		Msg("[Message Spammer] Failed to reload spam task %s\n", pszTaskName);

	return bReloaded;
}

bool CMessageSpammer::RemoveTask(const char *pszTaskName)
{
	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		CSpamTask *pTask = m_tasks[i];
		const char *pszName = pTask->GetName();

		if (pszName && !strcmp(pszName, pszTaskName))
		{
			m_tasks.erase(m_tasks.begin() + i);
			delete pTask;

			Msg("[Message Spammer] Spam task %s has been removed\n", pszTaskName);

			return true;
		}
	}

	return false;
}

CSpamTask *CMessageSpammer::GetTask(const char *pszTaskName)
{
	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		CSpamTask *pTask = m_tasks[i];
		const char *pszName = pTask->GetName();

		if (pszName && !strcmp(pszName, pszTaskName))
			return pTask;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// CSpamTask
//-----------------------------------------------------------------------------

CSpamTask::CSpamTask(const char *pszName)
{
	m_pszName = (const char *)strdup(pszName);
	m_bLoop = false;
	m_iOperatorBegin = 0;
}

CSpamTask::~CSpamTask()
{
	if (m_pszName)
		free((void *)m_pszName);

	for (size_t i = 0; i < m_operators.size(); ++i)
	{
		ISpamOperator *pOperator = m_operators[i];
		delete pOperator;
	}

	m_operators.clear();
}

bool CSpamTask::Run()
{
	size_t i;

	if (IsFinished())
		return false;

	for (i = m_iOperatorBegin; i < m_operators.size(); ++i)
	{
		ISpamOperator *pOperator = m_operators[i];

		m_iOperatorBegin = i;

		if (IsWaiting())
			return true;

		pOperator->Run(m_spamInfo);
	}

	if (i == m_operators.size())
	{
		if (m_bLoop)
			m_iOperatorBegin = 0; // go through the operators list again
		else
			m_iOperatorBegin = m_operators.size();
	}

	return true;
}

bool CSpamTask::IsWaiting()
{
	return g_pEngineFuncs->Sys_FloatTime() < m_spamInfo.flNextRunTime;
}

bool CSpamTask::IsFinished()
{
	return (!m_bLoop && m_operators.size() == m_iOperatorBegin) || m_operators.size() == 0;
}

bool CSpamTask::IsLooped()
{
	return m_bLoop;
}

void CSpamTask::SetLoop(bool bLoop)
{
	m_bLoop = bLoop;
}

void CSpamTask::ResetWaiting()
{
	m_spamInfo.flNextRunTime = 0.f;
}

void CSpamTask::AddOperator(ISpamOperator *pOperator)
{
	m_operators.push_back(pOperator);
}

//-----------------------------------------------------------------------------
// CSpamOperatorSend
//-----------------------------------------------------------------------------

CSpamOperatorSend::CSpamOperatorSend()
{
	m_pszMessage = NULL;
}

CSpamOperatorSend::~CSpamOperatorSend()
{
	if (m_pszMessage)
		free((void *)m_pszMessage);
}

void CSpamOperatorSend::Run(CSpamInfo &spamInfo)
{
	static char command_buffer[512];

	sprintf_s(command_buffer, sizeof(command_buffer), "say %s", m_pszMessage);
	g_pEngineFuncs->ClientCmd(command_buffer);
}

void CSpamOperatorSend::SetOperand(const char *pszMessage)
{
	m_pszMessage = (const char *)strdup(pszMessage);
}

//-----------------------------------------------------------------------------
// CSpamOperatorSleep
//-----------------------------------------------------------------------------

CSpamOperatorSleep::CSpamOperatorSleep()
{
	m_flSleepDelay = 0.f;
}

void CSpamOperatorSleep::Run(CSpamInfo &spamInfo)
{
	spamInfo.flNextRunTime = g_pEngineFuncs->Sys_FloatTime() + m_flSleepDelay;
}

void CSpamOperatorSleep::SetOperand(float flSleepDelay)
{
	m_flSleepDelay = flSleepDelay;
}

//-----------------------------------------------------------------------------
// Utilities for parsing files
//-----------------------------------------------------------------------------

#define MS_COMMENT_PREFIX ";#"
#define MS_STRIP_CHARS (" \t\n")

#define MS_STRIP_CHARS_LEN (sizeof(MS_STRIP_CHARS) - 1)
#define MS_COMMENT_PREFIX_LEN (sizeof(MS_COMMENT_PREFIX) - 1)

static int ms_contains_chars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if (chars[i] == ch)
			return 1;
	}

	return 0;
}

static char *ms_lstrip(char *str)
{
	while (*str && ms_contains_chars(*str, MS_STRIP_CHARS, MS_STRIP_CHARS_LEN))
		++str;

	return str;
}

static void ms_rstrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && ms_contains_chars(*end, MS_STRIP_CHARS, MS_STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

static char *ms_strip(char *str)
{
	char *result = ms_lstrip(str);
	ms_rstrip(result);
	return result;
}

static void ms_strip_extension(const char **pszString)
{
	char *pszExtension = NULL;
	char *str = (char *)* pszString;

	while (*str)
	{
		if (*str == '.')
			pszExtension = str;

		str++;
	}

	if (pszExtension)
	{
		*pszExtension = 0;
	}
}

static void ms_remove_comment(char *str)
{
	while (*str && !ms_contains_chars(*str, MS_COMMENT_PREFIX, MS_COMMENT_PREFIX_LEN))
		++str;

	if (*str)
		*str = '\0';
}