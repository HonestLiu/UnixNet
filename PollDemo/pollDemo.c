#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <ctype.h>

#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 8000
#define OPEN_MAX 1024

int main(int argc, char const *argv[])
{
    int i, j, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;

    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t clilen;

    struct pollfd client[OPEN_MAX];       // 定义pollfd结构体数组
    struct sockaddr_in cliaddr, servaddr; // 定义服务器和客户端地址结构体

    listenfd = Socket(AF_INET, SOCK_STREAM, 0); // 创建一个socket，得到监听描述符

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 设置端口复用

    bzero(&servaddr, sizeof(servaddr));           // 初始化服务器地址结构体
    servaddr.sin_family = AF_INET;                // 设置地址族
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 设置IP地址
    servaddr.sin_port = htons(SERV_PORT);         // 设置端口

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)); // 绑定地址结构体和监听描述符
    Listen(listenfd, 128);                                          // 设置监听描述符

    client[0].fd = listenfd;   // 设置要监听第一个文件描述符，client[0]为监听描述符
    client[0].events = POLLIN; // 设置监听事件为POLLIN, 表示有数据可读

    for (i = 1; i < OPEN_MAX; i++) // 初始化client[]数组
    {
        /*在poll函数中，其结构体数组中的fd成员为负数时表示这个pollfd结构体元素未被使用*/
        client[i].fd = -1; // 用-1初始化client[]里剩下的元素
    }

    maxi = 0; // client[]数组有效元素中最大元素下标

    /*  整体是先监听listenfd，然后监听client[]中的文件描述符 */
    for (;;)
    {
        nready = poll(client, maxi + 1, -1); // 阻塞监听事件发生，返回发生事件的个数

        // 使用&符号而不使用==号是为了避免revents内包好多个事件，这样可以避免遗漏事件
        if (client[0].revents & POLLIN) // client[0]对应的监听描述符有读事件发生
        {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen); // 接受连接请求
            printf("received from %s at PORT %d\n",
                   inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                   ntohs(cliaddr.sin_port));

            for (i = 1; i < OPEN_MAX; i++)
            {
                if (client[i].fd < 0) // 找client[]中没有使用的文件描述符
                {
                    client[i].fd = connfd; // 保存accept返回的文件描述符到client[]未使用的文件描述符中
                    break;
                }
            }

            if (i == OPEN_MAX) // 达到最大客户端数,输出错误信息并退出
            {
                perr_exit("too many clients");
            }

            client[i].events = POLLIN; // 设置刚刚返回的文件描述符监控事件为读事件POLLIN

            // 如果当前处理的文件描述符下标大于maxi，更新maxi
            if (i > maxi)
            {
                maxi = i; // 更新client[]中最大元素下标
            }

            //--nready的结果如果等于0，说明除了监听描述符没有其他就绪事件，继续回到poll阻塞监听
            if (--nready == 0)
            {
                continue; // 如果没有更多就绪事件，继续回到poll阻塞监听
            }
        }

        /*到这则说明除了监听描述符有就绪事件外，client[]中的文件描述符也有就绪事件*/

        for (i = 1; i <= maxi; i++) // 检测client[]中的就绪事件
        {
            if ((sockfd = client[i].fd) < 0)
            {
                // client[i].fd为-1表示这个client[i]没有被使用，直接continue即可
                continue;
            }

            if (client[i].revents & POLLIN) // sockfd准备好
            {
                if ((n = Read(sockfd, buf, MAXLINE)) < 0)
                {
                    if (errno == ECONNRESET) // 客户端已经关闭连接,ECONNRESET表示收到RST复位
                    {
                        // 客户端关闭连接，服务器也关闭对应连接
                        printf("client[%d] aborted connection\n", i);
                        Close(sockfd);
                        client[i].fd = -1;
                    }
                    else
                    {
                        perr_exit("read error");
                    }
                }
                else if (n == 0) // 客户端关闭连接
                {
                    printf("client[%d] closed connection\n", i);
                    Close(sockfd);
                    client[i].fd = -1;
                }
                else
                {
                    for (j = 0; j < n; j++)
                    {
                        buf[j] = toupper(buf[j]); // 转大写
                    }
                    Write(sockfd, buf, n); // 写给客户端
                }

                if (--nready <= 0) // 如果没有更多就绪事件，继续回到poll阻塞监听
                {
                    break;
                }
            }
        }
    }
    return 0;
}
