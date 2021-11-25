// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#include <nt.hpp>
#include <os/version.hpp>

#if defined(_WIN32)
    #include <lm.h>
    #include <sddl.h>
    #include <winternl.h>
    #include <ntsecapi.h>
    #include <wow64apiset.h>
#endif

namespace lavender {

namespace platform {

bool IsProcessElevated(const ::HANDLE process)
{
    if (process) {
        if (::HANDLE token = nullptr; ::OpenProcessToken(process, TOKEN_QUERY, &token) != 0) {
            ::DWORD unused = 0;
            if (::TOKEN_ELEVATION_TYPE elevation_type = TOKEN_ELEVATION_TYPE::TokenElevationTypeDefault; ::GetTokenInformation(token, TOKEN_INFORMATION_CLASS::TokenElevationType, (::LPVOID) &elevation_type, sizeof(elevation_type), &unused) != 0) {
                return elevation_type == TOKEN_ELEVATION_TYPE::TokenElevationTypeFull;
            }

            ::CloseHandle(token);
        }
    }

    return false;
}

bool IsPrivilegeEnabled(const ::LPCWSTR privilege)
{
    ::HANDLE token = nullptr;

    if (::HANDLE process = ::GetCurrentProcess(); ::OpenProcessToken(process, TOKEN_QUERY, &token) != 0) {
        ::LUID luid = {0};
        if (::LookupPrivilegeValueW(nullptr, (::LPCWSTR) privilege, &luid) != 0) {
            ::PRIVILEGE_SET privs = {
                .PrivilegeCount = 1,
                .Control = PRIVILEGE_SET_ALL_NECESSARY,
                .Privilege[0] = {
                    .Luid = luid,
                    .Attributes = SE_PRIVILEGE_ENABLED
                }
            };
            
            if (::BOOL result = FALSE; ::PrivilegeCheck(token, &privs, &result) != 0) {
                ::CloseHandle(token);
                return result == TRUE;
            }
        }

        ::CloseHandle(token);
    }

    return false;
}

using IsWow64Process2Ptr = ::BOOL (WINAPI *) (::HANDLE, ::USHORT *, ::USHORT *);
std::optional<bool> IsProcessWOW64(const ::HANDLE process)
{
    // IsWow64Process2() was introduced in NT 10 version 1511
    if (os::OSVersionInformation version; version.Initialize() && version.GetBuildNumber() >= 1511) {
        const auto module = ::GetModuleHandle("kernel32.dll");
        if (module != nullptr) {
            const IsWow64Process2Ptr f = (IsWow64Process2Ptr) ::GetProcAddress(module, "IsWow64Process2");
            ::USHORT target_info, host_info;
            if (f != nullptr && f(process, &target_info, &host_info) != 0) {
                return target_info != IMAGE_FILE_MACHINE_UNKNOWN; // target_info returns IMAGE_FILE_MACHINE_UNKNOWN if is not a WOW64 process.
            }
        }
    }
    else {
        ::BOOL status;
        if (::IsWow64Process(process, &status) != 0)
            return status;
    }

    return std::nullopt;   
}

std::optional<::DWORD> GetPageFileSize()
{
    ::SYSTEM_INFO info = {0};

    if (const auto WOW64 = IsProcessWOW64(::GetCurrentProcess()); WOW64.has_value()) {
        if (WOW64.value())
            ::GetNativeSystemInfo(&info);
        else
            ::GetSystemInfo(&info);
        
        return info.dwPageSize;
    }

    return std::nullopt;
}

std::optional<::ULONG> GetSystemErrorFromNTStatus(const ::NTSTATUS status)
{
    if (::ULONG error = ::RtlNtStatusToDosError(status); error != ERROR_MR_MID_NOT_FOUND)
        return error;
    
    return std::nullopt;
}

std::optional<::ULONG> GetSystemErrorFromLSAStatus(const ::NTSTATUS status)
{
    if (::ULONG error = ::LsaNtStatusToWinError(status); error != ERROR_MR_MID_NOT_FOUND)
        return error;
    
    return std::nullopt;
}

std::optional<std::string> GetStringSIDFromPSID(const ::PSID psid)
{
    if (::LPSTR buffer = nullptr; ::ConvertSidToStringSidA(psid, &buffer) != 0) {
        std::string sid(buffer);
        ::LocalFree(buffer);
        return sid;
    }

    return std::nullopt;
}

std::optional<std::unique_ptr<::SID>> GetPSIDFromName(const ::LPCWSTR name)
{
    std::unique_ptr<::SID> psid(nullptr);

    if (name) {
        ::DWORD size = 0;

        // Why on Earth would Microsoft make me passing in the buffers receiving the domain name when all I want is just getting the SID is beyond me.
        // And why would cchReferencedDomainName, being a pointer as it is, couldn't just be nullptr if ReferencedDomainName is also nullptr? Why does it has to be zero?
        if (::DWORD unused = 0; ::LookupAccountNameW(nullptr, name, psid.get(), &size, nullptr, &unused, nullptr) == 0) {
            if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                psid = std::unique_ptr<::SID>(reinterpret_cast<::SID *>(new ::BYTE[size]));
                if (!psid)
                    return std::nullopt;
                
                // Now that we've probed the required size of the SID, perform an actual query.
                // Note that domain name/size/type field (in that order) goes unused and thus would be named as such.
                wchar_t unused1[512];
                ::DWORD unused2 = 512;
                ::SID_NAME_USE unused3;
                if (::LookupAccountNameW(nullptr, name, psid.get(), &size, unused1, &unused2, &unused3) != 0)
                {  
                    if (::IsValidSid(psid.get()) == 0)
                        return std::nullopt;
                }
            }
        }
    }
    
    return psid;
}

std::vector<std::wstring> GetGroupsOfUser(const ::LPCWSTR name)
{
    std::vector<std::wstring> groups;

    if (name != nullptr) {
        ::LPLOCALGROUP_USERS_INFO_0 buffer = nullptr;
        ::DWORD count = 0;
        ::DWORD total = 0;
        
        if (::NET_API_STATUS status = ::NetUserGetLocalGroups(
            nullptr, 
            name, 
            0, // As of now, the only available level is 0, which corresponds to a buffer of type ::LPLOCALGROUP_USERS_INFO_0.
            LG_INCLUDE_INDIRECT,
            (::LPBYTE *) &buffer, 
            MAX_PREFERRED_LENGTH, 
            &count, 
            &total); 
            status == NERR_Success)
        {
            if (auto ptr = buffer; ptr != nullptr)
            {
                for (auto i = 0; i < count; ++i)
                {
                    if (ptr != nullptr)
                    {
                        groups.push_back(ptr->lgrui0_name);
                        ++ptr;
                    }
                }
            }
        }

        if (buffer != nullptr) {
            ::NetApiBufferFree(buffer);
            buffer = nullptr;
        }
    }

    return groups;
}

}

}
