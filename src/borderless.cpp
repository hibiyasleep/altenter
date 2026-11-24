#include "borderless.h"

#include "log.h"

namespace {

struct SavedState {
  LONG_PTR style = 0;
  LONG_PTR exstyle = 0;
  RECT rect{};
  bool valid = false;
};

SavedState g_saved{};
bool g_active = false;

}  // namespace

bool BorderlessIsActive() { return g_active; }

void BorderlessEnter(HWND hwnd) {
  if (!IsWindow(hwnd)) {
    return;
  }

  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(monitor, &mi)) {
    Log(L"GetMonitorInfo failed: %lu", GetLastError());
    return;
  }

  if (!g_active) {
    g_saved.style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    g_saved.exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    GetWindowRect(hwnd, &g_saved.rect);
    g_saved.valid = true;
  }

  LONG_PTR style = g_saved.valid ? g_saved.style : GetWindowLongPtrW(hwnd, GWL_STYLE);
  style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
             WS_SYSMENU);
  style |= WS_POPUP;
  SetWindowLongPtrW(hwnd, GWL_STYLE, style);

  LONG_PTR exstyle =
      g_saved.valid ? g_saved.exstyle : GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  exstyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE |
               WS_EX_WINDOWEDGE);
  SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exstyle);

  const RECT& r = mi.rcMonitor;
  SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left,
               r.bottom - r.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
  g_active = true;
  Log(L"Entered borderless (%dx%d)", r.right - r.left, r.bottom - r.top);
}

void BorderlessLeave(HWND hwnd) {
  if (!IsWindow(hwnd) || !g_saved.valid) {
    return;
  }

  SetWindowLongPtrW(hwnd, GWL_STYLE, g_saved.style);
  SetWindowLongPtrW(hwnd, GWL_EXSTYLE, g_saved.exstyle);
  SetWindowPos(hwnd, HWND_TOP, g_saved.rect.left, g_saved.rect.top,
               g_saved.rect.right - g_saved.rect.left,
               g_saved.rect.bottom - g_saved.rect.top,
               SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
  g_active = false;
  Log(L"Left borderless");
}

void BorderlessToggle(HWND hwnd) {
  if (g_active) {
    BorderlessLeave(hwnd);
  } else {
    BorderlessEnter(hwnd);
  }
}
