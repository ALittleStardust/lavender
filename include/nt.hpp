// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#pragma once

#if defined(_WIN32)
    #include <windows.h>
    #include <ntstatus.h>
#endif

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace lavender {

namespace platform {

bool IsProcessElevated(const ::HANDLE process);
bool IsPrivilegeEnabled(const ::LPCWSTR privilege);
std::optional<bool> IsProcessWOW64(const ::HANDLE process);
std::optional<::DWORD> GetPageFileSize();
std::vector<std::wstring> GetGroupsOfUser(const ::LPCWSTR name);
std::optional<::ULONG> GetSystemErrorFromNTStatus(const ::NTSTATUS status);
std::optional<::ULONG> GetSystemErrorFromLSAStatus(const ::NTSTATUS status);
std::optional<std::string> GetStringSIDFromPSID(const ::PSID psid);
std::optional<std::unique_ptr<::SID>> GetPSIDFromName(const ::LPCWSTR name);

}

}
