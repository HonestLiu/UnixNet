#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include "wrap.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    // 创建监听、绑定套接字
    int lfd = tcp4bind(8888, NULL);
    // 监听
    Listen(lfd, 128);
    // 创建epoll
    int epfd = epoll_create(1);
    // 创建epoll_event结构体
    struct epoll_event ev, events[1024];
    // 添加lfd到epoll
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    // 循环等待
    while (1)
    {
        //nready为就绪事件数
        int nready = epoll_wait(epfd, events, 1024, -1);
        if (nready < 0)
        {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < nready; i++)
        {
            if (events[i].data.fd == lfd && events[i].events & EPOLLIN)
            {
                // 接受连接
                int cfd = Accept(lfd, NULL, NULL);
                // 添加cfd到epoll
                ev.events = EPOLLIN;
                ev.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
            }
            else
            {
                // 读取数据
                char buf[1024];
                int len = Read(events[i].data.fd, buf, sizeof(buf));
                if (len == 0)
                {
                    // 关闭连接
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                }
                else
                {
                    // 输出数据
                    Write(STDOUT_FILENO, buf, len);
                    // 回写数据
                    Write(events[i].data.fd, buf, len);
                }
            }
        }
    }

    return 0;
}
