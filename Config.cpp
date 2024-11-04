#include "Config.h"
#include "Auto.h"
#include <sstream>
#include "ServerConnector.h"
#include <fstream>
#include <locale>
#include <codecvt>

void ConfigManager::SaveConfig() {
    std::ofstream file("config.ini");
    if (!file.is_open()) {
        return;
    }
    
    // 设置文件流为 UTF-8 编码
    file.imbue(std::locale(file.getloc(),
        new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>));
    
    file << "launcher_title=" << sServerInfo->LauncherTitle << std::endl;
    
    file.close();
}

bool ConfigManager::LoadConfig() {
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        return false;
    }
    
    // 设置文件流为 UTF-8 编码
    file.imbue(std::locale(file.getloc(),
        new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("launcher_title=") == 0) {
            sServerInfo->LauncherTitle = line.substr(14); // 14是"launcher_title="的长度
        }
    }
    
    file.close();
    return true;
}

void ConfigManager::WriteValue(std::ofstream& file, const std::string& key, const std::string& value) {
    file << key << "=" << value << "\n";
}

std::string ConfigManager::ReadValue(const std::string& line, const std::string& key) {
    if (line.find(key + "=") == 0) {
        return Trim(line.substr(key.length() + 1));
    }
    return "";
}

std::string ConfigManager::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last == std::string::npos) return "";
    return str.substr(first, last - first + 1);
} 