#include <nt.hpp>
#include <os/os.hpp>

#include <third_party/magic_enum.hpp>

#if defined(_WIN32)
    #include <winbase.h>
#endif

#include <iostream>
#include <vector>

static constexpr const char n = '\n';

static std::string ConvertPriorityClass(const uint32_t priority)
{
    switch (priority) {
        case ABOVE_NORMAL_PRIORITY_CLASS: return "ABOVE_NORMAL_PRIORITY_CLASS";
        case BELOW_NORMAL_PRIORITY_CLASS: return "BELOW_NORMAL_PRIORITY_CLASS";
        case HIGH_PRIORITY_CLASS: return "HIGH_PRIORITY_CLASS";
        case IDLE_PRIORITY_CLASS: return "IDLE_PRIORITY_CLASS";
        case NORMAL_PRIORITY_CLASS: return "NORMAL_PRIORITY_CLASS";
        case REALTIME_PRIORITY_CLASS: return "REALTIME_PRIORITY_CLASS";
        default: return "(?)";
    }
}

static std::wstring GetUserNameFromSIDString(const std::string &SID, /* const */ lavender::os::OSInformation &os)
{
    if (os.IsReady() && os.TakeSnapshot(lavender::os::SnapshotType::Users)) {
        for (const lavender::os::UserSnapshot &user : os.GetSystemSnapshot().GetUsers()) {
            if (user.GetSIDAsString() == SID) {
                return user.GetName();
            }
        }
    }

    return L"(?)";
}

int main(int, const char *argv[])
{
    using std::cout;
    using std::wcout;
    using std::endl;
    using std::flush;

    if (!lavender::platform::IsProcessElevated(::GetCurrentProcess())) {
        cout << "[Warning]" << n;
        cout << argv[0] << " wasn't run under Administrator privileges. The following features will NOT run properly: " << n;
        cout << "* User rights enumeration" << n << n;
    }
    
    if (lavender::os::OSInformation os; os.Initialize()) {
        if (const lavender::os::OSVersionInformation &version = os.GetVersionInformation(); version.IsReady()) {
            cout << "GetBuildNumber(): " << version.GetBuildNumber() << n;
            cout << "GetVersion(): " << magic_enum::enum_name(version.GetVersion()) << n;
            cout << "GetVersionAsString(): " << version.GetVersionAsString() << n;
            cout << "GetProductType(): " << version.GetProductType() << n;
        }
        
        cout << "GetArchitecture(): " << os.GetArchitecture() << "-bit" << n;
        cout << "GetPageFileSize(): " << os.GetPageFileSize() / (uint16_t) 1024u << " KiB" << n;
        wcout << "GetLocale(): " << os.GetLocale() << n;

        cout << "GetComputerName():\n";
        for (const auto &name : os.GetComputerName())
            cout << ' ' << ' ' << name.first << ' ' << name.second << n;

        cout << "GetEnvironmentStrings():\n";
        for (auto &str : os.GetEnvironmentStrings())
            cout << ' ' << ' ' << str.first << " = " << str.second << n;
        
        cout << "GetRegisteredProductKey(): " << os.GetRegisteredProductKey() << n;
        cout << "IsGenuine(): " << os.IsGenuine() << n;

        if (os.TakeSnapshot(lavender::os::SnapshotType::Everything)) {
            const auto snapshot = os.GetSystemSnapshot();

            for (const auto &service : snapshot.GetServices()) {
                cout << service.GetName() << " (" << magic_enum::enum_name(service.GetType()) << ", " << magic_enum::enum_name(service.GetStatus()) << ")\n";
            }
            
            for (const auto &process : snapshot.GetProcesses()) {
                cout << process.GetName() << " (ID: " << process.GetID() << " (parent: " << process.GetParentID() << "), priority: " << ConvertPriorityClass(process.GetPriority()) << ", threads: " << process.GetThreadCount() << ")" << n;
                wcout << L"  Owner: " << GetUserNameFromSIDString(process.GetOwnerSID(), os) << n;

                const auto &memory_usage_state = process.GetMemoryState();
                cout << "  Using " << memory_usage_state.GetPrivateWorkingSetUsage() / (::SIZE_T) 1024 << " KiB (private working set)" << n;
                cout << "  Using " << memory_usage_state.GetSharedWorkingSetUsage() / (::SIZE_T) 1024 << " KiB (shared working set)" << n;
                cout << "  PFs: " << memory_usage_state.GetPageFaultCount() << n;

                for (const auto &module : process.GetModules()) {
                    cout << "  GetName(): " << module.GetName() << n;
                    cout << "  GetImagePath(): " << module.GetImagePath() << n;
                    cout << "  GetParentID(): " << module.GetParentID() << n;
                }
                cout << n << n;
            }
            
            cout << "Users: " << snapshot.GetUsers().size() << n;
            for (const auto &user : snapshot.GetUsers()) {
                wcout << L"User: " << user.GetName() << n;
                wcout << L"  IsActive() = " << user.IsActive() << n;
                wcout << L"  IsCurrentUser() = " << user.IsCurrentUser() << n;
                wcout << L"  GetFullName() = " << user.GetFullName() << n;
                wcout << L"  GetDescription() = " << user.GetDescription() << n;
                wcout << L"  GetRelativeID() = " << user.GetRelativeID() << n;
                cout << "  GetSIDAsString() = " << user.GetSIDAsString() << n;
                cout << "  GetPrivilege() = " << magic_enum::enum_name(user.GetPrivilege()) << n;
                cout << "  GetLastLoginTimeAsString() = " << user.GetLastLoginTimeAsString() << n;
                cout << "  GetLastLogoutTimeAsString() = " << user.GetLastLogoutTimeAsString() << n;

                cout << "  GetRights():" << n;
                if (const auto rights = user.GetAvailableRights(); rights.size() > 0) {
                    for (const auto &right : rights) {
                        cout << "    " << magic_enum::enum_name(right.GetType()) << ' ' << (right.IsEnabled() ? "(enabled)" : "(disabled)") << n;
                    }
                }
                else {
                    cout << "    -" << n;
                }

                cout << "  GetGroups():" << n;
                if (const auto groups = user.GetGroups(); groups.size() > 0) {
                    for (const auto &group : groups) {
                        wcout << "    " << group << n;
                    }
                }
                else {
                    cout << "    -" << n;
                }

                cout << n;
            }

            for (const auto &software : snapshot.GetInstalledSoftware()) {
                wcout << L"Name = " << software.GetName() << n;
                wcout << L"  GUID = " << software.GetGUID() << n;
                wcout << L"  Version = " << software.GetVersion() << n;
                wcout << L"  Publisher = " << software.GetPublisher() << n;
                wcout << L"  Description = " << software.GetDescription() << n;
                wcout << L"  Contact = " << software.GetContact() << n;
                wcout << L"  Estimated Size = " << software.GetEstimatedSize() / 1024u << " KiB" << n;
                wcout << L"  Installed Date = " << software.GetInstallDate() << n;
                wcout << L"  Installed Path = " << software.GetInstalledPath() << n;
                wcout << L"  Uninstaller Path = " << software.GetUninstallerPath() << n;
                wcout << L"  Installer Source Path = " << software.GetInstallerSourcePath() << n;
                wcout << L"  About URL = " << software.GetAboutURL() << n;
                wcout << L"  Can Modify = " << software.CanModify() << "; Can Repair = " << software.CanRepair() << "; Can Uninstall = " << software.CanUninstall() << n;
                wcout << n;
            }
        }

        cout << "GetPaths():" << n;
        for (const auto &path : os.GetPaths()) {
            cout << magic_enum::enum_name(path.first) << " = ";
            wcout << path.second << n;
        }
        cout << n;
        
        cout << flush;
    }
}

