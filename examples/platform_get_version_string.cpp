#include <iostream>
#include <os/version.hpp>

int main(int, const char *[])
{
    if (lavender::os::OSVersionInformation version; version.Initialize()) {
        std::cout << "GetVersionAsString(): " << version.GetVersionAsString() << '\n';
        std::cout << "GetProductType(): " << version.GetProductType() << '\n';
        std::cout << "GetBuildNumber(): " << version.GetBuildNumber() << '\n';        
    }
}
