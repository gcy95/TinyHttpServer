#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void Start();   //1.运行

private:
    bool InitSocket_();   //socket(),bind(),listen(),
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;    // 最大的文件描述符的个数

    static int SetFdNonblock(int fd);   // 设置文件描述符非阻塞

//自身属性
    int port_;          // 端口
    bool openLinger_;   // 是否打开优雅关闭
    int timeoutMS_;  /* 毫秒MS 超时关闭时间*/
    bool isClose_;  // 服务器是否关闭判断
    bool isLogOpen_; //日志开关
    int listenFd_;  // 监听的文件描述符
    char* srcDir_;  // 资源的目录
    
    
    uint32_t listenEvent_;  // 监听的文件描述符的事件
    uint32_t connEvent_;    // 连接的文件描述符的事件

//内部包含的实际工作的单元
    std::unique_ptr<HeapTimer> timer_;         // 定时器类型的智能指针timer_
    std::unique_ptr<ThreadPool> threadpool_;   // 线程池
    std::unique_ptr<Epoller> epoller_;         // epoll对象
    std::unordered_map<int, HttpConn> users_;  // 用hashmap保存客户端连接的信息表，key用fd表示，value用HttpConn表示
};


#endif //WEBSERVER_H