#include "file.h"
#include <vector>
#include <fstream>
#include <format>

std::vector<char> readBinaryFile(const std::string &fileName) {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if(!file.is_open()) {
        throw std::runtime_error(std::format("Unable to open file: {}", fileName));
    }

    size_t fileSize = file.tellg();
    std::vector<char> contents(fileSize);
    file.seekg(0);
    file.read(contents.data(), fileSize);
    file.close();
    return contents;
}