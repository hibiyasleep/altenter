#include "config.h"
#include "log.h"
#include "window_hook.h"

#include <Windows.h>

namespace {

HMODULE g_self = nullptr;

DWORD WINAPI WorkerThread(LPVOID) {
  // Keep LoadConfig after this wait: helper processes such as
  // UnityCrashHandler64.exe also load this DLL, and LogInit would delete the
  // game's log from inside them.
  const HWND hwnd = WaitForUnityWindow();
  if (!hwnd) {
    return 0;
  }

  Config config{};
  if (!LoadConfig(g_self, &config)) {
    return 1;
  }

  Log(L"altenter worker started; found Unity window hwnd=%p", hwnd);
  WindowHookRun(config, hwnd);
  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_self = module;
    DisableThreadLibraryCalls(module);
    HANDLE thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    if (thread) {
      CloseHandle(thread);
    }
  }
  return TRUE;
}
