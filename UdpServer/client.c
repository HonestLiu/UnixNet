#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
UDP客户端和服务端代码整体区别不大，不过是读写的顺序存在差异罢了
 */
int main(int argc, char const *argv[]) {
    // 1.创建套接字,需要注意的是，UDP协议的套接字类型需要使用SOCK_DGRAM数据报类型
    int socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    // 2.绑定套接字
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5148); // 客户端的端口切记不能和服务端重复
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int lfd = bind(socketFd, (struct sockaddr *) &addr, sizeof(addr));
    if (lfd < 0) // 绑定失败，直接退出
    {
        perror("bind");
        return 0;
    }

    // 3.发送数据、接收数据
    char sendBuf[1024] = ""; // 用以存储待发送的数据
    char reBuf[1024] = "";   // 用以存储接收到的数据

    /* 定义服务器的socketAddr */
    struct sockaddr_in sendAddr;
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons(8080);                   // 设置为服务器的端口
    sendAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 设置为服务器的地址
    socklen_t len = sizeof(sendAddr);

    int n = 0;
    while (1) {
        memset(sendBuf, 0, sizeof(sendBuf));
        n = read(STDIN_FILENO, sendBuf, sizeof(sendBuf)); // 从标准输入读取要发送的数据
        if (n < 0) {
            perror("read");
            break;
        }
        sendto(socketFd, sendBuf, strlen(sendBuf), 0, (struct sockaddr *) &sendAddr, sizeof(sendAddr));

        /* Tip:这里因为知道发送数据的就是服务器了，所以直接写NULL，表不存储数据发送方信息 */
        n = recvfrom(socketFd, reBuf, sizeof(reBuf), 0, NULL, NULL); // 接收数据
        if (n < 0)                                                   // 错误，直接退出
        {
            perror("recvfrom");
            break;
        }
        printf("ServerReturn:%s\n", reBuf); // 将服务器发送回来的数据打印到屏幕上
    }

    // 4.关闭套接字
    close(socketFd);

    return 0;
}
