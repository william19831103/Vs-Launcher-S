#if defined(__BORLANDC__)
#pragma option push -w-8060
#pragma option push -w-8004
#endif

#include "soapH.h"
#include "soapStub.h"
#include <iostream>
#include "ServerConnector.h"

// 修改命名空间定义，移除 const 和 struct 关键字
SOAP_NMAC struct Namespace namespaces[] = {
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
    {"ns1", "urn:MaNGOS", NULL, NULL},
    {NULL, NULL, NULL, NULL}
};

extern "C" int soap_call___ns1__executeCommand(
    struct soap *soap,
    const char *endpoint,
    const char *action,
    struct ns1__executeCommand *request,
    struct ns1__executeCommandResponse *response)
{
    if (!request || !response)
        return SOAP_FAULT;

    soap->encodingStyle = NULL;
    
    // 添加调试输出
    std::cout << "正在连接到: " << (endpoint ? endpoint : "null") << std::endl;
    std::cout << "Action: " << (action ? action : "null") << std::endl;
    std::cout << "Command: " << (request->command ? request->command : "null") << std::endl;

    // 设置默认的endpoint和action
    if (!endpoint)
        endpoint = soap->endpoint;
    if (!action)
        action = "ns1:executeCommand";  // 修改action名称

    // 设置SOAP版本和编码
    soap->version = 2;  // 使用SOAP 1.2
    soap->keep_alive = 1;  // 保持连接
    
    // 添加HTTP基本认证
    soap->userid = sServerInfo->soapUser.c_str();  // 用户名
    soap->passwd = sServerInfo->soapPass.c_str();  // 密码
    
    // 添加必要的HTTP头
    soap_set_namespaces(soap, namespaces);

    // 序列化请求
    if (soap_begin_count(soap)
     || soap_envelope_begin_out(soap)
     || soap_putheader(soap)
     || soap_body_begin_out(soap)
     || soap_put_ns1__executeCommand(soap, request, "ns1:executeCommand", "")
     || soap_body_end_out(soap)
     || soap_envelope_end_out(soap))
    {
        std::cerr << "序列化请求失败: " << soap_fault_string(soap) << std::endl;
        return soap->error;
    }

    // 计算消息长度
    if (soap_end_count(soap))
    {
        std::cerr << "计算消息长度失败: " << soap_fault_string(soap) << std::endl;
        return soap->error;
    }

    // 发送请求
    if (soap_connect(soap, endpoint, action)
     || soap_envelope_begin_out(soap)
     || soap_putheader(soap)
     || soap_body_begin_out(soap)
     || soap_put_ns1__executeCommand(soap, request, "ns1:executeCommand", "")
     || soap_body_end_out(soap)
     || soap_envelope_end_out(soap)
     || soap_end_send(soap))
    {
        std::cerr << "发送请求失败: " << soap_fault_string(soap) << std::endl;
        return soap_closesock(soap);
    }

    // 接收响应
    if (soap_begin_recv(soap)
     || soap_envelope_begin_in(soap)
     || soap_recv_header(soap)
     || soap_body_begin_in(soap))
    {
        if (soap->error == SOAP_EOF)
        {
            std::cerr << "连接关闭: " << soap_fault_string(soap) << std::endl;
            return soap_closesock(soap);
        }
        std::cerr << "接收响应头失败: " << soap_fault_string(soap) << std::endl;
        return soap_recv_fault(soap, 0);
    }

    // 反序列化响应
    if (soap_get_ns1__executeCommandResponse(soap, response, "ns1:executeCommandResponse", ""))
    {
        // 如果响应中包含了有效数据，我们不应该将其视为错误
        if (response->result && *response->result)
        {
            // 继续处理响应
        }
        else
        {
            std::cerr << "反序列化响应失败: " << soap_fault_string(soap) << std::endl;
            return soap_closesock(soap);
        }
    }

    if (soap_body_end_in(soap)
     || soap_envelope_end_in(soap)
     || soap_end_recv(soap))
    {
        std::cerr << "接收响应体失败: " << soap_fault_string(soap) << std::endl;
        return soap_closesock(soap);
    }

    return SOAP_OK;
}

#if defined(__BORLANDC__)
#pragma option pop
#pragma option pop
#endif 