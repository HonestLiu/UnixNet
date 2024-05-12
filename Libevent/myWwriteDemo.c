#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
/* typedef void (*evconnlistener_cb)(struct evconnlistener *evl, evutil_socket_t fd, struct sockaddr *cliaddr, int socklen, void *ptr); */
void listenCb(struct evconnlistener *evl, evutil_socket_t fd, struct sockaddr *cliaddr, int socklen, void *ptr);
void signalCb(evutil_socket_t sig, short events,void *arg);
void readcb(struct bufferevent *bev, void *ctx);
void conn_eventcb(struct bufferevent *bev, short events, void *user_data);
void writecb(struct bufferevent *bev, void *ctx);

int main(int argc, char const *argv[])
{
    // 创建事件管理器
    struct event_base *base = event_base_new();

    // 创建监听器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9095);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // 创建、绑定套接字并创建监听器
    struct evconnlistener *listener = evconnlistener_new_bind(base, listenCb, (void *)base,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                                              (struct sockaddr *)&addr, sizeof(addr));

    // 创建信号事件，处理错误和客户端中断信息
    struct event *signal_event = evsignal_new(base, SIGINT, signalCb, (void *)base);

    // 循环监听
    event_base_dispatch(base);
    // 释放监听器
    evconnlistener_free(listener);
    // 释放事件管理器
    event_base_free(base);
    return 0;
}

void listenCb(struct evconnlistener *evl, evutil_socket_t fd, struct sockaddr *cliaddr, int socklen, void *ptr)
{
    // 从参数中获取事件处理器
    struct event_base *base = (struct event_base *)ptr;
    // 创建客户端节点
    struct bufferevent *cb = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // 设置客户端触发回调
    bufferevent_setcb(cb, readcb, writecb, conn_eventcb, NULL);
    // 使能事件
    bufferevent_enable(cb, EV_READ | EV_WRITE);
    // 向客户端发送欢迎信息
    bufferevent_write(cb, "Weclome connect Server\n", sizeof("Weclome connect Server\n"));
}

/* void (*cb)(struct evhttp_request *, void *); */
void signalCb(evutil_socket_t sig,short events, void *arg)
{
    struct event_base *base = arg;
    struct timeval delay = {2, 0}; // 延时2秒

    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    event_base_loopexit(base, &delay); // 退出事件循环(退出循环监听)
}

/* typedef void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx); */
void readcb(struct bufferevent *bev, void *ctx)
{
    char buf[1500] = "";
    bufferevent_read(bev, buf, sizeof(buf));
    printf("收到客户端的信息:%s\n", buf);
    // 将信息回塞给客户端
    bufferevent_write(bev, buf, strlen(buf));
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF)
	{
		printf("Connection closed.\n");
	}
	else if (events & BEV_EVENT_ERROR)
	{
		printf("Got an error on the connection: %s\n",
			   strerror(errno)); /*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

/* typedef void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx); */
void writecb(struct bufferevent *bev, void *ctx)
{
    // 暂时闲置
}
