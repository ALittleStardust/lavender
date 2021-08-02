// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#pragma once

#if defined(_WIN32)
    #include <windows.h>
#endif

#include <compiler.hpp>

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

#if !defined(_PROCESSOR_POWER_INFORMATION)
typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;
#endif

namespace lavender {

namespace cpu {

enum class CPUCapabilities : uint16_t {
    MMX, EXTENDED_MMX, x87, PAE, PSE36, EM64T, // EXTENDED_MMX is AMD MMX
    CLFLUSH, // Flush Cache Line having the _mm_clflush() intrinsic
    CLFLUSHOPT,
    SSE, SSE2, SSE3, SSSE3, SSE4A, SSE41, SSE42, SSE5,
    AVX, AVX2, 
    AVX512_F, AVX512_DQ, AVX512_IFMA, AVX512_PF, AVX512_ER, AVX512_CD, AVX512_BW, AVX512_VL,
    AVX512_4VNNIW, AVX512_4FMAPS, AVX512_VP2INTERSECT, AVX512_BF16, AVX512_VBMI, AVX512_VBMI2,
    AVX512_VNNI, AVX512_BITALG, AVX512_VPOPCNTDQ,
    FMA3, FMA4,
    BMI1, BMI2,
    AMX, AMX_INT8, AMX_BF16, // Intel AMX
    XOP, XSAVE, OSXSAVE, CVT16, // CVT16 = F16C, which it was originally called
    TSC, RDTSCP, 
    POPCNT, HTT, RDRAND, RDSEED, F16C, NX,
    TSX, // Restricted Transactional Memory
    EST, // Enhanced SpeedStep
    SHA, AES,
    SVM, // Secure Virtual Machine, e. g. AMD virtualization flag
    VMX, // Intel virtualization flag
    PSE, // Large page size (2/4 MiB). 4 MiB by default, and 2 MiB when PAE is enabled.
    PDPE1GB // Large page size (1 GiB) support, present in Westmere Intel CPUs going forward. However this isn't completely reliable, e. g. 
            // Sandy Bridge does NOT support PDPE1GB.
};

enum class CPUCacheType : uint8_t { I, D, Unified, Reserved };

enum class CPUVendorType : uint8_t { AMD, Intel, Reserved };

struct CPUCacheInformation {
private:
    CPUCacheType type_ = CPUCacheType::Reserved;
    uint32_t level_;
    uint32_t line_size_ = 0;
    uint32_t physical_line_partitions_ = 0;
    uint32_t ways_ = 0;
    uint32_t sets_ = 0;
    uint32_t size_ = 0;

    void CalculateCacheSize()
    {
        size_ = ways_ * physical_line_partitions_ * line_size_ * sets_;
    }

public:
    CPUCacheInformation(CPUCacheType type, uint32_t level, uint32_t line_size, uint32_t physical_line_partitions, uint32_t ways, uint32_t sets) :
        type_(type)
      , level_(level)
      , line_size_(line_size)
      , physical_line_partitions_(physical_line_partitions)
      , ways_(ways)
      , sets_(sets)
    {
        CalculateCacheSize();
    }

    CPUCacheType GetType() const { return type_; };
    uint32_t GetLevel() const { return level_; }
    uint32_t GetLineSize() const { return line_size_; }
    uint32_t GetPhysicalLinePartitions() const { return physical_line_partitions_; }
    uint32_t GetWaysOfAssociativity() const { return ways_; }
    uint32_t GetSets() const { return sets_; }
    uint32_t GetSize() const { return size_; }
};

class CPUInformation {
private:
    typedef const std::string & string_cref;

    bool ready_ = false;

    uint32_t cpus_ = 0;
    uint32_t logical_cpus_ = 0;
    uint16_t architecture_ = PROCESSOR_ARCHITECTURE_UNKNOWN;
    uint32_t stepping_ = 0;
    uint32_t model_ = 0;
    uint32_t family_ = 0;
    uint32_t cpu_type_ = 0;
    uint32_t model_extended_ = 0;
    uint32_t family_extended_ = 0;
    uint32_t clflush_size_ = 0;

    std::string architecture_string_ = {};
    std::string cpu_brand_ID_ = {};
    std::string cpu_brand_name_ = {};

    std::vector<uint64_t> frequencies_ = {};
    std::unordered_map<CPUCapabilities, bool> capabilities_ = {};
    std::vector<CPUCacheInformation> cache_ = {};

    CPUVendorType vendor_type_ = CPUVendorType::Reserved;
    
    uint32_t EAX, EBX, ECX, EDX;

    std::string ParseArchitectureBrand(const uint16_t architecture);
    CPUCacheType ParseCacheType(const uint32_t type);

    bool ParseBasicInformation();
    bool ParseCPUIDInformation();
    bool ParseVendorTypeInformation();
    bool ParseCLFLUSHLineSizeInformation();
    
public:
    CPUInformation() {}

    bool IsReady() const { return ready_; }

    uint32_t GetCPUCount() const { return cpus_; }
    uint32_t GetLogicalCPUCount() const { return logical_cpus_; }
    uint32_t GetSteppingID() const { return stepping_; }
    uint32_t GetModel() const { return model_; }
    uint32_t GetFamilyID() const { return family_; }
    uint32_t GetProcessorType() const { return cpu_type_; }
    uint32_t GetModelExtended() const { return model_extended_; }
    uint32_t GetFamilyIDExtended() const { return family_extended_; }
    uint32_t GetCLFLUSHLineSize() const { return clflush_size_; }

    uint16_t GetArchitecture() const { return architecture_; }

    bool HasFeature(const CPUCapabilities &capability) const { return capabilities_.find(capability) != capabilities_.end(); }

    CPUVendorType GetVendorType() const { return vendor_type_; }

    string_cref GetBrandID() const { return cpu_brand_ID_; }
    string_cref GetBrandName() const { return cpu_brand_name_; }
    string_cref GetArchitectureAsString() const { return architecture_string_; }

    const std::vector<uint64_t> &GetFrequencyInformation() const { return frequencies_; }

    const std::unordered_map<CPUCapabilities, bool> &GetCapabilities() const { return capabilities_; }

    const std::vector<CPUCacheInformation> &GetCacheInformation() const { return cache_; };

    bool Initialize();
    void ForceRefreshCPUFrequency();
};

}

}
