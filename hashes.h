#ifndef HASHES_H
#define HASHES_H

class Hashes
{
public:
	void LoadHashInit(void* CSvenInternalInstance);
	bool CheckLoadHash();

private:
	void* CSvenInternalPtr = nullptr;
	void* LoadPtr = nullptr;
	void* PostLoadPtr = nullptr;

	size_t LoadSize = 0;

	char InitialLoadHash[63];
};

extern Hashes g_Hashes;

#endif