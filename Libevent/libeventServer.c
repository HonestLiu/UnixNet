#include <event.h>
#include <stdio.h>
#include "wrap.h"
#define _MAXEVENT_ 1024

// 定义一个数组记录已添加到事件管理器的事件结构体
/* 事件结构体
    - 文件描述符:用以定位事件的
    - 事件地址:用来释放事件的
 */
typedef struct eventStructure
{
    int fd;
    struct event *ev;
} eS;
// 事件数组
eS FdArr[_MAXEVENT_];

/* 初始化事件数组函数 */
void initEventArr()
{
    for (int i = 0; i < _MAXEVENT_; i++)
    {
        // 将fd设置初始值为-1，标识未使用
        FdArr[i].fd = -1;
        FdArr[i].ev = NULL;
    }
}

/* 添加元素到事件数组 */
int addEvent(int fd, struct event *ev)
{
    // 找空位，找到才能添加
    for (int i = 0; i < _MAXEVENT_; i++)
    {
        if (FdArr[i].fd == -1)
        {
            FdArr[i].fd = fd;
            FdArr[i].ev = ev;
            return 0;
        }
    }
    return -1;
}
/* 在事件数组中查找元素 */
struct event *getEvent(int fd)
{
    // 根据文件描述符查找事件的地址
    for (int i = 0; i < _MAXEVENT_; i++)
    {
        if (FdArr[i].fd == fd)
        {
            return FdArr[i].ev;
        }
    }
}

// 客户端事件的回调函数
void cfdcb(int cfd, short events, void *arg)
{
    // 读数据
    char buf[1500] = "";
    int n = Read(cfd, buf, sizeof(buf));
    if (n <= 0)
    {
        printf("读取错误或客户端关闭\n");
        event_del(getEvent(cfd));
    }
    else
    {
        printf("收到客户端:%s\n", buf);
        Write(cfd, buf, n);
    }
}

// 监听事件的回调函数
/* typedef void (*event_callback_fn)(evutil_socket_t fd, short events, void *arg); */
void lfdcb(int lfd, short events, void *arg)
{
    // 强转得到事件管理器
    struct event_base *base = (struct event_base *)arg;

    // 提取新的连接
    int cfd = Accept(lfd, NULL, NULL);

    // 创建事件
    struct event *ev = event_new(base, cfd, EV_READ | EV_PERSIST, cfdcb, NULL);

    // 将事件添加到事件数组
    addEvent(cfd, ev);

    // 事件上树
    event_add(ev, NULL);
}

int main(int argc, char const *argv[])
{
    // 创建、绑定套接字
    int lfd = tcp4bind(8080, NULL);

    // 监听
    Listen(lfd, 128);

    // 创建事件管理器
    struct event_base *base = event_base_new();

    // 初始化事件数组
    initEventArr();

    // 创建监听事件(lfd).回调函数将事件管理器也传递过去了，监听事件添加客户端事件要用⭐
    struct event *ev = event_new(base, lfd, EV_READ | EV_PERSIST, lfdcb, base);
    addEvent(lfd, ev);

    // 添加事件到事件管理器
    event_add(ev, NULL);

    // 循环监听
    event_base_dispatch(base); // 阻塞

    // 收尾
    close(lfd);            // 释放监听套接字
    event_base_free(base); // 释放事件管理器

    return 0;
}
