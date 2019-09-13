#ifndef PLAYGROUND_PARENT_WINDOW_H
#define PLAYGROUND_PARENT_WINDOW_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(__cplusplus)
#define swift_playground_extern extern "C"
#else
#define swift_playground_extern extern
#endif

#if defined(swift_playground_EXPORTS)
#define swift_playground_ABI __declspec(dllexport)
#else
#define swift_playground_ABI __declspec(dllimport)
#endif

swift_playground_extern swift_playground_ABI HWND g_ui;

#endif
