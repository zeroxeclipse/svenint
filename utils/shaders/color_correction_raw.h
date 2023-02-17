#ifndef __color_correction__H
#define __color_correction__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char color_correction_bytes[];
extern "C" unsigned int color_correction_size;

#else

extern char color_correction_bytes[];
extern unsigned int color_correction_size;

#endif

#endif