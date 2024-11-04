#include "Auto.h"
#include "ServerConnector.h"
#include <thread>

// 取消 e 宏定义
#ifdef e
#undef e
#endif

#include "include/stb_image/stb_image.h"

/*
// 定义 ServerInfo 的静态成员
std::string ServerInfo::ip;
std::string ServerInfo::port;
std::string ServerInfo::name;
std::string ServerInfo::notice;
bool ServerInfo::isConnected = false; 
*/

extern int startServer();

// 实现LoadTextureFromFile函数
inline bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // 加载图片
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // 创建纹理
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // 创建纹理视图
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

void RenderBackground()
{
    if (g_background)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetWindowWidth();
        float height = ImGui::GetWindowHeight();
        ImGui::Image((ImTextureID)g_background, ImVec2(width, height));
    }
}

void MainWindow() {
    static bool initialized = false;
    if (!initialized) {
        InitializeServerName();
        initialized = true;
    }

    static bool open = true;

    if (open) {
        // 设置窗口填满整个客户端区域
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        
        // 修改窗口标志
        ImGui::Begin("魔兽世界服务端", &open,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );

        ImVec2 windowSize = ImGui::GetWindowSize();
        
        // 服务器配置区域
        ImGui::BeginGroup();
        {
            // IP地址输入
            ImGui::Text("服务器IP:");
            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            ImGui::InputText("##ServerIP", serverIP, sizeof(serverIP));
            ImGui::PopItemWidth();

            // 端口号输入
            ImGui::Text("端口号:");
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            ImGui::InputInt("##ServerPort", &serverPort, 0, 0);
            ImGui::PopItemWidth();

            // 确保端口号在有效范围内
            if (serverPort < 1) serverPort = 1;
            if (serverPort > 65535) serverPort = 65535;

            // 服务器名称输入
            ImGui::Text("服务器名称:");
            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            ImGui::InputText("##ServerName", serverName, sizeof(serverName));
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

        if (!g_serverRunning) {
            if (ImGui::Button("启动服务", ImVec2(buttonWidth, buttonHeight))) {
                try {
                    g_io_context = std::make_unique<asio::io_context>();
                    g_server = std::make_shared<TcpServer>(*g_io_context, serverPort);
                    
                    // 设置服务器配置
                    g_server->SetServerConfig(serverIP, serverPort, std::string(serverName));
                    
                    g_server->Start();
                    g_serverRunning = true;

                    g_io_thread = std::make_unique<std::thread>([&]() {
                        g_io_context->run();
                    });

                    // 转换服务器名称为宽字符用于显示
                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, serverName, -1, nullptr, 0);
                    std::vector<wchar_t> wideName(wideLen);
                    MultiByteToWideChar(CP_UTF8, 0, serverName, -1, wideName.data(), wideLen);

                    // 显示消息
                    //std::wstring msg = L"服务器已启动\nIP: " + std::wstring(serverIP, serverIP + strlen(serverIP)) + 
                    //                 L"\n端口: " + std::to_wstring(serverPort) + 
                    //                 L"\n名称: " + std::wstring(wideName.data());
                    //MessageBoxW(NULL, msg.c_str(), L"服器状态", MB_OK);
                }
                catch (const std::exception& e) {
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, e.what(), -1, NULL, 0);
                    std::wstring wstr(wlen, 0);
                    MultiByteToWideChar(CP_UTF8, 0, e.what(), -1, &wstr[0], wlen);
                    MessageBoxW(NULL, wstr.c_str(), L"错误", MB_OK);
                }
            }
        }
        else {
            if (ImGui::Button("停止服务", ImVec2(buttonWidth, buttonHeight))) {
                if (g_server) {
                    g_server->Stop();
                    g_io_context->stop();
                    if (g_io_thread && g_io_thread->joinable()) {
                        g_io_thread->join();
                    }
                    g_server.reset();
                    g_io_context.reset();
                    g_io_thread.reset();
                    g_serverRunning = false;
                    MessageBoxW(NULL, L"服务器已停止", L"服务器状态", MB_OK);
                }
            }
        }

        // 显示服务器状态
        ImGui::SetCursorPos(ImVec2(padding, windowSize.y - buttonHeight - padding));
        ImGui::Text("服务器状态: %s", g_serverRunning ? "运行中" : "已停止");

        ImGui::End();
    }
    else {
        if (g_server) {
            g_server->Stop();
            g_io_context->stop();
            if (g_io_thread && g_io_thread->joinable()) {
                g_io_thread->join();
            }
        }
        exit(0);
    }
}