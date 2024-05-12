#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "wrap.h" //Socket的包裹函数
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>

int main(int argc, char const *argv[])
{
    // 创建套接字
    int lfd = tcp4bind(8888, NULL);
    // 监听套接字
    Listen(lfd, 128);
    // 最大文件描述符
    int maxFd = lfd;
    /*
    定义文件描述符集合
    - oldSet:用以记录要监听的文件描述符集合
    - rest:实际进行查询操作的文件描述符集合
     */
    fd_set oldSet, rest;
    // 清空创建的文件描述符
    FD_ZERO(&oldSet);
    FD_ZERO(&rest);
    // 将lfd添加到oldSet集合中
    FD_SET(lfd, &oldSet);
    /*
    默认认为select发生变化是有新的连接，故第一步是对其进行提取
    如提取操作后--n结果不为0，则认为原有的文件描述符出现了变化，对其进一步处理
     */
    while (1)
    {
        // 每次循环都更新
        rest = oldSet;
        int n = select(maxFd + 1, &rest, NULL, NULL, NULL);
        if (n < 0) // 出现错误
        {
            perror("select");
            break;
        }
        else if (n == 0)
        { // 没有监听到
            continue;
        }
        else if (n > 0) // 监听到文件描述符
        {
            // 当负责监听的lfd发送变化时，代表有新的连接
            if (FD_ISSET(lfd, &rest))
            {
                // 提取
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);
                char ip[16] = "";
                int cfd = Accept(lfd, (struct sockaddr *)&addr, &len);

                printf("new client ip=%s port=%d\n", inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, 16),
                       ntohs(addr.sin_port));
                // 放置到oldSet集合中
                FD_SET(cfd, &oldSet);
                // 更新maxFd,仅当新提取的cfd大于maxFd时对其进行更新
                if (cfd > maxFd)
                {
                    maxFd = cfd;
                }
                //--n判断是否存在cfd变化
                if (--n == 0) // 如果select监听到的文件描述符数量--等于0证明原有文件描述符没有变化
                {
                    // 直接退出即可，无需继续
                    continue;
                }
            }

            /* 存在变化，则需要遍历rest集合，查看具体的变化情况 */
            // 遍历select后的集合，查看是哪些发生了变化
            for (int i = lfd + 1; i <= maxFd; i++)
            {
                // 如果i指向的文件描述符在集合内，则对其进行读操作
                if (FD_ISSET(i, &rest))
                {
                    char buf[1500] = ""; // 1500是一个报文的长度，以后会经常遇到
                    int ret = Read(i, buf, sizeof(buf));
                    if (ret < 0)
                    {
                        perror("read");
                        close(i);           // 关闭该描述符
                        FD_CLR(i, &oldSet); // 从集合中删除这个描述符
                        continue;
                    }
                    else if (ret == 0)
                    {
                        printf("客户端关闭\n");
                        close(i);
                        FD_CLR(i, &oldSet);
                    }
                    else
                    {
                        printf("读到信息:%s\n", buf);
                        Write(i, buf, ret); // 将收到的信息发送回给客户端
                    }
                }
            }
        }
    }

    return 0;
}
