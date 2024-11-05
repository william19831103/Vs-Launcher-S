#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <ws2tcpip.h>

// 确保正确链接 Mswsock 库
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "Ws2_32.lib")

// 只在未定义时定义这些宏
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <asio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include "opcodes.h"
#include <iostream>
#include "soap/soap_client.cpp"

namespace fs = std::filesystem;
using asio::ip::tcp;

// 保留 ServerInfo 单例
class ServerInfo
{
public:
    static ServerInfo *instance()
    {
        static ServerInfo instance;
        return &instance;
    }

    //登录器端口
    int serverport = 34128;
    //登录器标题
    std::string LauncherTitle = "XX魔兽";
    //登录器通知
    std::string LauncherNotice = "欢迎来到游戏服务器！当前版本: 1.0.0";
    //登录器服务器是否连接
    bool isLauncherConnected = false;
    //mangos服务器是否连接
    bool isMangosServerConnected = false;
    //mangos服务器IP
    std::string MangosServerIP = "127.0.0.1";
    //mangos服务器端口
    int MangosServerPort = 3724;
    //soap服务器IP
    std::string soapIp="127.0.0.1";
    //soap服务器端口
    int soapPort=7878;
    //soap用户名
    std::string soapUser="1";
    //soap密码
    std::string soapPass="1";

    // 加载通知
    void LoadNotice()
    {
        std::ifstream file("G.txt", std::ios::binary);
        if (!file.is_open())
        {
            MessageBoxW(NULL, L"无法打开通知文件 G.txt\n请确认文件存在且可访问", L"错误", MB_OK);
            return;
        }

        // 直接读取文件内容
        LauncherNotice.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
};
#define sServerInfo ServerInfo::instance()

// 修改 to_lower 函数声明
inline std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return s;
}

// Session类的完整定义
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket, size_t id)
        : socket_(std::move(socket)), session_id_(id), is_sending_file_(false)
    {
    }

    void start()
    {
        do_read_header();
    }

    // 修改检查连接状态的方法
    bool is_connected() const
    {
        return socket_.is_open();
    }

private:
    static constexpr size_t CHUNK_SIZE = 8192; // 8KB chunks

    void do_read_header()
    {
        if (is_sending_file_)
        {
            std::cout << "正在发送文件，暂不读取新消息" << std::endl;
            return;
        }

        auto header = std::make_shared<MessageHeader>();
        asio::async_read(socket_, asio::buffer(header.get(), sizeof(MessageHeader)),
                         [this, header](std::error_code ec, std::size_t /*length*/)
                         {
                             if (!ec)
                             {
                                 std::cout << "收到消息头，操作码: 0x" << std::hex << header->opcode
                                           << ", 大小: " << std::dec << header->size << std::endl;

                                 // 验证操作码的有效性
                                 if (header->opcode >= MSG_NULL_ACTION && header->opcode <= SMSG_PATCH_FILE_END)
                                 {
                                     handle_message_header(*header);
                                 }
                                 else
                                 {
                                     std::cout << "收到无效的操作码: 0x" << std::hex << header->opcode << std::dec << std::endl;
                                     // 重新开始读取新的消息头
                                     do_read_header();
                                 }
                             }
                             else
                             {
                                 if (ec == asio::error::eof ||
                                     ec == asio::error::connection_reset ||
                                     ec == asio::error::operation_aborted)
                                 {
                                     std::cout << "客户端[" << session_id_ << "]断开连接" << std::endl;
                                     handle_disconnect();
                                 }
                                 else
                                 {
                                     std::cout << "读取消息头失败: " << ec.message()
                                               << " (错误码: " << ec.value() << ")" << std::endl;
                                 }
                             }
                         });
    }

    void handle_disconnect()
    {
        try
        {
            if (socket_.is_open())
            {
                std::error_code ec;
                socket_.shutdown(tcp::socket::shutdown_both, ec);
                if (ec)
                {
                    std::cout << "关闭socket连接时发生错误: " << ec.message() << std::endl;
                }
                socket_.close(ec);
                if (ec)
                {
                    std::cout << "关闭socket时发生错误: " << ec.message() << std::endl;
                }
                std::cout << "客户端[" << session_id_ << "]连接已关闭" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "处理断开连接时发生错误: " << e.what() << std::endl;
        }
    }

    void handle_message_header(const MessageHeader &header)
    {
        std::cout << "处理消息，操作码: 0x" << std::hex << header.opcode << std::dec << std::endl;

        switch (header.opcode)
        {
        case CMSG_GET_SERVER_NOTICE:
            handle_server_notice();
            break;

        case CMSG_REGISTER_ACCOUNT:
            if (header.size > 0)
            {
                read_message_body(header.size,
                                  [this](const std::vector<char> &data)
                                  {
                                      handle_register_account(data);
                                  });
            }
            break;

        case CMSG_CHECK_PATCH:
            handle_check_patch();
            break;

        case CMSG_REQUEST_PATCH_FILE:
            if (header.size > 0)
            {
                read_message_body(header.size,
                                  [this](const std::vector<char> &data)
                                  {
                                      handle_request_patch_file(data);
                                  });
            }
            break;

        default:
            std::cout << "未知的操作码: 0x" << std::hex << header.opcode << std::dec << std::endl;
            do_read_header();
            break;
        }
    }

    void read_message_body(uint32_t size, std::function<void(const std::vector<char> &)> handler)
    {
        auto buffer = std::make_shared<std::vector<char>>(size);
        asio::async_read(socket_, asio::buffer(*buffer),
                         [this, buffer, handler](std::error_code ec, std::size_t length)
                         {
                             if (!ec)
                             {
                                 std::cout << "读取消息体成功，大小: " << length << " 字节" << std::endl;
                                 handler(*buffer);
                                 // 处理完消息后，继续读取下一个消息头
                                 do_read_header();
                             }
                             else
                             {
                                 std::cout << "读取消息体失败: " << ec.message() << std::endl;
                             }
                         });
    }

    void send_response(uint16_t opcode, const std::string &data)
    {
        MessageHeader header{opcode, static_cast<uint32_t>(data.size())};
        auto buffer = std::make_shared<std::vector<char>>(sizeof(header) + data.size());

        memcpy(buffer->data(), &header, sizeof(header));
        memcpy(buffer->data() + sizeof(header), data.data(), data.size());

        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(*buffer),
                          [this, self, buffer, opcode](std::error_code ec, std::size_t length)
                          {
                              if (ec)
                              {
                                  std::cout << "发送响应失败: " << ec.message() << std::endl;
                              }
                              else
                              {
                                  std::cout << "发送响应成功: opcode=0x" << std::hex << opcode
                                            << std::dec << ", length=" << length << std::endl;
                              }
                          });
    }

    void handle_server_notice()
    {        
        std::string serverinfo;
        serverinfo+=sServerInfo->LauncherTitle+"||";
        serverinfo+=sServerInfo->MangosServerIP+"||";
        serverinfo+=std::to_string(sServerInfo->MangosServerPort)+"||";
        serverinfo+=sServerInfo->LauncherNotice;       

        send_response(SMSG_SERVER_NOTICE, serverinfo);
        do_read_header();
    }

    void handle_register_account(const std::vector<char> &data)
    {
        std::string command(data.begin(), data.end());

        // 解析账号、密码和安全码
        size_t pos1 = command.find("||");
        size_t pos2 = command.find("||", pos1 + 2);
        
        if (pos1 == std::string::npos || pos2 == std::string::npos) {
            std::cout << "无效的注册数据格式" << std::endl;
            send_response(SMSG_REGISTER_RESULT, "注册数据格式错误");
            return;
        }

        std::string account = command.substr(0, pos1);
        std::string password = command.substr(pos1 + 2, pos2 - pos1 - 2); 
        std::string securityKey = command.substr(pos2 + 2);

        // 验证账号密码长度
        if (account.length() < 3 || password.length() >12) {
            send_response(SMSG_REGISTER_RESULT, "账号长度至少4位,密码长度至少6位");
            return;
        }

        // 验证账号密码是否包含非法字符
        if (account.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos) {
            send_response(SMSG_REGISTER_RESULT, "账号只能包含字母和数字");
            return;
        }

        std::string response;
        std::string regcommand = "account create " + account + " " + password+ " " + password;

        try {
            SoapClient client(sServerInfo->soapIp, sServerInfo->soapPort);
            if (client.executeCommand(regcommand, response)) 
            {
                std::cout << "注册命令执行成功！\n响应内容：\n" << response << std::endl;
                
                if (response.find("Account created") != std::string::npos) {
                    send_response(SMSG_REGISTER_RESULT, "账号注册成功!");
                } else {
                    send_response(SMSG_REGISTER_RESULT, response);
                }
            } else {
                std::cout << "注册命令执行失败！" << std::endl;
                send_response(SMSG_REGISTER_RESULT, "注册失败,请稍后重试");
            }
        } catch (const std::exception& e) {
            std::cout << "注册异常: " << e.what() << std::endl;
            send_response(SMSG_REGISTER_RESULT, "注册出现异常,请稍后重试");
        }
    }

    void handle_check_patch()
    {
        fs::path data_dir = fs::current_path() / "Data";
        std::vector<PatchFileInfo> patch_files;

        // 收集服务器端补丁文件信息
        for (const auto &entry : fs::directory_iterator(data_dir))
        {
            // 转换扩展名为小写进行比较
            std::string ext = to_lower(entry.path().extension().string());
            if (ext == ".mpq")
            {
                PatchFileInfo info{};
                std::string filename = entry.path().filename().string();
                strncpy(info.filename, filename.c_str(), sizeof(info.filename) - 1);
                info.filesize = fs::file_size(entry.path());
                info.timestamp = fs::last_write_time(entry.path()).time_since_epoch().count();
                patch_files.push_back(info);
            }
        }

        // 发送补丁文件信息列表
        std::string data;
        data.resize(sizeof(PatchFileInfo) * patch_files.size());
        memcpy(data.data(), patch_files.data(), data.size());
        send_response(SMSG_PATCH_INFO, data);
        do_read_header();
    }

    void handle_request_patch_file(const std::vector<char> &data)
    {
        if (is_sending_file_)
        {
            std::cout << "正在发送其他文件，忽略新的文件请求" << std::endl;
            return;
        }

        std::string filename(data.begin(), data.end());
        fs::path filepath = fs::current_path() / "Data" / filename;

        // 如果文件不存在，尝试查找不同大小写的版本
        if (!fs::exists(filepath))
        {
            for (const auto &entry : fs::directory_iterator(fs::current_path() / "Data"))
            {
                if (to_lower(entry.path().filename().string()) == to_lower(filename))
                {
                    filepath = entry.path();
                    break;
                }
            }
        }

        std::cout << "\n收到文请求: " << filename << std::endl;
        std::cout << "完整路径: " << filepath << std::endl;

        if (!fs::exists(filepath))
        {
            std::cout << "请求的文件不存在: " << filename << std::endl;
            return;
        }

        try
        {
            is_sending_file_ = true;
            auto fileSize = fs::file_size(filepath);
            std::cout << "开始发送文件: " << filename << ", 大小: " << fileSize << " 字节" << std::endl;

            MessageHeader header{SMSG_PATCH_FILE, static_cast<uint32_t>(fileSize)};
            auto headerBuffer = std::make_shared<std::vector<char>>(sizeof(header));
            memcpy(headerBuffer->data(), &header, sizeof(header));

            auto self(shared_from_this());
            asio::async_write(socket_, asio::buffer(*headerBuffer),
                              [this, self, filepath, filename, fileSize](std::error_code ec, std::size_t /*length*/)
                              {
                                  if (ec)
                                  {
                                      std::cout << "发送文件头失败: " << ec.message() << std::endl;
                                      is_sending_file_ = false;
                                      do_read_header();
                                      return;
                                  }
                                  std::cout << "文件头发送成功，开始发送文件内容" << std::endl;

                                  auto file = std::make_shared<std::ifstream>(filepath, std::ios::binary);
                                  if (!file->is_open())
                                  {
                                      std::cout << "无法打开文件: " << filename << std::endl;
                                      is_sending_file_ = false;
                                      do_read_header();
                                      return;
                                  }
                                  send_file_chunk(file, filename, fileSize, 0);
                              });
        }
        catch (const std::exception &e)
        {
            std::cout << "处理文件时发生异常: " << e.what() << std::endl;
            is_sending_file_ = false;
            do_read_header();
        }
    }

    void send_file_chunk(std::shared_ptr<std::ifstream> file,
                         const std::string &filename,
                         size_t totalSize,
                         size_t sentBytes)
    {
        if (!file->is_open())
        {
            std::cout << "文件已关闭" << std::endl;
            is_sending_file_ = false;
            do_read_header();
            return;
        }

        auto buffer = std::make_shared<std::vector<char>>(CHUNK_SIZE);
        file->read(buffer->data(), CHUNK_SIZE);
        size_t bytesRead = file->gcount();

        if (bytesRead == 0)
        {
            std::cout << "\n文件 " << filename << " 内容发送完成，发送完成标记" << std::endl;
            file->close();

            std::string endMarker = "END_OF_FILE";
            MessageHeader endHeader{SMSG_PATCH_FILE_END, static_cast<uint32_t>(endMarker.size())};
            auto endBuffer = std::make_shared<std::vector<char>>(sizeof(endHeader) + endMarker.size());
            memcpy(endBuffer->data(), &endHeader, sizeof(endHeader));
            memcpy(endBuffer->data() + sizeof(endHeader), endMarker.data(), endMarker.size());

            auto self(shared_from_this());
            asio::async_write(socket_, asio::buffer(*endBuffer),
                              [this, self, filename](std::error_code ec, std::size_t /*length*/)
                              {
                                  if (ec)
                                  {
                                      std::cout << "发送结束标记失败: " << ec.message() << std::endl;
                                  }
                                  else
                                  {
                                      std::cout << "文件 " << filename << " 的结束标记发送成功" << std::endl;
                                  }
                                  is_sending_file_ = false;
                                  do_read_header();
                              });
            return;
        }

        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(buffer->data(), bytesRead),
                          [this, self, file, filename, totalSize, sentBytes, bytesRead](std::error_code ec, std::size_t /*length*/)
                          {
                              if (ec)
                              {
                                  std::cout << "发送文件块失败: " << ec.message() << std::endl;
                                  file->close();
                                  is_sending_file_ = false;
                                  do_read_header();
                                  return;
                              }

                              size_t newSentBytes = sentBytes + bytesRead;
                              float progress = (float)newSentBytes / totalSize * 100;
                              std::cout << "\r发送进度: " << std::fixed << std::setprecision(2)
                                        << progress << "% (" << newSentBytes << "/" << totalSize
                                        << " bytes)" << std::flush;

                              if (newSentBytes < totalSize)
                              {
                                  send_file_chunk(file, filename, totalSize, newSentBytes);
                              }
                              else
                              {
                                  std::cout << "\n文件 " << filename << " 内容发送完成，发送完成标记" << std::endl;
                                  file->close();

                                  std::string endMarker = "END_OF_FILE";
                                  MessageHeader endHeader{SMSG_PATCH_FILE_END, static_cast<uint32_t>(endMarker.size())};
                                  auto endBuffer = std::make_shared<std::vector<char>>(sizeof(endHeader) + endMarker.size());
                                  memcpy(endBuffer->data(), &endHeader, sizeof(endHeader));
                                  memcpy(endBuffer->data() + sizeof(endHeader), endMarker.data(), endMarker.size());

                                  asio::async_write(socket_, asio::buffer(*endBuffer),
                                                    [this, self, filename](std::error_code ec, std::size_t /*length*/)
                                                    {
                                                        if (ec)
                                                        {
                                                            std::cout << "发送结束标记失败: " << ec.message() << std::endl;
                                                        }
                                                        else
                                                        {
                                                            std::cout << "文件 " << filename << " 的结束标记发送成功" << std::endl;
                                                        }
                                                        is_sending_file_ = false;
                                                        do_read_header();
                                                    });
                              }
                          });
    }

    tcp::socket socket_;
    size_t session_id_;
    bool is_sending_file_;
};

// TcpServer类的定义
class TcpServer
{
public:
    // 删除默认构造函数
    TcpServer() = delete;

    // 获取单例对象的静态方法
    static TcpServer *instance(asio::io_context &io_context, short port)
    {
        static TcpServer instance(io_context, port);
        return &instance;
    }

    // 确保单例不能被复制或移动
    TcpServer(const TcpServer &) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    // 构造函数
    TcpServer(asio::io_context &io_context, short port)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          next_session_id_(1)
    {
        start_accept();
    }

private:
    void start_accept()
    {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    size_t session_id = next_session_id_++;
                    std::cout << "新客户端连接！分配会话ID: " << session_id << std::endl;

                    auto session = std::make_shared<Session>(std::move(socket), session_id);
                    sessions_[session_id] = session;
                    session->start();

                    // 清理已断开的会话
                    clean_disconnected_sessions();
                }
                start_accept();
            });
    }

    void clean_disconnected_sessions()
    {
        for (auto it = sessions_.begin(); it != sessions_.end();)
        {
            if (!it->second->is_connected())
            {
                std::cout << "清理已断开的会话: " << it->first << std::endl;
                it = sessions_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    asio::io_context &io_context_;
    tcp::acceptor acceptor_;
    size_t next_session_id_;
    std::unordered_map<size_t, std::shared_ptr<Session>> sessions_;
};

#define sServer TcpServer::instance()
