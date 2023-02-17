#ifndef __ssao__H
#define __ssao__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char ssao_bytes[];
extern "C" unsigned int ssao_size;

#else

extern char ssao_bytes[];
extern unsigned int ssao_size;

#endif

#endif