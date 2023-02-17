#ifndef __vignette__H
#define __vignette__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char vignette_bytes[];
extern "C" unsigned int vignette_size;

#else

extern char vignette_bytes[];
extern unsigned int vignette_size;

#endif

#endif