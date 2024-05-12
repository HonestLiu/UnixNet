#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
/*
UDP协议面向无连接，所以只管接收数据处理后响应即可，无需监听。
 */
int main(int argc, char const *argv[])
{
    // 1.创建套接字,需要注意的是，UDP协议的套接字类型需要使用SOCK_DGRAM数据报类型
    int socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    // 2.绑定套接字
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int lfd = bind(socketFd, (struct sockaddr *)&addr, sizeof(addr));
    if (lfd < 0) // 绑定失败，直接退出
    {
        perror("bind");
        return 0;
    }

    // 3.接收数据、响应数据
    char reBuf[1024] = "";     // 用以存储接收到的数据
    struct sockaddr_in reAddr; // 用于记录请求的主机地址
    socklen_t len = sizeof(reAddr);
    while (1)
    {
        memset(reBuf, 0, sizeof(reBuf));
        int n = recvfrom(socketFd, reBuf, sizeof(reBuf), 0, (struct sockaddr *)&reAddr, &len); // 接收数据
        if (n < 0)                                                                             // 错误，直接退出
        {
            perror("recvfrom");
            break;
        }
        else
        {
            printf("接收到数据:%s\n", reBuf);
            sendto(socketFd, reBuf, sizeof(reBuf), 0, (struct sockaddr *)&reAddr, sizeof(reAddr));
        }
    }

    // 4.关闭套接字
    close(socketFd);

    return 0;
}
