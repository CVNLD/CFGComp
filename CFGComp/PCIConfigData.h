#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct PCIConfigData {
    std::vector<uint8_t> data;
    uint16_t vendorId = 0;
    uint16_t deviceId = 0;
};

PCIConfigData ParseCOEFile(const std::string& filename);
void CompareConfigSpaces(const std::vector<uint8_t>& coeData, const std::vector<uint8_t>& deviceData);