#include <stdio.h>
#include "wrap.h"
#include <sys/types.h>
#include <sys/epoll.h>

int main(int argc, char const *argv[])
{

    /* 创建并绑定套接字 */
    int lfd = tcp4bind(8080, NULL); // 绑定服务器所有IP地址的8080端口
    /* 监听套接字 */
    Listen(lfd, 128);

    /* 创建树 */
    int epfd = epoll_create(1);

    /* 将监听套接字上树 */
    struct epoll_event ee; // 创建节点结构体
    ee.events = EPOLLIN;   // 将要监听的事件设置为读事件
    ee.data.fd = lfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ee);

    /* 循环监听 */
    struct epoll_event eArr[1024];
    while (1)
    {
        int nready = epoll_wait(epfd, eArr, 1024, -1);
        if (nready < 0)
        {
            // 监听到事件数小于0，表示出错了，关闭lfd并下树退出
            // close(lfd);⭐
            epoll_ctl(epfd, EPOLL_CTL_DEL, lfd, &ee);
            break;
        }
        else if (nready == 0)
        {
            // 没有读到数据，直接跳过本次循环即可
            continue;
        }
        else
        {
            // 遍历文件描述符集合，获取具体变化的节点
            for (int i = 0; i < nready; i++)
            {
                // 判断lfd变化,是lfd且发生读事件
                if (eArr[i].data.fd == lfd && eArr[i].events & EPOLLIN)
                {
                    // 提取监听到描述符
                    struct sockaddr_in addr;
                    char ip[16] = "";
                    socklen_t len = sizeof(addr);
                    int cfd = Accept(lfd, (struct sockaddr *)&addr, &len);

                    printf("new client ip=%s port=%d\n", inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, 16),
                           ntohs(addr.sin_port));

                    /* 上树后原先定义的结构体就无用了，因此可以复用 ⭐*/
                    ee.data.fd = cfd;
                    ee.events = EPOLLIN;

                    // 让提取的描述符上树
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ee);
                }
                // 判断cfd变化，仅监听发生读事件的情况即可
                else if (eArr[i].events & EPOLLIN)
                {
                    // 读数据
                    char buf[1024] = "";
                    int n = read(eArr[i].data.fd, buf, sizeof(buf));
                    if (n < 0)
                    {
                        perror("read");
                        epoll_ctl(epfd, EPOLL_CTL_DEL, eArr[i].data.fd, &eArr[i]); // 该文件描述符下树
                    }
                    else if (n == 0)
                    {
                        close(eArr[i].data.fd);                                    // 关闭文件描述符
                        epoll_ctl(epfd, EPOLL_CTL_DEL, eArr[i].data.fd, &eArr[i]); // 该文件描述符下树
                        write(STDIN_FILENO, "client close\n", sizeof("client close\n"));
                        break;
                    }
                    else
                    {
                        // 正常输出内容
                        printf("%s\n", buf);
                        write(eArr[i].data.fd, "serverReceived\n", sizeof("serverReceived\n"));
                    }
                }
            }
        }
    }

    return 0;
}
