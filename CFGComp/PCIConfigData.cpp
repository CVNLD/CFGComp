#include "PCIConfigData.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <Windows.h>

PCIConfigData ParseCOEFile(const std::string& filename) {
    std::cout << "Parsing COE file: " << filename << std::endl;
    std::ifstream file(filename);
    PCIConfigData result;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open COE file." << std::endl;
        return result;
    }

    bool foundVector = false;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;

        if (line.empty() || line[0] == ';') {
            continue;
        }

        if (line.find("memory_initialization_vector=") != std::string::npos) {
            foundVector = true;
            size_t pos = line.find("=");
            if (pos != std::string::npos && pos + 1 < line.length()) {
                line = line.substr(pos + 1);
            }
            else {
                continue;
            }
        }

        if (foundVector) {
            std::istringstream iss(line);
            std::string token;
            while (std::getline(iss, token, ',')) {
                token.erase(std::remove_if(token.begin(), token.end(), isspace), token.end());
                if (!token.empty()) {
                    try {
                        DWORD value = std::stoul(token, nullptr, 16);
                        result.data.push_back(static_cast<uint8_t>(value & 0xFF));
                        result.data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
                        result.data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
                        result.data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ERROR] Failed to parse token: " << token << ". Error: " << e.what() << std::endl;
                    }
                }
            }
        }
    }

    std::cout << "COE file parsing complete. Lines read: " << lineCount << std::endl;
    std::cout << "COE data size: " << result.data.size() << " bytes" << std::endl;

    if (result.data.size() >= 4) {
        result.deviceId = (static_cast<uint16_t>(result.data[0]) << 8) | result.data[1];
        result.vendorId = (static_cast<uint16_t>(result.data[2]) << 8) | result.data[3];
        std::cout << "Extracted Vendor ID: 0x" << std::hex << result.vendorId
            << ", Device ID: 0x" << result.deviceId << std::dec << std::endl;
    }

    return result;
}

void CompareConfigSpaces(const std::vector<uint8_t>& coeData, const std::vector<uint8_t>& deviceData) {
    DEBUG_PRINT(L"Entering CompareConfigSpaces function");
    DEBUG_PRINT(L"COE data size: {}, Device data size: {}", coeData.size(), deviceData.size());

    std::wcout << L"Comparing config spaces" << std::endl;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD defaultAttributes = consoleInfo.wAttributes;

    int mismatches = 0;

    for (size_t i = 0; i < CONFIG_SPACE_SIZE && i < coeData.size() && i < deviceData.size(); i += 4) {
        std::wcout << std::setfill(L'0') << std::setw(2) << std::hex << (i / 16) << std::setw(1) << (i % 16) << L"   ";

        for (size_t j = 0; j < 4 && (i + j) < coeData.size() && (i + j) < deviceData.size(); ++j) {
            uint8_t coeValue = coeData[i + (3 - j)];
            uint8_t deviceValue = deviceData[i + j];

            if (coeValue == deviceValue) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            }
            else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                ++mismatches;
            }
            std::wcout << std::setfill(L'0') << std::setw(2) << std::hex << static_cast<int>(deviceValue) << L" ";
            SetConsoleTextAttribute(hConsole, defaultAttributes);
        }

        if ((i + 4) % 16 == 0 || i + 4 >= CONFIG_SPACE_SIZE) {
            std::wcout << std::endl;
        }
    }

    if (mismatches == 0) {
        std::wcout << L"Congratulations, CFG Space is 1:1 with input coe file." << std::endl;
    }
    else {
        std::wcout << std::dec << mismatches << L" bytes didn't match." << std::endl;
    }

    DEBUG_PRINT(L"Exiting CompareConfigSpaces function");
}