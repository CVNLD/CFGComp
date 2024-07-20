#include "RTCoreDriver.h"
#include "Utils.h"

RTCoreDriver::RTCoreDriver() : hDevice(INVALID_HANDLE_VALUE), schSCManager(NULL), schService(NULL) {}

RTCoreDriver::~RTCoreDriver() {
    Unload();
}

bool RTCoreDriver::Load(const std::wstring& driverPath) {
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        DEBUG_PRINT(L"OpenSCManager failed. Error: {}", GetLastError());
        return false;
    }

    schService = CreateServiceW(
        schSCManager,
        RTC64_SERVICE_NAME,
        RTC64_SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverPath.c_str(),
        NULL, NULL, NULL, NULL, NULL
    );

    if (schService == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            schService = OpenServiceW(schSCManager, RTC64_SERVICE_NAME, SERVICE_ALL_ACCESS);
        }
        else {
            DEBUG_PRINT(L"CreateService failed. Error: {}", error);
            CloseServiceHandle(schSCManager);
            return false;
        }
    }

    if (schService == NULL) {
        DEBUG_PRINT(L"OpenService failed. Error: {}", GetLastError());
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (!StartServiceW(schService, 0, NULL)) {
        DWORD error = GetLastError();
        if (error != ERROR_SERVICE_ALREADY_RUNNING) {
            DEBUG_PRINT(L"StartService failed. Error: {}", error);
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return false;
        }
    }

    return Open();
}

void RTCoreDriver::Unload() {
    Close();

    if (schService) {
        SERVICE_STATUS serviceStatus;
        ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);
        DeleteService(schService);
        CloseServiceHandle(schService);
        schService = NULL;
    }

    if (schSCManager) {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
}

bool RTCoreDriver::Open() {
    hDevice = CreateFileA(RTC64_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDevice == INVALID_HANDLE_VALUE) {
        DEBUG_PRINT(L"Error: CreateFileA failed with code {}", GetLastError());
        return false;
    }
    return true;
}

void RTCoreDriver::Close() {
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        hDevice = INVALID_HANDLE_VALUE;
    }
}

DWORD RTCoreDriver::ReadMemory(QWORD address, DWORD size) {
    IO_STRUCT operation{};
    operation.address = address;
    operation.size = size;
    if (!DeviceIoControl(hDevice, RTC64_IOCTL_READ_MEMORY, &operation, sizeof(operation),
        &operation, sizeof(operation), NULL, NULL)) {
        DEBUG_PRINT(L"Error: Memory Read IRP Failed with code {}", GetLastError());
        return 0;
    }
    return operation.data;
}

bool RTCoreDriver::WriteMemory(QWORD address, DWORD size, DWORD value) {
    IO_STRUCT operation{};
    operation.address = address;
    operation.size = size;
    operation.data = value;
    return DeviceIoControl(hDevice, RTC64_IOCTL_WRITE_MEMORY, &operation, sizeof(operation),
        &operation, sizeof(operation), NULL, NULL);
}

DWORD RTCoreDriver::ReadPCIConfig(UINT8 bus, UINT8 device, UINT8 function, DWORD offset, DWORD length) {
    IO_STRUCT_PCI io{};
    io.bus_number = static_cast<DWORD>(bus);
    io.slot_number = static_cast<DWORD>(device);
    io.func_number = static_cast<DWORD>(function);
    io.offset = offset;
    io.length = length;

    DWORD bytesReturned = 0;
    BOOL result = DeviceIoControl(hDevice, RTC64_IOCTL_READ_PCI_CONFIG, &io, sizeof(io),
        &io, sizeof(io), &bytesReturned, NULL);

    if (!result) {
        DWORD error = GetLastError();
        DEBUG_PRINT(L"[ERROR] ReadPCIConfig failed. Error code: {}", error);
        return 0xFFFFFFFF;
    }

    if (bytesReturned != sizeof(io)) {
        DEBUG_PRINT(L"[ERROR] ReadPCIConfig: Unexpected bytes returned. Expected: {} Actual: {}", sizeof(io), bytesReturned);
        return 0xFFFFFFFF;
    }

    return io.result;
}

bool RTCoreDriver::WritePCIConfig(BYTE bus, BYTE slot, BYTE func, DWORD offset, DWORD value, DWORD length) {
    IO_STRUCT_PCI io{};
    io.bus_number = bus;
    io.slot_number = slot;
    io.func_number = func;
    io.offset = offset;
    io.length = length;
    io.result = value;
    return DeviceIoControl(hDevice, RTC64_IOCTL_WRITE_PCI_CONFIG, &io, sizeof(io),
        &io, sizeof(io), NULL, NULL);
}