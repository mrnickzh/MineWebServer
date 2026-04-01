#pragma once

#include <vector>
#include <ranges>
#include <string>

class StrSplit {
public:
    static std::vector<std::string> str_split(const std::string& str, std::string delimiter) {
        std::vector<std::string> tokens;
        for (const auto& view : std::views::split(str, delimiter)) {
            tokens.push_back(std::string(view.begin(), view.end()));
        }
        return tokens;
    }
};
