cmake_minimum_required(VERSION 3.3.2)

project(AutoUpdater)

set(AUTOUPDATER "svenint_autoupdate")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(AUTOUPDATER_SOURCE_FILES
	src/main.cpp
	
	# Raws of update files
	src/files/au_beep_synthtone01wav_raw.c
	src/files/au_hitmarkerwav_raw.c
	src/files/au_bunny_raw.c
	src/files/au_glew32_raw.c
	src/files/au_logo_raw.c
	src/files/au_mainlua_raw.c
	src/files/au_menu_click01wav_raw.c
	src/files/au_menu_image_raw.c
	src/files/au_hitmarkertga_raw.c
	src/files/au_radar_roundtga_raw.c
	src/files/au_sven_internal_raw.c
	src/files/au_talkwav_raw.c
	src/files/au_testlua_raw.c
	
	src/files/au_beep_synthtone01wav_raw.h
	src/files/au_hitmarkerwav_raw.h
	src/files/au_bunny_raw.h
	src/files/au_glew32_raw.h
	src/files/au_logo_raw.h
	src/files/au_mainlua_raw.h
	src/files/au_menu_click01wav_raw.h
	src/files/au_menu_image_raw.h
	src/files/au_hitmarkertga_raw.h
	src/files/au_radar_roundtga_raw.h
	src/files/au_sven_internal_raw.h
	src/files/au_talkwav_raw.h
	src/files/au_testlua_raw.h
	
	# Shared
	shared/au_platform.h
	shared/au_protocol.h
	shared/au_app_info.h
	shared/au_app_version.h
	shared/au_utils.cpp
	shared/au_utils.h
)

add_subdirectory(server)

if (MSVC)
	add_executable(${AUTOUPDATER} WIN32 ${AUTOUPDATER_SOURCE_FILES})
else()
	add_executable(${AUTOUPDATER} ${AUTOUPDATER_SOURCE_FILES})
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)

if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
	set(CMAKE_CXX_FLAGS "-m32 -g -std=c++14 -w -Wl,--no-as-needed -ldl")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-Os")
	
	set(CMAKE_C_FLAGS "-m32 -g -w -Wl,--no-as-needed -ldl")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -s -DNDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "-Os")
endif()
