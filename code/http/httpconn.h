#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

// Http连接类，其中封装了请求和响应对象
class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);  //

    ssize_t read(int* saveErrno);  //读数据

    ssize_t write(int* saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;
    
    sockaddr_in GetAddr() const;
    
    bool process();

    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    //静态成员变量
    static bool isET;   //是ET判断
    static const char* srcDir;  // 资源的目录
    static std::atomic<int> userCount; // 总共连入的的客户端数量，因为是记录这个类的通用性质，因此要用静态变量
    
private:
    //每个连入用户的fd和addr_，需要记录在hashmap中
    int fd_;  //连入用户的fd
    struct  sockaddr_in addr_;  //连入用户的IP

    bool isClose_; //记录这个连接是否关闭了的选项
    
    int iovCnt_;    // 分散内存的数量
    struct iovec iov_[2];   // 分散内存
    
    Buffer readBuff_;   // 读(请求)缓冲区，保存请求数据的内容
    Buffer writeBuff_;  // 写(响应)缓冲区，保存响应数据的内容

    HttpRequest request_;   // 请求对象
    HttpResponse response_; // 响应对象
};


#endif //HTTP_CONN_H