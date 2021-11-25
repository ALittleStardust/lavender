// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#include <cpu/cpu.hpp>

#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
    #include <ntstatus.h>
    #include <Powrprof.h>
#endif

#if defined(LAVENDER_COMPILER_MSVC)
    #include <intrin.h>
#elif defined(LAVENDER_COMPILER_GCC) || defined(LAVENDER_COMPILER_CLANG)
    #include <cpuid.h>
#endif

#include <nt.hpp>
#include <process/process.hpp>

#include <third_party/magic_enum.hpp>

static bool cpuid(const uint32_t leaf, const uint32_t subleaf, uint32_t *EAX, uint32_t *EBX, uint32_t *ECX, uint32_t *EDX)
{
#if defined(LAVENDER_COMPILER_MSVC)
    std::array<int, 4> registers;
    __cpuidex(registers.data(), leaf, subleaf);

    *EAX = registers[0];
    *EBX = registers[1];
    *ECX = registers[2];
    *EDX = registers[3];

    // __cpuidex() doesn't return anything indicating if the query was granted or not, therefore the return here is moot.
    return true;
#elif defined(LAVENDER_COMPILER_GCC) || defined(LAVENDER_COMPILER_CLANG)
    // Apparently LLVM is the same, returning 1 after __cpuid_count(), with failure only on (__max_leaf == 0 || __max_leaf < __leaf).
    // https://github.com/microsoft/clang/blob/master/lib/Headers/cpuid.h
    return __get_cpuid_count(leaf, subleaf, EAX, EBX, ECX, EDX);
#endif
}

namespace lavender {

namespace cpu {

std::string CPUInformation::ParseArchitectureBrand(const uint16_t architecture)
{
    switch (architecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x86-64";
        case PROCESSOR_ARCHITECTURE_ARM:   return "ARM";
        case PROCESSOR_ARCHITECTURE_ARM64: return "ARM64";
        case PROCESSOR_ARCHITECTURE_IA64:  return "Intel Itanium";
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
    }
    
    return "Unknown";
}

CPUCacheType CPUInformation::ParseCacheType(const uint32_t type)
{
    switch (type) {
        case 1:
            return CPUCacheType::D;
        case 2:
            return CPUCacheType::I;
        case 3:
            return CPUCacheType::Unified;
        case 0:
            // Purposefully ignore the end-of-cache enumeration value, which is zero (we've already discarded them when doing CPUID 04h)
#if defined(LAVENDER_COMPILER_GCC) || defined(LAVENDER_COMPILER_CLANG)
        case 4 ... 31:
#endif
        default:
            return CPUCacheType::Reserved;
    }
}

bool CPUInformation::ParseBasicInformation()
{
    ::SYSTEM_INFO info = {0};
    const auto is_WOW64 = platform::IsProcessWOW64(::GetCurrentProcess());

    if (is_WOW64.has_value()) {
        if (is_WOW64.value())
            ::GetNativeSystemInfo(&info);
        else
            ::GetSystemInfo(&info);
        
        {
            logical_cpus_ = info.dwNumberOfProcessors;
            architecture_ = info.wProcessorArchitecture;
            architecture_string_ = ParseArchitectureBrand(architecture_);

            // For Intel CPUs (with HTT enabled), this would most likely be enough as each physical core has 2 threads.
            // This is NOT always the case for other CPUs though (i. e. SPARC M7 has 8 threads/core)
            if (capabilities_[CPUCapabilities::HTT])
                cpus_ = logical_cpus_ / 2;
            else
                cpus_ = logical_cpus_;
            
            return true;
        }
    }

    return false;
}

bool CPUInformation::ParseCPUIDInformation()
{
    // Query for processor manufacturer ID strings (e. g. "GenuineIntel", "AuthenticAMD", "bhyve bhyve "...)
    if (const auto r = cpuid(0, 0, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        char buffer[13];
        std::memcpy(&buffer[0], &EBX, 4);
        std::memcpy(&buffer[0] + 4, &EDX, 4);
        std::memcpy(&buffer[0] + 8, &ECX, 4);
        buffer[12] = '\0';

        cpu_brand_ID_ = std::string(buffer);
    }

    // Query for basic processor properties (stepping, family (extended) ID, model (extended)...)
    if (const auto r = cpuid(1, 0, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        stepping_ = EAX & 0xF;
        model_ = (EAX >> 4) & 0xF;
        family_ = (EAX >> 8) & 0xF;
        cpu_type_ = (EAX >> 12) & 0x3;
        model_extended_ = (EAX >> 16) & 0xF;
        family_extended_ = (EAX >> 20) & 0xFF;
    }

    // Query for processor's brand name
    if (uint32_t brand[12]; 
        !(cpuid(0x80000002, 0, &brand[0], &brand[1], &brand[2], &brand[3]) &&
        cpuid(0x80000003, 0, &brand[4], &brand[5], &brand[6], &brand[7]) &&
        cpuid(0x80000004, 0, &brand[8], &brand[9], &brand[10], &brand[11]))) {
        return false;
    }
    else {
        cpu_brand_name_ = std::string((const char *) brand);
    }

    // For performance purposes, CPU frequencies will only be calculated once, also due to restrictions of user-mode programs, only 
    // the base frequency can be retrieved.
    ForceRefreshCPUFrequency();

    // Query for processor's capabilities
    for (const auto &capability : magic_enum::enum_values<CPUCapabilities>()) {
        capabilities_[capability] = false;
    }

    // Basic capabilities EAX = 1
    if (const auto r = cpuid(1, 0, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        if (EDX         & 0x1) capabilities_[CPUCapabilities::x87] = true;
        if ((EDX >>  3) & 0x1) capabilities_[CPUCapabilities::PSE] = true;
        if ((EDX >>  4) & 0x1) capabilities_[CPUCapabilities::TSC] = true;
        if ((EDX >>  6) & 0x1) capabilities_[CPUCapabilities::PAE] = true;
        if ((EDX >> 17) & 0x1) capabilities_[CPUCapabilities::PSE36] = true;
        if ((EDX >> 19) & 0x1) capabilities_[CPUCapabilities::CLFLUSH] = true;
        if ((EDX >> 23) & 0x1) capabilities_[CPUCapabilities::MMX] = true;
        if ((EDX >> 25) & 0x1) capabilities_[CPUCapabilities::SSE] = true;
        if ((EDX >> 26) & 0x1) capabilities_[CPUCapabilities::SSE2] = true;
        if ((EDX >> 28) & 0x1) capabilities_[CPUCapabilities::HTT] = true;

        if ((ECX      ) & 0x1) capabilities_[CPUCapabilities::SSE3] = true;
        if ((ECX >>  5) & 0x1) capabilities_[CPUCapabilities::VMX] = true;
        if ((ECX >>  7) & 0x1) capabilities_[CPUCapabilities::EST] = true;
        if ((ECX >>  9) & 0x1) capabilities_[CPUCapabilities::SSSE3] = true;
        if ((ECX >> 12) & 0x1) capabilities_[CPUCapabilities::FMA3] = true;
        if ((ECX >> 19) & 0x1) capabilities_[CPUCapabilities::SSE41] = true;
        if ((ECX >> 20) & 0x1) capabilities_[CPUCapabilities::SSE42] = true;
        if ((ECX >> 23) & 0x1) capabilities_[CPUCapabilities::POPCNT] = true;
        if ((ECX >> 25) & 0x1) capabilities_[CPUCapabilities::AES] = true;
        if ((ECX >> 26) & 0x1) capabilities_[CPUCapabilities::XSAVE] = true;
        if ((ECX >> 27) & 0x1) capabilities_[CPUCapabilities::OSXSAVE] = true;
        if ((ECX >> 28) & 0x1) capabilities_[CPUCapabilities::AVX] = true;
        if ((ECX >> 29) & 0x1) {
            capabilities_[CPUCapabilities::F16C] = true;
            capabilities_[CPUCapabilities::CVT16] = true;
        }
        if ((ECX >> 30) & 0x1) capabilities_[CPUCapabilities::RDRAND] = true;
    }

    // Extended capabilities EAX = 7, ECX = 0
    if (const auto r = cpuid(7, 0, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        if ((EBX >>  3) & 0x1) capabilities_[CPUCapabilities::BMI1] = true;
        if ((EBX >>  5) & 0x1) capabilities_[CPUCapabilities::AVX2] = true;
        if ((EBX >>  8) & 0x1) capabilities_[CPUCapabilities::BMI2] = true;
        if ((EBX >> 11) & 0x1) capabilities_[CPUCapabilities::TSX] = true;
        if ((EBX >> 23) & 0x1) capabilities_[CPUCapabilities::CLFLUSHOPT] = true;
        
        // If AVX512_F (foundation) isn't present, then the rest of AVX-512 extensions wouldn't present, too.
        if ((EBX >> 16) & 0x1) capabilities_[CPUCapabilities::AVX512_F] = true;
        if ((EBX >> 17) & 0x1) capabilities_[CPUCapabilities::AVX512_DQ] = true;
        if ((EBX >> 18) & 0x1) capabilities_[CPUCapabilities::RDSEED] = true;
        if ((EBX >> 21) & 0x1) capabilities_[CPUCapabilities::AVX512_IFMA] = true;
        if ((EBX >> 26) & 0x1) capabilities_[CPUCapabilities::AVX512_PF] = true;
        if ((EBX >> 27) & 0x1) capabilities_[CPUCapabilities::AVX512_ER] = true;
        if ((EBX >> 28) & 0x1) capabilities_[CPUCapabilities::AVX512_CD] = true;
        if ((EBX >> 30) & 0x1) capabilities_[CPUCapabilities::AVX512_BW] = true;
        if ((EBX >> 31) & 0x1) capabilities_[CPUCapabilities::AVX512_VL] = true;

        if ((ECX >>  1) & 0x1) capabilities_[CPUCapabilities::AVX512_VBMI] = true;
        if ((ECX >>  6) & 0x1) capabilities_[CPUCapabilities::AVX512_VBMI2] = true;
        if ((ECX >> 11) & 0x1) capabilities_[CPUCapabilities::AVX512_VNNI] = true;
        if ((ECX >> 12) & 0x1) capabilities_[CPUCapabilities::AVX512_BITALG] = true;
        if ((ECX >> 14) & 0x1) capabilities_[CPUCapabilities::AVX512_VPOPCNTDQ] = true;

        if ((EDX >>  2) & 0x1) capabilities_[CPUCapabilities::AVX512_4VNNIW] = true;
        if ((EDX >>  3) & 0x1) capabilities_[CPUCapabilities::AVX512_4FMAPS] = true;
        if ((EDX >>  8) & 0x1) capabilities_[CPUCapabilities::AVX512_VP2INTERSECT] = true;

        if ((EDX >> 22) & 0x1) capabilities_[CPUCapabilities::AMX_BF16] = true;
        if ((EDX >> 24) & 0x1) capabilities_[CPUCapabilities::AMX] = true;
        if ((EDX >> 25) & 0x1) capabilities_[CPUCapabilities::AMX_INT8] = true;
    }

    // Extended capabilities EAX = 7, ECX = 1
    if (const auto r = cpuid(7, 1, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        if ((EAX >>  5) & 0x1) capabilities_[CPUCapabilities::AVX512_BF16] = true;
    }

    // Additional capabilities EAX = 80000001h (NX, EXTENDED_MMX, RDTSCP, SSE4A)
    if (const auto r = cpuid(0x80000001, 0, &EAX, &EBX, &ECX, &EDX); !r) {
        return false;
    }
    else {
        if ((EDX >> 20) & 0x1) capabilities_[CPUCapabilities::NX] = true;
        if ((EDX >> 22) & 0x1) capabilities_[CPUCapabilities::EXTENDED_MMX] = true;
        if ((EDX >> 26) & 0x1) capabilities_[CPUCapabilities::PDPE1GB] = true;
        if ((EDX >> 27) & 0x1) capabilities_[CPUCapabilities::RDTSCP] = true;
        if ((EDX >> 29) & 0x1) capabilities_[CPUCapabilities::EM64T] = true;

        if ((ECX >>  6) & 0x1) capabilities_[CPUCapabilities::SSE4A] = true;
        if ((ECX >> 11) & 0x1) capabilities_[CPUCapabilities::XOP] = true;
        
        if ((ECX >>  2) & 0x1) capabilities_[CPUCapabilities::SVM] = true;
    }

    capabilities_[CPUCapabilities::SSE5] = 
        capabilities_[CPUCapabilities::XOP] &&
        capabilities_[CPUCapabilities::FMA4] &&
        (capabilities_[CPUCapabilities::CVT16] || capabilities_[CPUCapabilities::F16C]) &&
        capabilities_[CPUCapabilities::AVX] &&
        capabilities_[CPUCapabilities::XSAVE];
    
    // Query for cache information: line size, associativity & base size. Note that AMD does things differently.
    // See https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html, page 292 
    // (CPU Identification)
    for (int i = 0; i < 16; ++i) {
        if (cpuid(4, i, &EAX, &EBX, &ECX, &EDX)) {
            uint32_t type = EAX & 0x1F;

            if (type == 0) {
                break;
            }
            else {
                cache_.push_back(CPUCacheInformation(
                    ParseCacheType(type),
                    (EAX >> 5) & 0x7,
                    (EBX & 0xFFF) + 1,
                    ((EBX >> 12) & 0x3FF) + 1,
                    ((EBX >> 22) & 0x3FF) + 1,
                    ECX + 1
                ));
            }
        }
    }

    return true;
}

bool CPUInformation::ParseVendorTypeInformation()
{
    if (cpu_brand_ID_ == "GenuineIntel")
        vendor_type_ = CPUVendorType::Intel;
    else if (cpu_brand_ID_ == "AuthenticAMD" || cpu_brand_name_ == "AMDisbetter!")
        vendor_type_ = CPUVendorType::AMD;
    else
        vendor_type_ = CPUVendorType::Reserved;
    
    return true;
}

bool CPUInformation::ParseCLFLUSHLineSizeInformation()
{
    if (capabilities_[CPUCapabilities::CLFLUSH] && cpuid(1, 0, &EAX, &EBX, &ECX, &EDX)) {
        clflush_size_ = (EBX >> 8) & 0xFF;
    }

    return true;
}

bool CPUInformation::Initialize()
{
    if (!ready_) {
        ready_ = 
            ParseBasicInformation() &&
            ParseCPUIDInformation() &&
            ParseVendorTypeInformation() &&
            ParseCLFLUSHLineSizeInformation();
    }

    return ready_;
}

void CPUInformation::ForceRefreshCPUFrequency()
{   
    const auto buffer_size = sizeof(::PROCESSOR_POWER_INFORMATION) * cpus_;

    std::vector<::BYTE> buffer(buffer_size);
    frequencies_.clear();

    if (::CallNtPowerInformation(::POWER_INFORMATION_LEVEL::ProcessorInformation, nullptr, 0, &buffer[0], buffer_size) == STATUS_SUCCESS)
    {
        auto ptr = (::PPROCESSOR_POWER_INFORMATION) &buffer[0];
        for (auto i = 0; i < cpus_; ++i) {
            frequencies_.push_back(ptr->CurrentMhz);
            ++ptr;
        }
    }
}

}

}
