#include "config.h"
#include "log.h"
#include "window_hook.h"

#include <Windows.h>

namespace {

HMODULE g_self = nullptr;

bool IsArknightsProcess() {
  wchar_t path[MAX_PATH] = {};
  const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) {
    return false;
  }
  const wchar_t* base = path;
  for (const wchar_t* p = path; *p; ++p) {
    if (*p == L'\\' || *p == L'/') {
      base = p + 1;
    }
  }
  return lstrcmpiW(base, L"Arknights.exe") == 0;
}

DWORD WINAPI WorkerThread(LPVOID) {
  Config config{};
  if (!LoadConfig(g_self, &config)) {
    return 1;
  }

  Log(L"altenter worker started");
  WindowHookRun(config);
  return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_self = module;
    DisableThreadLibraryCalls(module);
    if (!IsArknightsProcess()) {
      return TRUE;
    }
    HANDLE thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
    if (thread) {
      CloseHandle(thread);
    }
  }
  return TRUE;
}
