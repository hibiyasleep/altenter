#include <Windows.h>

#include "log.h"

namespace {

HMODULE RealVersion() {
  static HMODULE mod = []() -> HMODULE {
    wchar_t path[MAX_PATH] = {};
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
      return nullptr;
    }
    if (wcscat_s(path, L"\\version.dll") != 0) {
      return nullptr;
    }
    HMODULE real = LoadLibraryW(path);
    if (!real) {
      Log(L"Failed to load system version.dll: %lu", GetLastError());
    }
    return real;
  }();
  return mod;
}

FARPROC RealProc(const char* name) {
  HMODULE mod = RealVersion();
  return mod ? GetProcAddress(mod, name) : nullptr;
}

}  // namespace

#define EXPORT_AS(exported) \
  __pragma(comment(linker, "/export:" #exported "=Proxy_" #exported))

#define PROXY_FN(ret, name, args, call_args)                                 \
  EXPORT_AS(name)                                                            \
  extern "C" ret WINAPI Proxy_##name args {                                  \
    using Fn = ret(WINAPI*) args;                                            \
    static Fn fn = reinterpret_cast<Fn>(RealProc(#name));                    \
    if (!fn) {                                                               \
      return ret{};                                                          \
    }                                                                        \
    return fn call_args;                                                     \
  }

PROXY_FN(BOOL, GetFileVersionInfoA,
         (LPCSTR a, DWORD b, DWORD c, LPVOID d), (a, b, c, d))
PROXY_FN(BOOL, GetFileVersionInfoW,
         (LPCWSTR a, DWORD b, DWORD c, LPVOID d), (a, b, c, d))
PROXY_FN(BOOL, GetFileVersionInfoExA,
         (DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e), (a, b, c, d, e))
PROXY_FN(BOOL, GetFileVersionInfoExW,
         (DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e), (a, b, c, d, e))
PROXY_FN(DWORD, GetFileVersionInfoSizeA, (LPCSTR a, LPDWORD b), (a, b))
PROXY_FN(DWORD, GetFileVersionInfoSizeW, (LPCWSTR a, LPDWORD b), (a, b))
PROXY_FN(DWORD, GetFileVersionInfoSizeExA,
         (DWORD a, LPCSTR b, LPDWORD c), (a, b, c))
PROXY_FN(DWORD, GetFileVersionInfoSizeExW,
         (DWORD a, LPCWSTR b, LPDWORD c), (a, b, c))
PROXY_FN(DWORD, VerFindFileA,
         (DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPSTR e, PUINT f, LPSTR g,
          PUINT h),
         (a, b, c, d, e, f, g, h))
PROXY_FN(DWORD, VerFindFileW,
         (DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPWSTR e, PUINT f, LPWSTR g,
          PUINT h),
         (a, b, c, d, e, f, g, h))
PROXY_FN(DWORD, VerInstallFileA,
         (DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPCSTR e, LPCSTR f, LPSTR g,
          PUINT h),
         (a, b, c, d, e, f, g, h))
PROXY_FN(DWORD, VerInstallFileW,
         (DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPCWSTR e, LPCWSTR f,
          LPWSTR g, PUINT h),
         (a, b, c, d, e, f, g, h))
PROXY_FN(DWORD, VerLanguageNameA, (DWORD a, LPSTR b, DWORD c), (a, b, c))
PROXY_FN(DWORD, VerLanguageNameW, (DWORD a, LPWSTR b, DWORD c), (a, b, c))
PROXY_FN(BOOL, VerQueryValueA,
         (LPCVOID a, LPCSTR b, LPVOID* c, PUINT d), (a, b, c, d))
PROXY_FN(BOOL, VerQueryValueW,
         (LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d), (a, b, c, d))

EXPORT_AS(GetFileVersionInfoByHandle)
extern "C" DWORD WINAPI Proxy_GetFileVersionInfoByHandle(DWORD flags,
                                                         HANDLE file,
                                                         LPVOID buffer,
                                                         DWORD len) {
  using Fn = DWORD(WINAPI*)(DWORD, HANDLE, LPVOID, DWORD);
  static Fn fn =
      reinterpret_cast<Fn>(RealProc("GetFileVersionInfoByHandle"));
  if (!fn) {
    return 0;
  }
  return fn(flags, file, buffer, len);
}
