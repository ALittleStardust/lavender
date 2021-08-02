// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#include <nt.hpp>
#include <os/version.hpp>

#include <string>
#include <optional>
#include <unordered_map>

#if defined(_WIN32)
    #include <windows.h>
    #include <ntstatus.h>
#endif

namespace lavender {

namespace os {

static const std::unordered_map<OSVersion, std::string> NT_version_code_table = {
    {OSVersion::NT_UNIDENTIFIED, "Unidentified"},
    {OSVersion::NT_2000, "2000"},
    {OSVersion::NT_XP, "XP"},
    {OSVersion::NT_XP_64, "XP x64"},
    {OSVersion::NT_VISTA_2008, "Vista/Server 2008"},
    {OSVersion::NT_7_2008_R2, "7/Server 2008 R2"},
    {OSVersion::NT_8_2012, "8/Server 2012"},
    {OSVersion::NT_8_1_2012_R2, "8.1/Server 2012 R2"},
    {OSVersion::NT_10_2016_2019, "10/Server 2016/Server 2019"}
};

// https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getproductinfo
static const std::unordered_map<uint32_t, std::string> NT_product_type_table = {
    {PRODUCT_BUSINESS, "Business"},
    {PRODUCT_BUSINESS_N, "Business N"},
    {PRODUCT_CORE, "10 Home"},
    {PRODUCT_CORE_COUNTRYSPECIFIC, "10 Home (China)"},
    {PRODUCT_CORE_N, "10 Home N"},
    {PRODUCT_CORE_SINGLELANGUAGE, "10 Home (Single Language)"},
    {PRODUCT_EDUCATION, "10 Education"},
    {PRODUCT_EDUCATION_N, "10 Education N"},
    {PRODUCT_ENTERPRISE, "10 Enterprise"},
    {PRODUCT_ENTERPRISE_E, "10 Enterprise E"},
    {PRODUCT_ENTERPRISE_N, "10 Enterprise N"},
    {PRODUCT_ENTERPRISE_EVALUATION, "10 Enterprise Evaluation"},
    {PRODUCT_ENTERPRISE_N_EVALUATION, "10 Enterprise N Evaluation"},
    {PRODUCT_ENTERPRISE_S, "10 Enterprise 2015 LTSB"},
    {PRODUCT_ENTERPRISE_S_EVALUATION, "10 Enterprise 2015 LTSB Evaluation"},
    {PRODUCT_ENTERPRISE_S_N, "10 Enterprise 2015 LTSB N"},
    {PRODUCT_ENTERPRISE_S_N_EVALUATION, "10 Enterprise 2015 LTSB N Evaluation"},
    {PRODUCT_STARTER, "Starter"},
    {PRODUCT_STARTER_E, "Starter E"},
    {PRODUCT_STARTER_N, "Starter N"},
    {PRODUCT_HOME_BASIC, "Home Basic"},
    {PRODUCT_HOME_BASIC_E, "Home Basic E"},
    {PRODUCT_HOME_BASIC_N, "Home Basic N"},
    {PRODUCT_HOME_PREMIUM, "Home Premium"},
    {PRODUCT_HOME_PREMIUM_E, "Home Premium E"},
    {PRODUCT_HOME_PREMIUM_N, "Home Premium N"},
    {PRODUCT_ULTIMATE, "Ultimate"},
    {PRODUCT_ULTIMATE_E, "Ultimate E"},
    {PRODUCT_ULTIMATE_N, "Ultimate N"},
    {PRODUCT_IOTUAP, "10 IoT Core"},
    {/* PRODUCT_IOTUAPCOMMERCIAL */ 0x00000083, "10 IoT Core Commercial"},
    {PRODUCT_MOBILE_CORE, "10 Mobile"},
    {PRODUCT_MOBILE_ENTERPRISE, "10 Mobile Enterprise"},
    {PRODUCT_PROFESSIONAL, "10 Pro"},
    {PRODUCT_PROFESSIONAL_E, "10 Pro E"},
    {PRODUCT_PROFESSIONAL_N, "10 Pro N"},
    {PRODUCT_SERVER_FOUNDATION, "Server Foundation"}
};

// As you'll be needing the DDK for proper RtlGetVersion() access, just fetch it from ntdll.dll.
// Note that RtlGetVersion() is the kernel-mode equivalent of GetVersionEx*() which is deprecated.
using RtlGetVersionPtr = NTSTATUS (WINAPI *)(PRTL_OSVERSIONINFOEXW);
static bool GetNTVersionInformation(RTL_OSVERSIONINFOEXW *r)
{
    const HMODULE module = ::GetModuleHandle("ntdll.dll");
    if (module != nullptr)
    {
        const RtlGetVersionPtr f = (RtlGetVersionPtr) ::GetProcAddress(module, "RtlGetVersion");
        if (f != nullptr && r != nullptr && f(r) == STATUS_SUCCESS) {
            r->dwOSVersionInfoSize = sizeof *r;
            return true;
        }
    }

    return false;
}

static OSVersion GetNTVersionCode(const uint32_t major, const uint32_t minor)
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
    if (major == 5 && minor == 0)
        return OSVersion::NT_2000;
    else if (major == 5 && minor == 1)
        return OSVersion::NT_XP;
    else if (major == 5 && minor == 2)
        return OSVersion::NT_XP_64;
    else if (major == 6 && minor == 0)
        return OSVersion::NT_VISTA_2008;
    else if (major == 6 && minor == 1)
        return OSVersion::NT_7_2008_R2;
    else if (major == 6 && minor == 2)
        return OSVersion::NT_8_2012;
    else if (major == 6 && minor == 3)
        return OSVersion::NT_8_1_2012_R2;
    else if (major == 10 && minor == 0)
        return OSVersion::NT_10_2016_2019;
    else
        return OSVersion::NT_UNIDENTIFIED;
}

static std::string GetNTProductType(const uint32_t type)
{
    if (const std::unordered_map<uint32_t, std::string>::const_iterator i = NT_product_type_table.find(type); i != NT_product_type_table.end()) {
        return std::string(i->second);
    }

    return "(?)";
}

bool OSVersionInformation::ParseVersionInformation()
{
    if (RTL_OSVERSIONINFOEXW r = {0}; GetNTVersionInformation(&r)) {
        version_ = GetNTVersionCode(r.dwMajorVersion, r.dwMinorVersion);
        build_number_ = r.dwBuildNumber;

        if (const auto ptr = NT_version_code_table.find(version_); ptr != NT_version_code_table.end())
            version_string_ = ptr->second;
        
        if (::DWORD product_type = PRODUCT_UNDEFINED; ::GetProductInfo(r.dwMajorVersion, r.dwMinorVersion, r.wServicePackMajor, r.wServicePackMinor, &product_type) != 0) {
            product_type_ = GetNTProductType(product_type);
        }
        
        return true;
    }

    return false;
}

bool OSVersionInformation::Initialize()
{
    ready_ = ParseVersionInformation();

    return ready_;
}

}

}
