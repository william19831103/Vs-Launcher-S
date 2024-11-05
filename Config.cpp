#include "Config.h"
//#include "Auto.h"
//#include <sstream>
#include "ServerConnector.h"
#include <fstream>

void ConfigManager::SaveConfig() {
    std::ofstream file("config.ini");
    if (!file.is_open()) {
        return;
    }    

    file << "launcher_title=" << sServerInfo->LauncherTitle << std::endl;
    file << "serverport=" << sServerInfo->serverport << std::endl;
    file << "serverip=" << sServerInfo->MangosServerIP << std::endl;
    file << "MangosServerPort=" << sServerInfo->MangosServerPort << std::endl;
    //SOAP服务器IP
    file << "soapip=" << sServerInfo->soapIp << std::endl;
    //SOAP服务器端口
    file << "soapport=" << sServerInfo->soapPort << std::endl;
    //SOAP用户名
    file << "soapuser=" << sServerInfo->soapUser << std::endl;
    //SOAP密码
    file << "soappass=" << sServerInfo->soapPass << std::endl;
    
    file.close();
}

bool ConfigManager::LoadConfig() {
    std::ifstream file("config.ini");
    if (!file.is_open()) {
        return false;
    }

    
    std::string line;
    while (std::getline(file, line)) {
        std::string value = ReadValue(line, "launcher_title");
        if (!value.empty()) {
            sServerInfo->LauncherTitle = value;
        }
        value = ReadValue(line, "serverport");
        if (!value.empty()) {
            sServerInfo->serverport = std::stoi(value);
        }
        value = ReadValue(line, "serverip");
        if (!value.empty()) {
            sServerInfo->MangosServerIP = value;
        }
        value = ReadValue(line, "MangosServerPort");
        if (!value.empty()) {
            sServerInfo->MangosServerPort = std::stoi(value);
        }
        value = ReadValue(line, "soapip");
        if (!value.empty()) {
            sServerInfo->soapIp = value;
        }
        value = ReadValue(line, "soapport");
        if (!value.empty()) {
            sServerInfo->soapPort = std::stoi(value);
        }
        value = ReadValue(line, "soapuser");
        if (!value.empty()) {
            sServerInfo->soapUser = value;
        }
        value = ReadValue(line, "soappass");
        if (!value.empty()) {
            sServerInfo->soapPass = value;
        }
    }
    
    file.close();
    return true;
}

void ConfigManager::WriteValue(std::ofstream& file, const std::string& key, const std::string& value) {
    file << key << "=" << value << "\n";
}

std::string ConfigManager::ReadValue(const std::string& line, const std::string& key) {
    size_t pos = line.find(key + "=");
    if (pos == 0) {
        size_t valueStart = key.length() + 1; // +1 是为了跳过等号
        if (valueStart < line.length()) {
            return Trim(line.substr(valueStart));
        }
    }
    return "";
}

std::string ConfigManager::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last == std::string::npos) return "";
    return str.substr(first, last - first + 1);
} 