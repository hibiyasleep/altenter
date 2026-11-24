#include "window_hook.h"

#include "borderless.h"
#include "log.h"

#include <atomic>
#include <vector>

namespace {

Config g_config{};
HWND g_hwnd = nullptr;
std::atomic<WNDPROC> g_original_proc{nullptr};
std::atomic<UINT> g_swallow_keyup_vk{0};

LRESULT CallOriginal(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  WNDPROC original = g_original_proc.load(std::memory_order_acquire);
  if (!original) {
    const DWORD start = GetTickCount();
    do {
      SwitchToThread();
      original = g_original_proc.load(std::memory_order_acquire);
    } while (!original && static_cast<DWORD>(GetTickCount() - start) < 50);
  }
  if (!original) {
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
  return CallWindowProcW(original, hwnd, msg, wParam, lParam);
}

struct EnumData {
  DWORD pid = 0;
  HWND found = nullptr;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
  auto* data = reinterpret_cast<EnumData*>(lParam);
  DWORD window_pid = 0;
  GetWindowThreadProcessId(hwnd, &window_pid);
  if (window_pid != data->pid) {
    return TRUE;
  }
  if (!IsWindowVisible(hwnd)) {
    return TRUE;
  }

  wchar_t class_name[64] = {};
  if (GetClassNameW(hwnd, class_name, static_cast<int>(std::size(class_name))) ==
      0) {
    return TRUE;
  }
  if (_wcsicmp(class_name, L"UnityWndClass") != 0) {
    return TRUE;
  }

  data->found = hwnd;
  return FALSE;
}

HWND FindUnityWindow() {
  EnumData data{};
  data.pid = GetCurrentProcessId();
  EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
  return data.found;
}

bool MatchesAnyHotkey(UINT msg, WPARAM wParam, LPARAM lParam) {
  for (const Hotkey& hk : g_config.hotkeys) {
    if (HotkeyMatches(hk, msg, wParam, lParam)) {
      return true;
    }
  }
  return false;
}

bool IsRepeat(LPARAM lParam) { return (lParam & (1 << 30)) != 0; }

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                             LPARAM lParam) {
  if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
    if (MatchesAnyHotkey(msg, wParam, lParam)) {
      if (!IsRepeat(lParam)) {
        Log(L"Hotkey matched vk=0x%02X msg=0x%04X",
            static_cast<unsigned>(wParam), msg);
        BorderlessToggle(hwnd);
      }
      g_swallow_keyup_vk.store(static_cast<UINT>(wParam));
      return 0;
    }
  }

  if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
    const UINT swallow_vk = g_swallow_keyup_vk.load();
    if (swallow_vk != 0 && swallow_vk == static_cast<UINT>(wParam)) {
      g_swallow_keyup_vk.store(0);
      return 0;
    }
  }

  // Some Unity builds re-apply styles on activate; keep borderless sticky.
  if (msg == WM_ACTIVATE && BorderlessIsActive() && LOWORD(wParam) != WA_INACTIVE) {
    BorderlessEnter(hwnd);
  }

  return CallOriginal(hwnd, msg, wParam, lParam);
}

bool SubclassWindow(HWND hwnd) {
  if (!hwnd || g_original_proc.load(std::memory_order_acquire) != nullptr) {
    return false;
  }
  SetLastError(0);
  LONG_PTR prev = SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(HookWndProc));
  if (prev == 0 && GetLastError() != 0) {
    Log(L"SetWindowLongPtr failed: %lu", GetLastError());
    return false;
  }
  g_original_proc.store(reinterpret_cast<WNDPROC>(prev),
                        std::memory_order_release);
  g_hwnd = hwnd;
  Log(L"Subclassed hwnd=%p", hwnd);
  return true;
}

}  // namespace

HWND WaitForUnityWindow() {
  HWND hwnd = nullptr;
  for (int i = 0; i < 600 && !hwnd; ++i) {
    hwnd = FindUnityWindow();
    if (!hwnd) {
      Sleep(100);
    }
  }
  return hwnd;
}

void WindowHookRun(const Config& config, HWND hwnd) {
  g_config = config;

  if (!SubclassWindow(hwnd)) {
    return;
  }

  if (g_config.start_borderless) {
    // Give Unity a moment to finish its own show/style setup.
    Sleep(500);
    BorderlessEnter(hwnd);
  }

  // Keep the thread alive for the process lifetime. Only reinstall when the
  // current proc is the one we saved, which means the hook was removed.
  // Anything else has wrapped HookWndProc and will call it; replacing that
  // wrapper makes the two procedures call each other until the stack overflows.
  while (IsWindow(g_hwnd)) {
    Sleep(1000);
    const WNDPROC saved_proc = g_original_proc.load(std::memory_order_acquire);
    const WNDPROC installed_proc = reinterpret_cast<WNDPROC>(
        GetWindowLongPtrW(g_hwnd, GWLP_WNDPROC));
    const bool hook_removed = installed_proc == saved_proc;
    if (!hook_removed) {
      continue;
    }
    Log(L"WndProc hook was removed; subclassing again");
    g_original_proc.store(nullptr, std::memory_order_release);
    if (!SubclassWindow(g_hwnd)) {
      break;
    }
  }

  Log(L"Window gone; hook thread exiting");
}
