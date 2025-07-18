#ifndef __AUTOUPDATE_APP_INFO__H
#define __AUTOUPDATE_APP_INFO__H

#ifdef _WIN32
#pragma once
#endif

#include "../../plugin.h"

#define AUTOUPDATE_APP_NAME "SvenInt"
#define AUTOUPDATE_APP_FILENAME "svenmod/plugins/sven_internal.dll"

#define AUTOUPDATE_APP_MAJOR_VERSION ( SVENINT_MAJOR_VERSION )
#define AUTOUPDATE_APP_MINOR_VERSION ( SVENINT_MINOR_VERSION )
#define AUTOUPDATE_APP_PATCH_VERSION ( SVENINT_PATCH_VERSION )

#define AUTOUPDATE_APP_MAJOR_VERSION_STRING SVENINT_MAJOR_VERSION_STRING
#define AUTOUPDATE_APP_MINOR_VERSION_STRING SVENINT_MINOR_VERSION_STRING
#define AUTOUPDATE_APP_PATCH_VERSION_STRING SVENINT_PATCH_VERSION_STRING

#define AUTOUPDATE_APP_VERSION_STRING AUTOUPDATE_APP_MAJOR_VERSION_STRING "." AUTOUPDATE_APP_MINOR_VERSION_STRING "." AUTOUPDATE_APP_PATCH_VERSION_STRING

#endif
