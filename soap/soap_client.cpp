#include <iostream>
#include <string>
#include "stdsoap2.h"
#include "soapH.h"
#include "soapStub.h"

// 定义SOAP函数
extern "C" {
    int soap_call___ns1__executeCommand(
        struct soap *soap,
        const char *endpoint,
        const char *action,
        struct ns1__executeCommand *request,
        struct ns1__executeCommandResponse *response
    );
}

class SoapClient {
private:
    struct soap *soap;
    std::string endpoint;
    
public:
    SoapClient(const std::string& host = "127.0.0.1", int port = 7878) 
        : endpoint("http://" + host + ":" + std::to_string(port)) {
        
        soap = soap_new();
        if (!soap) {
            throw std::runtime_error("无法创建SOAP上下文");
        }

        soap->connect_timeout = 10;
        soap->send_timeout = 10;
        soap->recv_timeout = 10;
    }

    ~SoapClient() {
        if (soap) {
            soap_destroy(soap);
            soap_end(soap);
            soap_free(soap);
        }
    }

    bool executeCommand(const std::string& command, std::string& response) {
        // 动态分配内存并初始化为0
        struct ns1__executeCommand* request = 
            (struct ns1__executeCommand*)soap_malloc(soap, sizeof(struct ns1__executeCommand));
            
        struct ns1__executeCommandResponse* result = 
            (struct ns1__executeCommandResponse*)soap_malloc(soap, sizeof(struct ns1__executeCommandResponse));

        if (!request || !result) {
            std::cerr << "内存分配失败" << std::endl;
            return false;
        }

        // 初始化结构体成员
        request->command = nullptr;
        result->result = nullptr;

        // 设置命令
        request->command = soap_strdup(soap, command.c_str());

        if (soap_call___ns1__executeCommand(
            soap, 
            endpoint.c_str(), 
            nullptr, 
            request, 
            result) != SOAP_OK) {
            std::cerr << "SOAP请求失败: ";
            soap_print_fault(soap, stderr);
            return false;
        }

        if (result->result && *result->result) {  // 检查指针和指针指向的内容
            response = *result->result;  // 解引用指针获取字符串内容
            return true;
        }

        return false;
    }
};
/*
int startcomman(std::string command) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    try {
        SoapClient client("127.0.0.1", 7878);
        std::string response;

        if (client.executeCommand(command, response)) {
            std::cout << "命令执行成功！\n响应内容：\n" << response << std::endl;
        } else {
            std::cout << "命令执行失败！" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 
*/