#ifndef MESSAGE_SPAMMER_H
#define MESSAGE_SPAMMER_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>

class ISpamOperator;
class CSpamTask;

//-----------------------------------------------------------------------------
// Type of spam operator to run
//-----------------------------------------------------------------------------

enum SpamOperatorType
{
	SPAM_OPERATOR_SEND = 0,
	SPAM_OPERATOR_SLEEP
};

//-----------------------------------------------------------------------------
// Shared vars of a spam task
//-----------------------------------------------------------------------------

class CSpamInfo
{
public:
	float flNextRunTime = 0.f;
};

//-----------------------------------------------------------------------------
// Main handler of spam tasks
//-----------------------------------------------------------------------------

class CMessageSpammer
{
public:
	CMessageSpammer();
	~CMessageSpammer();

	void CreateMove(float frametime, struct usercmd_s *cmd, int active);

public:
	void RunTasks();
	void PrintTasks();

	bool AddTask(const char *pszTaskName);
	bool ReloadTask(const char *pszTaskName);
	bool RemoveTask(const char *pszTaskName);

	CSpamTask *GetTask(const char *pszTaskName);

private:
	std::vector<CSpamTask *> m_tasks;
};

//-----------------------------------------------------------------------------
// Spam task to run
//-----------------------------------------------------------------------------

class CSpamTask
{
public:
	CSpamTask(const char *pszName);
	~CSpamTask();

	bool Run();

	bool IsWaiting();
	bool IsFinished();
	bool IsLooped();

	const char *GetName() { return m_pszName; }

	void SetLoop(bool bLoop);
	void ResetWaiting();
	void AddOperator(ISpamOperator *pOperator);

public:
	CSpamInfo m_spamInfo;

private:
	std::vector<ISpamOperator *> m_operators;
	const char *m_pszName;
	int m_iOperatorBegin;
	bool m_bLoop;
};

//-----------------------------------------------------------------------------
// Spam operator interface
//-----------------------------------------------------------------------------

class ISpamOperator
{
public:
	virtual ~ISpamOperator() { }

	virtual void Run(CSpamInfo &spamInfo) = 0;

	virtual SpamOperatorType GetType() = 0;
	virtual const char *GetSyntax() = 0;
};

//-----------------------------------------------------------------------------
// Implement spam operators
//-----------------------------------------------------------------------------

class CSpamOperatorSend : public ISpamOperator
{
public:
	CSpamOperatorSend();
	~CSpamOperatorSend() override;

	void Run(CSpamInfo &spamInfo) override;

	SpamOperatorType GetType() override { return SPAM_OPERATOR_SEND; }
	const char *GetSyntax() override { return "send [message]"; }

public:
	void SetOperand(const char *pszMessage);

private:
	const char *m_pszMessage;
};

class CSpamOperatorSleep : public ISpamOperator
{
public:
	CSpamOperatorSleep();

	void Run(CSpamInfo &spamInfo) override;

	SpamOperatorType GetType() override { return SPAM_OPERATOR_SLEEP; }
	const char *GetSyntax() override { return "sleep [delay]"; }

public:
	void SetOperand(float flSleepDelay);

private:
	float m_flSleepDelay;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern CMessageSpammer g_MessageSpammer;

#endif // MESSAGE_SPAMMER_H