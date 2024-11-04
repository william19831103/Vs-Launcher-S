#pragma once
#include <string>
#include <fstream>

class ConfigManager {
public:
    static void SaveConfig();
    static bool LoadConfig();
private:
    static std::string Trim(const std::string& str);
    static void WriteValue(std::ofstream& file, const std::string& key, const std::string& value);
    static std::string ReadValue(const std::string& line, const std::string& key);
}; 