#pragma once

#include <Windows.h>

#include <string>
#include <vector>

struct Hotkey {
  UINT modifiers = 0;  // MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN
  UINT vk = 0;
};

struct Config {
  std::vector<Hotkey> hotkeys;
  bool start_borderless = false;
  bool log = false;
  std::wstring dll_dir;
};

bool LoadConfig(HMODULE self, Config* out);
bool HotkeyMatches(const Hotkey& hk, UINT msg, WPARAM wParam, LPARAM lParam);
