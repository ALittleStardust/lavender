#include <iostream>
#include <cpu/cpu.hpp>

#include <third_party/magic_enum.hpp>

static constexpr const char n = '\n';

int main(int, const char *[])
{
    using std::cout;
    using std::endl;
    using std::flush;
    
    if (lavender::cpu::CPUInformation cpu; cpu.Initialize()) {
        cout << "GetBrandID(): " << cpu.GetBrandID() << " (of type " << magic_enum::enum_name(cpu.GetVendorType()) << ')' << n;
        cout << "GetBrandName(): " << cpu.GetBrandName() << n;
        cout << "GetCPUCount()/GetLogicalCPUCount(): " << cpu.GetCPUCount() << '/' << cpu.GetLogicalCPUCount() << n;
        {
          std::ios state(nullptr);
          state.copyfmt(std::cout);

          cout << std::hex << std::uppercase;
          cout << "GetSteppingID(): " << cpu.GetSteppingID() << n;
          cout << "GetModel(): " << cpu.GetModel() << n;
          cout << "GetFamilyID(): " << cpu.GetFamilyID() << n;
          cout << "GetProcessorType(): " << cpu.GetProcessorType() << n;
          cout << "GetModelExtended(): " << cpu.GetModelExtended() << n;
          cout << "GetFamilyIDExtended(): " << cpu.GetFamilyIDExtended() << n;
          
          std::cout.copyfmt(state);
        }
        cout << "GetArchitecture(): " << cpu.GetArchitecture() << n;
        cout << "GetArchitectureAsString(): " << cpu.GetArchitectureAsString() << n;
        cout << "GetFrequency(): ";
        for (const auto &cpu : cpu.GetFrequencyInformation()) {
          cout << cpu << ' ';
        }
        cout << n;

        cout << "GetCLFLUSHLineSize(): " << cpu.GetCLFLUSHLineSize() << " bytes" << n;
        
        cout << "GetCapabilities(): " << n;
        for (const auto &capability : cpu.GetCapabilities()) {
          cout << magic_enum::enum_name(capability.first) << ' ' << capability.second << n;
        }
        cout << n;

        cout << "GetCacheInformation(): " << n;
        for (const auto &cache : cpu.GetCacheInformation()) {
          cout << "Cache L" << cache.GetLevel() << ", of type " << magic_enum::enum_name(cache.GetType()) << ": " << n;
          cout << "----------------------------------" << n;
          cout << "GetLineSize(): " << cache.GetLineSize() << " bytes" << n;
          cout << "GetPhysicalLinePartitions(): " << cache.GetPhysicalLinePartitions() << n;
          cout << "GetWaysOfAssociativity(): " << cache.GetWaysOfAssociativity() << "-way" << n;
          cout << "GetSets(): " << cache.GetSets() << n;
          cout << "GetSize(): " << cache.GetSize() / 1024 << " KiB" << n;
          cout << "----------------------------------" << n;
        }
        cout << n;

        cout << "Intel virtualization flag: " << cpu.HasFeature(lavender::cpu::CPUCapabilities::VMX) << n;
        cout << "Intel SpeedStep flag: " << cpu.HasFeature(lavender::cpu::CPUCapabilities::EST) << n;
        cout << "AMD virtualization flag: " << cpu.HasFeature(lavender::cpu::CPUCapabilities::SVM) << n;

        cout << flush;
    }
}
