#pragma once

#include "config.h"

#include <Windows.h>

HWND WaitForUnityWindow();
void WindowHookRun(const Config& config, HWND hwnd);
