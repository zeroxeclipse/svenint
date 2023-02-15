#ifndef __dof_blur__H
#define __dof_blur__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char dof_blur_bytes[];
extern "C" unsigned int dof_blur_size;

#else

extern char dof_blur_bytes[];
extern unsigned int dof_blur_size;

#endif

#endif