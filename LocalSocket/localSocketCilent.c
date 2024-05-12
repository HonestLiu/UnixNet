#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

int main(int argc, char const *argv[])
{
    // 1.创建套接字
    int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);

    // 2.绑定套接字(需要为客户端创建一个独立的本地套接字结构体并绑定，否则就是隐式绑定了)
    struct sockaddr_un cAddr;
    cAddr.sun_family = AF_UNIX;
    strcpy(cAddr.sun_path, "sock.ca");
    socklen_t len = offsetof(struct sockaddr_un, sun_path) + strlen(cAddr.sun_path); // 使用offsetof计算客户端本地套接字结构体的大小

    // 绑定前删除sock.ca,避免路径冲突
    unlink("sock.ca");
    if (bind(sockFd, (struct sockaddr *)&cAddr, len) < 0)
    {
        perror("bind");
        return 1;
    }

    // 3.连接服务器(创建有服务器信息的本地套接字结构体用以连接服务器)
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "sock.s");

    connect(sockFd, (struct sockaddr *)&addr, sizeof(addr));
    // 4.收发数据
    char buf[1500] = "";
    while (1)
    {
        // 向服务器发送信息
        memset(buf, 0, sizeof(buf));
        int n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n < 0)
        {
            perror("read");
            break;
        }
        send(sockFd, buf, sizeof(buf), 0);
        memset(buf, 0, sizeof(buf));
        // 接收服务器发来的信息
        n = recv(sockFd, buf, sizeof(buf), 0);
        if (n <= 0)
        {
            perror("recv");
            break;
        }
        printf("收到服务器信息:%s\n", buf);
    }

    // 5.关闭套接字
    close(sockFd);
    return 0;
}
