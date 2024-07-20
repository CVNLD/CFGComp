#include "Utils.h"
#include "RTCoreDriver.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

bool g_debug = false;

UINT16 SwapBytes(UINT16 value) {
    return (value >> 8) | (value << 8);
}

bool IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

std::wstring GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    size_t pos = ws.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? ws.substr(0, pos) : ws;
}

bool ExtractDriverFile(const std::wstring& outputPath) {
    HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(101), RT_RCDATA);
    if (hResInfo == NULL) {
        DWORD error = GetLastError();
        DEBUG_PRINT(L"Failed to find embedded driver resource. Error code: {}", error);
        return false;
    }

    HGLOBAL hResData = LoadResource(NULL, hResInfo);
    if (hResData == NULL) {
        DWORD error = GetLastError();
        DEBUG_PRINT(L"Failed to load embedded driver resource. Error code: {}", error);
        return false;
    }

    DWORD size = SizeofResource(NULL, hResInfo);
    LPVOID pDriverData = LockResource(hResData);

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        DEBUG_PRINT(L"Failed to create output file for driver: {}", outputPath);
        return false;
    }

    outFile.write(static_cast<const char*>(pDriverData), size);
    if (!outFile) {
        DEBUG_PRINT(L"Failed to write data to output file. Size: {}", size);
        outFile.close();
        return false;
    }

    outFile.close();

    std::ifstream verifyFile(outputPath, std::ios::binary);
    if (!verifyFile) {
        DEBUG_PRINT(L"Failed to open the created file for verification: {}", outputPath);
        return false;
    }

    verifyFile.seekg(0, std::ios::end);
    std::streampos fileSize = verifyFile.tellg();
    verifyFile.close();

    if (fileSize != size) {
        DEBUG_PRINT(L"Created file size ({}) does not match resource size ({})", fileSize, size);
        return false;
    }

    DEBUG_PRINT(L"Driver file extracted successfully to: {}", outputPath);
    return true;
}

void DumpConfigSpace(const std::vector<uint8_t>& configSpace) {
    DEBUG_PRINT(L"Dumping config space:");
    for (size_t i = 0; i < configSpace.size(); i += 16) {
        std::wostringstream woss;
        woss << std::hex << std::setw(4) << std::setfill(L'0') << i << L": ";
        for (size_t j = 0; j < 16 && i + j < configSpace.size(); ++j) {
            woss << std::setw(2) << std::setfill(L'0') << static_cast<int>(configSpace[i + j]) << L" ";
        }
        DEBUG_PRINT(woss.str());
    }
}

std::vector<uint8_t> ReadPCIConfigSpace(RTCoreDriver& driver, UINT8 bus, UINT8 device, UINT8 function) {
    std::vector<uint8_t> configSpace(CONFIG_SPACE_SIZE, 0);

    DEBUG_PRINT(L"ReadPCIConfigSpace input - Bus: 0x{:X}, Device: 0x{:X}, Function: 0x{:X}",
        static_cast<int>(bus), static_cast<int>(device), static_cast<int>(function));

    for (UINT32 offset = 0; offset < CONFIG_SPACE_SIZE; offset += 4) {
        DWORD data = driver.ReadPCIConfig(bus, device, function, offset, 4);

        DEBUG_PRINT(L"Offset 0x{:02X}: 0x{:08X}", offset, data);

        configSpace[offset] = data & 0xFF;
        configSpace[offset + 1] = (data >> 8) & 0xFF;
        configSpace[offset + 2] = (data >> 16) & 0xFF;
        configSpace[offset + 3] = (data >> 24) & 0xFF;
    }

    return configSpace;
}

std::wstring GetTempDriverPath() {
    std::wstring tempPath(MAX_PATH, L'\0');
    DWORD pathLength = GetTempPathW(MAX_PATH, &tempPath[0]);
    if (pathLength == 0 || pathLength > MAX_PATH) {
        DEBUG_PRINT(L"[ERROR] Failed to get temp path. Error code: {}", GetLastError());
        return L"";
    }
    tempPath.resize(pathLength);
    return tempPath + L"RTCore64.sys";
}

bool ParsePCIAddress(const std::string& pciAddress, UINT8& bus, UINT8& device, UINT8& function) {
    std::istringstream iss(pciAddress);
    std::string segment;

    try {
        if (std::getline(iss, segment, ':')) {
            bus = static_cast<UINT8>(std::stoul(segment, nullptr, 16));
        }
        if (std::getline(iss, segment, ':')) {
            device = static_cast<UINT8>(std::stoul(segment, nullptr, 16));
        }
        if (std::getline(iss, segment)) {
            function = static_cast<UINT8>(std::stoul(segment, nullptr, 16));
        }
    }
    catch (const std::exception& e) {
        DEBUG_PRINT(L"[ERROR] Failed to parse PCI address: {}", e.what());
        return false;
    }

    if (bus > 255 || device > 31 || function > 7) {
        DEBUG_PRINT(L"[ERROR] Invalid PCI address values. Bus (0-255), Device (0-31), Function (0-7)");
        return false;
    }

    DEBUG_PRINT(L"Parsed PCI Address: {:X}:{:X}:{:X}", static_cast<int>(bus), static_cast<int>(device), static_cast<int>(function));
    return true;
}