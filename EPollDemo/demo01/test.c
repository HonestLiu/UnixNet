#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>

int main(int argc, char const *argv[])
{
    int pipfd[2];
    int ret = -1;
    ret = pipe(pipfd);
    if (ret < 0)
    {
        perror("pipe");
        _exit(-1);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        _exit(-1);
    }
    else if (pid == 0)
    {
        // 每5秒向管道写数据
        close(pipfd[0]); // 关闭无用的读管道
        while (1)
        {
            char buf[128] = "Child Write Data";
            write(pipfd[1], buf, sizeof(buf));
            sleep(5); // 休眠5s
        }
    }
    else
    {
        // 父进程:使用epoll监听管道，如果监听到fd[0]有变化就去读
        close(pipfd[1]);            // 关闭无用的写管道

        int epfd = epoll_create(1); // 创建epoll红黑树

        struct epoll_event ee; // 创建节点结构体
        ee.events = EPOLLIN;   // 将要监听的事件设置为读事件
        ee.data.fd = pipfd[0]; // 将读管道放入data的fd中

        epoll_ctl(epfd, EPOLL_CTL_ADD, pipfd[0], &ee); // 节点上树

        struct epoll_event eArr[128]; // 存放变化节点的数组首地址
        while (1)
        {
            // 监听节点变化，有变化就去读
            int num = 0;
            num = epoll_wait(epfd, eArr, 128, -1);
            if (num > 0)
            {
                // 去读数据，并输出到终端
                char buf[128] = "";
                int ret = read(pipfd[0], buf, sizeof(buf));
                if (ret <= 0)
                {
                    // 管道读取错误或对方管道已关闭
                    close(pipfd[0]);                               // 关闭监听的描述符
                    epoll_ctl(epfd, EPOLL_CTL_DEL, pipfd[0], &ee); // 下树
                    write(STDIN_FILENO, "所监听的描述符关闭\n", sizeof("所监听的描述符关闭\n"));
                    break;
                }
                else
                {
                    // 正常输出内容
                    write(STDIN_FILENO, buf, sizeof(buf));
                    write(STDIN_FILENO, "\n", sizeof("\n"));
                }
            }
        }
    }

    return 0;
}
