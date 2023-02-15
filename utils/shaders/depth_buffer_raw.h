#ifndef __depth_buffer__H
#define __depth_buffer__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char depth_buffer_bytes[];
extern "C" unsigned int depth_buffer_size;

#else

extern char depth_buffer_bytes[];
extern unsigned int depth_buffer_size;

#endif

#endif