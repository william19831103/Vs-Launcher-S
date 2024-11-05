#include "Auto.h"
#include "ServerConnector.h"
#include <thread>
#include "Config.h"
#include <string>

extern int start_server(bool start);

void MainWindow() 
{
    static bool open = true;

    if (open) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);         
     
        ImGui::Begin("魔兽世界服务端", &open,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );

        ImVec2 windowSize = ImGui::GetWindowSize();
        
        // 服务器配置区域
        ImGui::BeginGroup();
        {
            // 登录器端口
            ImGui::Text("登录器端口:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            if (ImGui::InputInt("##LauncherPort", &sServerInfo->serverport, 0, 0)) 
            {
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

            // WOW服务器IP
            ImGui::Text("WOW服务器IP:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            static char bufferIP[256];
            static bool bufferIPinitialized = false;
            if (!bufferIPinitialized) {
                strncpy(bufferIP, sServerInfo->MangosServerIP.c_str(), sizeof(bufferIP) - 1);
                bufferIP[sizeof(bufferIP) - 1] = '\0';  // 确保字符串正确终止
                bufferIPinitialized = true;
            }
            if (ImGui::InputText("##ServerIP", bufferIP, sizeof(bufferIP))) {
                sServerInfo->MangosServerIP = bufferIP;
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

            // WOW服务器端口
            ImGui::Text("WOW端口号:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            ImGui::InputInt("##ServerPort", &sServerInfo->MangosServerPort, 0, 0);
            ImGui::PopItemWidth();

            // 确保端口号在有效范围内
            if (sServerInfo->MangosServerPort < 1) sServerInfo->MangosServerPort = 1;
            if (sServerInfo->MangosServerPort > 65535) sServerInfo->MangosServerPort = 65535;

            // 登录器标题
            ImGui::Text("登录器标题:");
            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            static char buffer[256];
            static bool initialized = false;
            if (!initialized) {
                strncpy(buffer, sServerInfo->LauncherTitle.c_str(), sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';  // 确保字符串正确终止
                initialized = true;
            }
            if (ImGui::InputText("##ServerName", buffer, sizeof(buffer)))
            {
                sServerInfo->LauncherTitle = buffer;
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

            // SOAP服务器IP
            ImGui::Text("SOAP服务器IP:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            static char bufferSoapIP[256];
            static bool bufferSoapIPinitialized = false;
            if (!bufferSoapIPinitialized) {
                strncpy(bufferSoapIP, sServerInfo->soapIp.c_str(), sizeof(bufferSoapIP) - 1);
                bufferSoapIP[sizeof(bufferSoapIP) - 1] = '\0';  // 确保字符串正确终止
                bufferSoapIPinitialized = true;
            }
            if (ImGui::InputText("##SoapIP", bufferSoapIP, sizeof(bufferSoapIP))) {
                sServerInfo->soapIp = bufferSoapIP;
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

            // SOAP服务器端口
            ImGui::Text("SOAP端口号:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            ImGui::InputInt("##SoapPort", &sServerInfo->soapPort, 0, 0);
            ImGui::PopItemWidth();

            // 确保端口号在有效范围内
            if (sServerInfo->soapPort < 1) sServerInfo->soapPort = 1;
            if (sServerInfo->soapPort > 65535) sServerInfo->soapPort = 65535;

            // SOAP用户名
            ImGui::Text("SOAP用户名:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            static char bufferSoapUser[256];
            static bool bufferSoapUserInitialized = false;
            if (!bufferSoapUserInitialized) {
                strncpy(bufferSoapUser, sServerInfo->soapUser.c_str(), sizeof(bufferSoapUser) - 1);
                bufferSoapUser[sizeof(bufferSoapUser) - 1] = '\0';  // 确保字符串正确终止
                bufferSoapUserInitialized = true;
            }
            if (ImGui::InputText("##SoapUser", bufferSoapUser, sizeof(bufferSoapUser))) {
                sServerInfo->soapUser = bufferSoapUser;
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

            // SOAP密码
            ImGui::Text("SOAP密码:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            static char bufferSoapPass[256];
            static bool bufferSoapPassInitialized = false;
            if (!bufferSoapPassInitialized) {
                strncpy(bufferSoapPass, sServerInfo->soapPass.c_str(), sizeof(bufferSoapPass) - 1);
                bufferSoapPass[sizeof(bufferSoapPass) - 1] = '\0';  // 确保字符串正确终止
                bufferSoapPassInitialized = true;
            }
            if (ImGui::InputText("##SoapPass", bufferSoapPass, sizeof(bufferSoapPass))) {
                sServerInfo->soapPass = bufferSoapPass;
                ConfigManager::SaveConfig();
            }
            ImGui::PopItemWidth();

        }
        ImGui::EndGroup();

        // 启动/停止按钮
        float buttonWidth = 150;
        float buttonHeight = 60;
        float padding = 10;
        ImGui::SetCursorPos(ImVec2(
            windowSize.x - buttonWidth - padding,
            windowSize.y - buttonHeight - padding
        ));


        if (!sServerInfo->isLauncherConnected) {
            if (ImGui::Button("启动服务", ImVec2(buttonWidth, buttonHeight))) {
                try 
                {
                    start_server(true);
                }
                catch (const std::exception& e) {
                    std::wstring error_message;
                    size_t size = std::mbstowcs(nullptr, e.what(), 0) + 1;
                    error_message.resize(size);
                    std::mbstowcs(&error_message[0], e.what(), size);
                    MessageBoxW(NULL, error_message.c_str(), L"错误", MB_OK);
                }
            }
        }
        else {
            if (ImGui::Button("停止服务", ImVec2(buttonWidth, buttonHeight)))
            {
               start_server(false);
 
            }
        }

        // 显示服务器状态
        ImGui::SetCursorPos(ImVec2(padding, windowSize.y - buttonHeight - padding));
        ImGui::Text("服务器状态: %s", sServerInfo->isLauncherConnected ? "运行中" : "已停止");

        ImGui::End();
    }

}