#ifndef __gaussian_blur_fast__H
#define __gaussian_blur_fast__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char gaussian_blur_fast_bytes[];
extern "C" unsigned int gaussian_blur_fast_size;

#else

extern char gaussian_blur_fast_bytes[];
extern unsigned int gaussian_blur_fast_size;

#endif

#endif