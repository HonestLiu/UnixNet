#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "wrap.h" //Socket的包裹函数
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/*
这是一个多线程的回复服务器，收到客服端发来的数据会原样返回

 */
// 自定义信号处理函数
void Fun(int arg)
{
    // 记录退出的pid
    pid_t pid;
    // 等待任意的子进程，且无子进程退出立即返回不阻塞
    while (1)
    {
        // 当进程中已无要回收的子进程时，就会返回-1,此时break即可
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid == -1)
        {
            break;
        }
        else
        {
            printf("回收子进程:%d\n", pid);
        }
    }
}

int main(int argc, char const *argv[])
{
    /* 设置屏蔽集，在父进程完成对SIGCHLD信号的组成前屏蔽SIGCHLD信号 */
    sigset_t set;                       // 创建信号集
    sigemptyset(&set);                  // 清空信号集，保证干净
    sigaddset(&set, SIGCHLD);           // 将SIGCHLD添加入信号集
    sigprocmask(SIG_BLOCK, &set, NULL); // 将信号集set设置为屏蔽信号集

    // 创建套接字
    int sockFd = Socket(AF_INET, SOCK_STREAM, 0); // 包裹函数的Socket函数，底层自带错误判断
    // 绑定套接字
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    addr.sin_port = htons(8082);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "172.27.214.232", &addr.sin_addr);

    Bind(sockFd, (struct sockaddr *)&addr, len);

    // 监听套接字
    Listen(sockFd, 128);

    // 提取套接字
    // 回复信息
    struct sockaddr_in userAddr;
    socklen_t userLen = sizeof(userAddr);
    char ip[16] = "";
    while (1)
    {
        // 提取连接(⭐主要要判断accept函数的退出是不是因为外部信号而异常退出，是的话需要重新启动它，否则会出问题，我这直接用的是封装没这个问题)
        int fd = Accept(sockFd, (struct sockaddr *)&userAddr, &userLen);
        printf("new client connect ip=%s port=%d\n", inet_ntop(AF_INET, &userAddr.sin_addr.s_addr, ip, sizeof(ip)),
               ntohs(userAddr.sin_port)); // 打印连接客户端信息
        // 开启子进程
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            _exit(-1);
        }
        else if (pid == 0) // 子进程
        {
            // 关闭无用的无用的监听Socket
            close(sockFd);
            char buf[1024] = "";
            // 使用提取到的去获取客户端发送来的信息并写出
            while (1)
            {
                int n = read(fd, buf, sizeof(buf));

                if (n < 0) // 出错了，直接退出
                {
                    perror("read");
                    close(fd);
                    exit(0);
                }
                else if (n == 0) // 对方关闭了
                {
                    printf("客户端关闭\n");
                    close(fd);
                    exit(0);
                }
                else
                {
                    printf("收到客户端:%s\n", buf);
                    write(fd, buf, n);
                    // exit(0);
                }
            }
        }
        else // 父进程
        {
            close(fd); // 关掉提取的管道符
            // 注册子进程退出信号，待接收到退出信号后，调用信号处理函数
            struct sigaction act;
            act.sa_flags = 0;
            act.sa_handler = Fun;      // 信号处理函数
            sigemptyset(&act.sa_mask); // 清空sigaction内的屏蔽集，避免有不必要的东西
            // SIGCHLD是子进程状态发生状态发生改变时发出的信号
            // 通过自定义这个信号的处理函数，使用waitpid进行处理
            sigaction(SIGCHLD, &act, NULL);

            /* 父进程完成对SIGCHLD信号的注册，解除先前设置的屏蔽信号集 */
            sigprocmask(SIG_UNBLOCK, &set, NULL);
        }
    }

    return 0;
}
