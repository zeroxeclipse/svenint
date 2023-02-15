#ifndef __pp_fullscreen__H
#define __pp_fullscreen__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char pp_fullscreen_bytes[];
extern "C" unsigned int pp_fullscreen_size;

#else

extern char pp_fullscreen_bytes[];
extern unsigned int pp_fullscreen_size;

#endif

#endif