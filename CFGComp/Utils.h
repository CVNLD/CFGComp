#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define CONFIG_SPACE_SIZE 256
#define MAX_PCI_BUS 256
#define MAX_PCI_DEVICE 32
#define MAX_PCI_FUNCTION 8

extern bool g_debug;

#define DEBUG_PRINT(...) DebugPrint(__VA_ARGS__)
#define DEBUG_PRINT_HEX(x) DEBUG_PRINT(L#x L" = 0x{:X}", (x))
#define DEBUG_PRINT_DEC(x) DEBUG_PRINT(L#x L" = {}", (x))

template<typename T>
std::wstring ConvertToWideString(const T& value) {
    std::wostringstream woss;
    woss << value;
    return woss.str();
}

// Specialization for std::string
template<>
inline std::wstring ConvertToWideString<std::string>(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

template<typename... Args>
void DebugPrint(const wchar_t* format, const Args&... args) {
    if (g_debug) {
        std::wostringstream woss;
        woss << L"[DEBUG] " << format;
        int unpack[] = { 0, (woss << ConvertToWideString(args), 0)... };
        (void)unpack;
        std::wcout << woss.str() << std::endl;
    }
}

template<typename T>
void DebugPrint(const T& value) {
    if (g_debug) {
        std::wcout << L"[DEBUG] " << ConvertToWideString(value) << std::endl;
    }
}

UINT16 SwapBytes(UINT16 value);
bool IsElevated();
std::wstring GetExecutablePath();
bool ExtractDriverFile(const std::wstring& outputPath);
void DumpConfigSpace(const std::vector<uint8_t>& configSpace);
std::vector<uint8_t> ReadPCIConfigSpace(class RTCoreDriver& driver, UINT8 bus, UINT8 device, UINT8 function);
std::wstring GetTempDriverPath();
bool ParsePCIAddress(const std::string& pciAddress, UINT8& bus, UINT8& device, UINT8& function);