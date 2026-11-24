#include "config.h"

#include "log.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

std::wstring Trim(std::wstring s) {
  auto not_space = [](wchar_t c) { return !iswspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

std::wstring ToLower(std::wstring s) {
  for (auto& c : s) {
    c = static_cast<wchar_t>(towlower(c));
  }
  return s;
}

bool ParseVk(const std::wstring& token, UINT* vk) {
  if (token == L"enter" || token == L"return") {
    *vk = VK_RETURN;
    return true;
  }
  if (token == L"escape" || token == L"esc") {
    *vk = VK_ESCAPE;
    return true;
  }
  if (token == L"space") {
    *vk = VK_SPACE;
    return true;
  }
  if (token == L"tab") {
    *vk = VK_TAB;
    return true;
  }
  if (token.size() >= 2 && (token[0] == L'f' || token[0] == L'F')) {
    bool all_digits = true;
    for (size_t i = 1; i < token.size(); ++i) {
      if (!iswdigit(token[i])) {
        all_digits = false;
        break;
      }
    }
    if (all_digits) {
      int n = _wtoi(token.c_str() + 1);
      if (n >= 1 && n <= 24) {
        *vk = VK_F1 + (n - 1);
        return true;
      }
    }
  }
  if (token.size() == 1) {
    wchar_t c = token[0];
    if (c >= L'a' && c <= L'z') {
      *vk = static_cast<UINT>(c - L'a' + 'A');
      return true;
    }
    if (c >= L'0' && c <= L'9') {
      *vk = static_cast<UINT>(c);
      return true;
    }
  }
  return false;
}

bool ParseHotkey(const std::wstring& spec, Hotkey* out) {
  Hotkey hk{};
  std::wstring remaining = ToLower(Trim(spec));
  if (remaining.empty()) {
    return false;
  }

  size_t pos = 0;
  while (pos < remaining.size()) {
    size_t plus = remaining.find(L'+', pos);
    std::wstring part = Trim(remaining.substr(
        pos, plus == std::wstring::npos ? std::wstring::npos : plus - pos));
    pos = plus == std::wstring::npos ? remaining.size() : plus + 1;

    if (part.empty()) {
      continue;
    }
    if (part == L"alt") {
      hk.modifiers |= MOD_ALT;
    } else if (part == L"ctrl" || part == L"control") {
      hk.modifiers |= MOD_CONTROL;
    } else if (part == L"shift") {
      hk.modifiers |= MOD_SHIFT;
    } else if (part == L"win" || part == L"windows") {
      hk.modifiers |= MOD_WIN;
    } else if (ParseVk(part, &hk.vk)) {
      // key set
    } else {
      return false;
    }
  }

  if (hk.vk == 0) {
    return false;
  }
  *out = hk;
  return true;
}

void ParseHotkeyList(const std::wstring& list, std::vector<Hotkey>* out) {
  size_t start = 0;
  while (start <= list.size()) {
    size_t comma = list.find(L',', start);
    std::wstring item = Trim(list.substr(
        start, comma == std::wstring::npos ? std::wstring::npos : comma - start));
    if (!item.empty()) {
      Hotkey hk{};
      if (ParseHotkey(item, &hk)) {
        out->push_back(hk);
      } else {
        Log(L"Ignoring unrecognized hotkey: %s", item.c_str());
      }
    }
    if (comma == std::wstring::npos) {
      break;
    }
    start = comma + 1;
  }
}

bool ReadIniBool(const wchar_t* path, const wchar_t* section, const wchar_t* key,
                 bool default_value) {
  wchar_t buf[32] = {};
  GetPrivateProfileStringW(section, key, default_value ? L"true" : L"false",
                           buf, static_cast<DWORD>(std::size(buf)), path);
  std::wstring v = ToLower(Trim(buf));
  return v == L"1" || v == L"true" || v == L"yes" || v == L"on";
}

void WriteDefaultIni(const std::wstring& path) {
  static const char kDefault[] =
      "[hotkeys]\r\n"
      "; Comma-separated list. Supported modifiers: alt, ctrl, shift, win.\r\n"
      "; Keys: enter, f1-f24, a-z, 0-9, escape, space, tab.\r\n"
      "; Empty value disables interception.\r\n"
      "toggle = alt+enter, f11\r\n"
      "\r\n"
      "[general]\r\n"
      "; Apply borderless shortly after the Unity window appears.\r\n"
      "; Use Windowed mode in the game settings (not exclusive fullscreen).\r\n"
      "start_borderless = false\r\n"
      "; Write altenter.log next to this ini.\r\n"
      "log = false\r\n";
  HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) {
    return;
  }
  DWORD written = 0;
  WriteFile(h, kDefault, static_cast<DWORD>(sizeof(kDefault) - 1), &written,
            nullptr);
  CloseHandle(h);
}

}  // namespace

bool LoadConfig(HMODULE self, Config* out) {
  Config cfg{};

  wchar_t module_path[MAX_PATH] = {};
  if (!GetModuleFileNameW(self, module_path, MAX_PATH)) {
    return false;
  }

  std::wstring path(module_path);
  const size_t slash = path.find_last_of(L"\\/");
  cfg.dll_dir = slash == std::wstring::npos ? L"." : path.substr(0, slash);
  const std::wstring ini_path = cfg.dll_dir + L"\\altenter.ini";

  WriteDefaultIni(ini_path);

  cfg.log = ReadIniBool(ini_path.c_str(), L"general", L"log", false);
  LogInit(cfg.log, cfg.dll_dir.c_str());

  cfg.start_borderless =
      ReadIniBool(ini_path.c_str(), L"general", L"start_borderless", false);

  wchar_t toggle_buf[256] = {};
  GetPrivateProfileStringW(L"hotkeys", L"toggle", L"alt+enter, f11", toggle_buf,
                           static_cast<DWORD>(std::size(toggle_buf)),
                           ini_path.c_str());
  ParseHotkeyList(toggle_buf, &cfg.hotkeys);

  if (cfg.hotkeys.empty()) {
    Log(L"No valid hotkeys configured; key interception disabled");
  } else {
    Log(L"Loaded %zu hotkey(s)", cfg.hotkeys.size());
  }
  Log(L"start_borderless=%d", cfg.start_borderless ? 1 : 0);

  *out = std::move(cfg);
  return true;
}

bool HotkeyMatches(const Hotkey& hk, UINT msg, WPARAM wParam, LPARAM lParam) {
  (void)lParam;
  const UINT vk = static_cast<UINT>(wParam);
  if (vk != hk.vk) {
    return false;
  }

  const bool alt_down = (GetKeyState(VK_MENU) & 0x8000) != 0;
  const bool ctrl_down = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  const bool shift_down = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  const bool win_down =
      (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;

  // Alt+key arrives as WM_SYSKEYDOWN; bare F11 as WM_KEYDOWN.
  if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN && msg != WM_KEYUP &&
      msg != WM_SYSKEYUP) {
    return false;
  }

  const bool want_alt = (hk.modifiers & MOD_ALT) != 0;
  const bool want_ctrl = (hk.modifiers & MOD_CONTROL) != 0;
  const bool want_shift = (hk.modifiers & MOD_SHIFT) != 0;
  const bool want_win = (hk.modifiers & MOD_WIN) != 0;

  return alt_down == want_alt && ctrl_down == want_ctrl &&
         shift_down == want_shift && win_down == want_win;
}
