#pragma once

#include <string>
#include <filesystem>

class ZipUtils {
public:
    static void extract_all(std::string src, std::string dst);
};