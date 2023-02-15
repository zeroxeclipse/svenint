#ifndef __bokeh__H
#define __bokeh__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char bokeh_bytes[];
extern "C" unsigned int bokeh_size;

#else

extern char bokeh_bytes[];
extern unsigned int bokeh_size;

#endif

#endif