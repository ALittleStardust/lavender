// Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.
#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace lavender {

namespace os {

enum class OSVersion : uint8_t {
    NT_UNIDENTIFIED,
    NT_2000,
    NT_XP,
    NT_XP_64,
    NT_VISTA_2008,
    NT_7_2008_R2,
    NT_8_2012,
    NT_8_1_2012_R2,
    NT_10_2016_2019
};

struct OSVersionInformation {
private:
    bool ready_ = false;
    OSVersion version_ = OSVersion::NT_UNIDENTIFIED;
    uint32_t build_number_ = 0;
    std::string version_string_ = {};
    std::string product_type_ = {};

    bool ParseVersionInformation();

public:
    OSVersionInformation() {}

    uint32_t GetBuildNumber() const { return build_number_; }
    OSVersion GetVersion() const { return version_; }
    const std::string &GetVersionAsString() const { return version_string_; }
    const std::string &GetProductType() const { return product_type_; }

    bool Is2000OrHigher() const { return version_ >= OSVersion::NT_2000; }
    bool IsXPOrHigher() const { return version_ >= OSVersion::NT_XP; }
    bool IsVistaOrHigher() const { return version_ >= OSVersion::NT_VISTA_2008; }
    bool Is7OrHigher() const { return version_ >= OSVersion::NT_7_2008_R2; }
    bool Is8OrHigher() const { return version_ >= OSVersion::NT_8_2012; }
    bool Is81OrHigher() const { return version_ >= OSVersion::NT_8_1_2012_R2; }
    bool Is10OrHigher() const { return version_ >= OSVersion::NT_10_2016_2019; }
    
    bool IsReady() const { return ready_; }
    bool Initialize();
};

}

}
