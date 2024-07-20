#pragma once
#include <Windows.h>
#include <string>

#define RTC64_DEVICE_NAME "\\\\.\\RTCore64"
#define RTC64_SERVICE_NAME L"RTCore64"
#define RTC64_IOCTL_READ_MEMORY 0x80002048
#define RTC64_IOCTL_WRITE_MEMORY 0x8000204C
#define RTC64_IOCTL_READ_PCI_CONFIG 0x80002050
#define RTC64_IOCTL_WRITE_PCI_CONFIG 0x80002054

typedef ULONG_PTR QWORD;

typedef struct {
    QWORD gap1;
    QWORD address;
    DWORD gap2;
    DWORD offset;
    DWORD size;
    DWORD data;
    uint8_t gap3[16];
} IO_STRUCT;

typedef struct {
    DWORD bus_number;
    DWORD slot_number;
    DWORD func_number;
    DWORD offset;
    DWORD length;
    DWORD result;
} IO_STRUCT_PCI;

class RTCoreDriver {
private:
    HANDLE hDevice;
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

public:
    RTCoreDriver();
    ~RTCoreDriver();

    bool Load(const std::wstring& driverPath);
    void Unload();
    bool Open();
    void Close();
    DWORD ReadMemory(QWORD address, DWORD size);
    bool WriteMemory(QWORD address, DWORD size, DWORD value);
    DWORD ReadPCIConfig(UINT8 bus, UINT8 device, UINT8 function, DWORD offset, DWORD length);
    bool WritePCIConfig(BYTE bus, BYTE slot, BYTE func, DWORD offset, DWORD value, DWORD length);
};