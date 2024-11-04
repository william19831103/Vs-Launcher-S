#include "ServerConnector.h"
#include <iostream>
#include <asio.hpp>
#include <string>
#include <thread>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <iomanip>
#include "opcodes.h"
#include <set>
#include <map>

using asio::ip::tcp;
namespace fs = std::filesystem;

// 在文件开头添加静态io_context
static asio::io_context io_context;

int start_server(bool start) 
{
    //SetConsoleOutputCP(CP_UTF8);
    //SetConsoleCP(CP_UTF8);

    // 加载通知
    sServerInfo->LoadNotice();

    static std::thread io_thread;

    if (start && !sServerInfo->isLauncherConnected)    
    {
        try {
            // 创建服务器实例
            auto server = TcpServer::instance(io_context, sServerInfo->serverport);
            std::cout << "服务器启动，监听端口 " << sServerInfo->serverport << "..." << std::endl;

            // 使用类内的io_context
            io_thread = std::thread([&]() {
                io_context.run();
            });

            sServerInfo->isLauncherConnected = true;
            std::cout << "服务器正在运行..." << std::endl;
        }
        catch (std::exception& e) {
            std::cerr << "启动服务器时异常: " << e.what() << std::endl;
            return -1;
        }
    } else if (!start && sServerInfo->isLauncherConnected) {
        std::cout << "正在关闭服务器..." << std::endl;
        io_context.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
        std::cout << "服务器已关闭" << std::endl;
        sServerInfo->isLauncherConnected = false;
    }

    return 0;
}