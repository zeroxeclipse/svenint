#ifndef __gaussian_blur__H
#define __gaussian_blur__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char gaussian_blur_bytes[];
extern "C" unsigned int gaussian_blur_size;

#else

extern char gaussian_blur_bytes[];
extern unsigned int gaussian_blur_size;

#endif

#endif