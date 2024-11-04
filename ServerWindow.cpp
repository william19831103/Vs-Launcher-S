#include "Auto.h"
#include "ServerConnector.h"
#include <thread>
#include "Config.h"
#include <string>

// 取消 e 宏定义
#ifdef e
#undef e
#endif

#include "include/stb_image/stb_image.h"

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
            if (ImGui::InputText("##ServerIP", sServerInfo->MangosServerIP.data(), sServerInfo->MangosServerIP.size())) {
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
            if (ImGui::InputText("##ServerName", sServerInfo->LauncherTitle.data(), sServerInfo->LauncherTitle.size())) {    
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