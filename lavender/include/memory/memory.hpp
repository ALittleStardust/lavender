#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>
#include <iostream>

#include <nt.hpp>
#include <cpu/cpu.hpp>

#if defined(_WIN32)
    #include <windows.h>
    #include <io.h>
    #include <shlobj.h>
    #include <tlhelp32.h>
    #include <wow64apiset.h>
    #include <ntstatus.h>
    #if defined(__MINGW32__)
        #include <lmcons.h>
        #include <ntdef.h> 
    #endif
#endif

namespace lavender {

struct MemoryInformation {
public:
    DWORD percentage = 0;
    DWORDLONG usable_physical = 0;
    DWORDLONG available_physical = 0;
    ULONGLONG total_physical = 0;

    MemoryInformation() {}

    MemoryInformation(DWORD percentage, DWORDLONG usable_physical, DWORDLONG available_physical, ULONGLONG total_physical) :
        percentage(percentage) 
        , usable_physical(usable_physical)
        , total_physical(total_physical)
        , available_physical(available_physical)
    {
    }

    bool Parse()
    {
        ::MEMORYSTATUSEX memory_status = {0};
        memory_status.dwLength = sizeof(memory_status);

        ULONGLONG total_physical = 0;

        if (::GlobalMemoryStatusEx(&memory_status) != 0 && ::GetPhysicallyInstalledSystemMemory(&total_physical) == TRUE) {
        }

        return true;
    }
};
}
