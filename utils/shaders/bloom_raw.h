#ifndef __bloom__H
#define __bloom__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char bloom_bytes[];
extern "C" unsigned int bloom_size;

#else

extern char bloom_bytes[];
extern unsigned int bloom_size;

#endif

#endif