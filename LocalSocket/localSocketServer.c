#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> //offsetof所需
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h> //本地套接字结构体所需
/*
使用本地套接字实现TCP通信
 */

int main(int argc, char const *argv[])
{
    /* 删除指定的套接字文件(因为其只能用一次，每次使用都要对其进行删除) */
    unlink("sock.s");

    /*创建Unix流式套接字，使用AF_UNIX作为Socket的类型*/
    int lfd = socket(AF_UNIX, SOCK_STREAM, 0);

    /*绑定*/
    // 创建本地套接字结构体
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;       // 本地套接
    strcpy(addr.sun_path, "sock.s"); // 设置本地套接字文件路径

    // 计算本地套接字结构体的大小
    socklen_t len = offsetof(struct sockaddr_un, sun_path); // 计算sockaddr_un结构体从首地址到sun_path成员的长度，即sun_family的长度
    len += strlen(addr.sun_path);                           // 加上本地套接字中的文件路径长度

    bind(lfd, (struct sockaddr *)&addr, len);

    /*监听*/
    listen(lfd, 128);

    // 提取
    // 定义记录请求者信息的结构体
    struct sockaddr_un UserAddr;
    socklen_t len_a = sizeof(UserAddr);
    int cfd = accept(lfd, (struct sockaddr *)&UserAddr, &len_a);
    printf("new client=%s\n", UserAddr.sun_path); // 打印新连接的本地套接字所使用的文件路径(隐式连接不会显示)
    // 读写
    char buf[1500] = "";
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        int n = recv(cfd, buf, sizeof(buf), 0);
        if (n <= 0) // 出错了，直接break即可
        {
            perror("recv");
            break;
        }
        printf("读到客户端信息:%s\n", buf);
        // 将信息写回去
        send(cfd, buf, strlen(buf), 0);
    }

    // 关闭
    return 0;
}
