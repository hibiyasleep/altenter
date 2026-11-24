#include "log.h"

#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace {

std::mutex g_mutex;
bool g_enabled = false;
wchar_t g_path[MAX_PATH] = {};

}  // namespace

void LogInit(bool enabled, const wchar_t* dll_dir) {
  g_enabled = enabled;
  if (!enabled || !dll_dir) {
    return;
  }
  _snwprintf_s(g_path, _TRUNCATE, L"%s\\altenter.log", dll_dir);
  DeleteFileW(g_path);
}

void Log(const wchar_t* fmt, ...) {
  if (!g_enabled || g_path[0] == L'\0') {
    return;
  }

  wchar_t message[1024];
  va_list args;
  va_start(args, fmt);
  _vsnwprintf_s(message, _TRUNCATE, fmt, args);
  va_end(args);

  SYSTEMTIME st{};
  GetLocalTime(&st);

  wchar_t line[1200];
  _snwprintf_s(line, _TRUNCATE, L"[%02u:%02u:%02u.%03u] %s\r\n", st.wHour,
               st.wMinute, st.wSecond, st.wMilliseconds, message);

  std::lock_guard<std::mutex> lock(g_mutex);
  HANDLE file =
      CreateFileW(g_path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return;
  }

  DWORD written = 0;
  WriteFile(file, line, static_cast<DWORD>(wcslen(line) * sizeof(wchar_t)),
            &written, nullptr);
  CloseHandle(file);
}
