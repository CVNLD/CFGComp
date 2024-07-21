#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include "RTCoreDriver.h"
#include "PCIConfigData.h"
#include "Utils.h"

constexpr auto IDR_RTCORE64_SYS = 101;

int main(int argc, char* argv[]) {
    std::string coeFile;
    std::string pciAddress;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            g_debug = true;
        }
        else if (coeFile.empty()) {
            coeFile = argv[i];
        }
        else if (pciAddress.empty()) {
            pciAddress = argv[i];
        }
    }

    DEBUG_PRINT(L"Program started");

    if (coeFile.empty() || pciAddress.empty()) {
        std::wcerr << L"Usage: " << argv[0] << L" <coe_file> <bus:device:function> [--debug]" << std::endl;
        std::wcerr << L"Example: " << argv[0] << L" file.coe 13:0:0 --debug" << std::endl;
        return 1;
    }

    if (!IsElevated()) {
        printf("[ERROR] This program requires administrator privileges.\n");
        return 1;
    }
    DEBUG_PRINT(L"Running with administrator privileges");

    DEBUG_PRINT(L"COE file: {}", coeFile);
    DEBUG_PRINT(L"Raw PCI Address input: {}", pciAddress);

    PCIConfigData coeData = ParseCOEFile(coeFile);
    if (coeData.data.empty()) {
        DEBUG_PRINT(L"[ERROR] Failed to parse COE file or file is empty.");
        return 1;
    }

    DEBUG_PRINT(L"COE data size: {} bytes", coeData.data.size());

    UINT8 bus = 0, device = 0, function = 0;
    if (!ParsePCIAddress(pciAddress, bus, device, function)) {
        return 1;
    }

    std::wstring driverPath = GetTempDriverPath();
    if (driverPath.empty()) {
        return 1;
    }

    DEBUG_PRINT(L"Attempting to extract driver to: {}", driverPath);

    if (!ExtractDriverFile(driverPath)) {
        DEBUG_PRINT(L"[ERROR] Failed to extract embedded driver file.");
        return 1;
    }

    DEBUG_PRINT(L"Driver extracted successfully. Attempting to load...");

    RTCoreDriver driver;
    if (!driver.Load(driverPath)) {
        DEBUG_PRINT(L"[ERROR] Failed to load RTCore driver.");
        DeleteFileW(driverPath.c_str());
        return 1;
    }

    std::vector<uint8_t> deviceData = ReadPCIConfigSpace(driver, bus, device, function);
    if (deviceData.empty()) {
        DEBUG_PRINT(L"[ERROR] Failed to read PCIe config space.");
        driver.Unload();
        return 1;
    }

    DEBUG_PRINT(L"Device data size: {} bytes", deviceData.size());

    if (g_debug) {
        DumpConfigSpace(deviceData);
    }

    UINT16 readVendorId = *reinterpret_cast<UINT16*>(&deviceData[0]);
    UINT16 readDeviceId = *reinterpret_cast<UINT16*>(&deviceData[2]);

    DEBUG_PRINT(L"Raw Read Vendor ID: 0x{:X}, Device ID: 0x{:X}", readVendorId, readDeviceId);

    if (readVendorId == 0xFFFF && readDeviceId == 0x0000) {
        std::wcout << L"Error: The specified device does not exist." << std::endl;
        std::wcout << L"Please check the PCI address (bus:device:function) and try again." << std::endl;
        driver.Unload();
        return 1;
    }

    std::wcout << L"Device Information:" << std::endl;
    std::wcout << L"Vendor ID: 0x" << std::hex << readVendorId
        << L", Device ID: 0x" << readDeviceId << std::dec << std::endl;

    // Compare the configuration spaces
    DEBUG_PRINT(L"Starting comparison of config spaces");
    DEBUG_PRINT(L"COE data size: {}, Device data size: {}", coeData.data.size(), deviceData.size());

    CompareConfigSpaces(coeData.data, deviceData);

    DEBUG_PRINT(L"Comparison completed");

    DEBUG_PRINT(L"Program completed successfully");

    driver.Unload();
    if (!DeleteFileW(driverPath.c_str())) {
        DEBUG_PRINT(L"Failed to delete temporary driver file. Error code: {}", GetLastError());
    }
    return 0;
}