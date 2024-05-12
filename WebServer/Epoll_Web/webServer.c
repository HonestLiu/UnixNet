#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "wrap.h"
#include "pub.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
/*
 * 目前只能处理非中文、小体积文件，待完善
 * */

void send_header(int cfd, int code, char *type, int length);

void SendData(int cfd, char *path, int epFd, struct epoll_event *ev);

int main()
{
    /*切换工作目录*/
    char pwd_path[256] = "";
    char *TPath = getenv("PWD"); // 从环境变量中获取当前的工作目录
    strcpy(pwd_path, TPath);
    strcat(pwd_path, "/HTML"); // 将要切换的目录追加到获取的路径下
    chdir(pwd_path);           // 切换到HTML目录下

    // 创建、绑定Socket
    int lfd = tcp4bind(8080, NULL);
    // 设置监听
    Listen(lfd, 128);

    // 创建树
    int epFd = epoll_create(1);
    // 监听套接字上树
    struct epoll_event ev, events[1024];
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epFd, EPOLL_CTL_ADD, lfd, &ev);

    // 循环监听
    while (1)
    {
        int n = epoll_wait(epFd, events, 1024, -1);
        if (n < 0)
        {
            perror("epoll_wait");
        }
        else if (n == 0)
        {
            continue; // 本轮没有连接，直接退出
        }
        else
        {
            for (int i = 0; i < n; i++)
            {
                if (events[i].data.fd == lfd && events[i].events & EPOLLIN)
                {
                    // 如果为lfd,提取监听到的cfd
                    int cfd = Accept(lfd, NULL, NULL);

                    // 设置cfd为非阻塞
                    int flag = fcntl(cfd, F_GETFL);
                    flag = flag | O_NONBLOCK;
                    fcntl(cfd, F_SETFL, flag);

                    printf("new client connect\n");
                    // 将监听到的cfd上树
                    ev.data.fd = cfd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epFd, EPOLL_CTL_ADD, cfd, &ev);
                }
                else if (events[i].events & EPOLLIN) // 除lfd外的其它标识符
                {
                    // 先读取第一行，判断客户端的请求信息
                    char buf[1500] = "";

                    // 读客户端发送来的第一行请求
                    int ret = Readline(events[i].data.fd, buf, sizeof(buf));
                    // int ret = read(events[i].data.fd, buf, sizeof(buf));
                    if (ret < 0)
                    {
                        perror("read");
                        // 下树
                        epoll_ctl(epFd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                        // 关闭套接字
                        close(events[i].data.fd);
                    }
                    else if (ret == 0) // 客户端关闭
                    {
                        printf("client close\n");
                        epoll_ctl(epFd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
                        close(events[i].data.fd);
                    }
                    else
                    {
                        // 写出数据
                        printf("client send data:%s\n", buf); // GET /index.jpg HTTP/1.1
                        // 截取请求行获取请求类型
                        char *requestType = strtok(buf, " "); // "GET"
                        printf("%s\n", requestType);
                        // 截出文件名
                        char *fileName = strtok(NULL, " "); // "/index.jpg"
                        printf("%s\n", fileName);

                        // 判断请求类型是不是GET类型，是的话才处理请求
                        if (strcmp("GET", requestType) == 0)
                        {

                            // send_header(events[i].data.fd, 200, fileType, fileName);
                            SendData(events[i].data.fd, fileName, epFd, &ev);
                        }
                    }
                    // 将剩下的数据全部抛弃
                    while (Readline(events[i].data.fd, buf, sizeof(buf)) >= 0)
                        ;
                }
            }
        }
    }
}

// 组包发送响应头
void send_header(int cfd, int code, char *type, int length)
{
    // 组状态行
    char state[128] = "";
    if (code == 200)
    {
        strcpy(state, "OK");
    }
    else if (code == 404)
    {
        strcpy(state, "Not Found");
    }

    char buf[1024] = "";
    int len = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", code, state);
    printf("%s\n", buf);
    send(cfd, buf, len, 0);

    // 组信息报头
    len = snprintf(buf, sizeof(buf), "Content-Type:%s\r\n", type);
    printf("%s", buf);
    send(cfd, buf, len, 0);

    // 发送消息头(对于发生文件来说是必须的)
    if (length > 0)
    {
        len = snprintf(buf, sizeof(buf), "Content-length:%d\r\n", length);
        send(cfd, buf, len, 0);
    }

    // 发送空行，千万不要用strlen计算长度，否则会多一个字节，进而导致出错⭐
    send(cfd, "\r\n", 2, 0);
}

// 发送文件
void SendData(int cfd, char *path, int epFd, struct epoll_event *ev)
{
    unsigned char buf[2048] = ""; // 存储读到数据的缓冲区
    int n = 0;                    // 读到数据的长度
    // 组路径
    char repath[1024] = "";
    snprintf(repath, sizeof(repath), ".%s", path); //"./文件名"
    printf("拼接后的路径:%s\n", repath);

    // 判断请求的文件是否存在
    struct stat fileStat;
    if (stat(repath, &fileStat) < 0) // 请求文件不存在
    {
        printf("请求的文件不存在");
        send_header(cfd, 404, get_mime_type("*.html"), 0);
        int fd = open("./error.html", O_RDONLY);

        n = read(fd, buf, sizeof(buf));
        send(cfd, buf, n, 0);
        close(fd);
        close(cfd);
        // 下树
        epoll_ctl(epFd, EPOLL_CTL_DEL, cfd, ev);
    }
    else if (S_ISREG(fileStat.st_mode))
    {
        printf("请求的是普通文件\n");

        // 发送响应行
        printf("读取的文件大小:%ld\n", fileStat.st_size);
        send_header(cfd, 200, get_mime_type(path), fileStat.st_size);

        // 发送文件
        int FileFd = open(repath, O_RDONLY);
        if (FileFd < 0)
        {
            perror("open error");
            return;
        }

        // 发送数据
        while ((n = read(FileFd, buf, sizeof(buf))) > 0)
        {
            ssize_t nrent = send(cfd, buf, n, 0);
            if (nrent < 0)
            {
                perror("send");
                close(FileFd);
                return;
            }
            printf("发送了%ld字节\n", nrent);
        }

        close(FileFd);
        close(cfd);
        // 下树
        epoll_ctl(epFd, EPOLL_CTL_DEL, cfd, ev);
    }
    else if (S_ISDIR(fileStat.st_mode))
    {
        // 发送响应行
        send_header(cfd, 200, get_mime_type("*.html"), 0);

        printf("请求的是目录\n");
        // 扫描目录
        struct dirent **namelist = NULL;
        int num = scandir(repath, &namelist, NULL, alphasort);
        if (num < 0)
        {
            perror("scanf");
            return;
        }

        // 先发送页头
        char cache[2048] = "";
        int ffd = open("./SanfDir/front.html", O_RDONLY);
        int j = read(ffd, cache, sizeof(cache));
        send(cfd, cache, j, 0);
        close(ffd);
        // 遍历扫描到目录文件,拼接列表并发送
        for (int i = 0; i < num; i++) // 这里的num是扫描到的文件数目
        {
            memset(cache, 0, sizeof(cache));
            printf("ScanDir:%s\n", namelist[i]->d_name);
            // 判断当前遍历到是不是目录，拼接目录路径的a标签的li标签
            if ((namelist[i]->d_type & DT_DIR) == DT_DIR)
            {

                j = snprintf(cache, sizeof(cache), "<li><a href = \".%s%s/\">%s</a></li>", repath, namelist[i]->d_name,
                             namelist[i]->d_name);
                printf("拼接文件夹路径:%s\n", cache);
            }
            // 当前遍历到的是普通文件，拼接文件路径的a标签的li标签
            else if ((namelist[i]->d_type & DT_REG) == DT_REG)
            {
                j = snprintf(cache, sizeof(cache), "<li><a href = \".%s%s\">%s</a></li>", repath, namelist[i]->d_name,
                             namelist[i]->d_name);
                printf("拼接文件路径:%s\n", cache);
            }
            // 发送拼接好的li标签
            send(cfd, cache, j, 0);
            // 释放无用的namelist成员
            free(namelist[i]);
        }
        // 释放整个namelist
        free(namelist);

        // 最后发送页尾
        int bfd = open("./SanfDir/behind.html", O_RDONLY);
        j = read(bfd, cache, sizeof(cache));
        send(cfd, cache, num, 0);
        close(bfd);

        // 最终发送文本，关闭客户端文件描述符并下树
        close(cfd);
        // 下树
        epoll_ctl(epFd, EPOLL_CTL_DEL, cfd, ev);
    }
}
