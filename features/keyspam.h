#ifndef KEYSPAM_H
#define KEYSPAM_H

#ifdef _WIN32
#pragma once
#endif

class CKeySpam
{
public:
	bool Init();
	void CreateMove(float frametime, struct usercmd_s *cmd, int active);

private:
	void KeySpam();
};

extern CKeySpam g_KeySpam;

#endif // KEYSPAM_H