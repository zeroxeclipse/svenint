#ifndef __godrays__H
#define __godrays__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char godrays_bytes[];
extern "C" unsigned int godrays_size;

#else

extern char godrays_bytes[];
extern unsigned int godrays_size;

#endif

#endif