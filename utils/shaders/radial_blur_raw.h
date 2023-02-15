#ifndef __radial_blur__H
#define __radial_blur__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char radial_blur_bytes[];
extern "C" unsigned int radial_blur_size;

#else

extern char radial_blur_bytes[];
extern unsigned int radial_blur_size;

#endif

#endif