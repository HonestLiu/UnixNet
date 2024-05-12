/*
  This example program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello, World!\n";

static const int PORT = 9995;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
						struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

int main(int argc, char **argv)
{
	struct event_base *base;		 // 事件管理器，即根节点
	struct evconnlistener *listener; // 监听器
	struct event *signal_event;		 // 信号事件

	struct sockaddr_in sin = {0}; // 可以理解为是服务器的地址结构体，用于下面创建监听器的

#ifdef _WIN32 // windows下初始化socket
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	base = event_base_new(); // 创建一个事件管理器
	if (!base)
	{
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	// 初始化服务器地址结构体
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	// 创建一个监听器，绑定到base上，监听端口PORT
	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
									   LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
									   (struct sockaddr *)&sin,
									   sizeof(sin));

	if (!listener)
	{
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	// 创建一个信号事件，监听SIGINT信号，当信号发生时，调用signal_cb函数，这是一个宏函数
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	// 如果信号事件不为空，则添加到事件管理器中(上树监听)
	if (!signal_event || event_add(signal_event, NULL) < 0)
	{
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	// 进入事件循环
	event_base_dispatch(base);
	// 释放资源
	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}

// 监听器回调函数，当有新的连接fd到来时，调用此函数,将fd封装成bufferevent事件后上树
static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
			struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = user_data; // 从参数中获取事件管理器
	struct bufferevent *bev;			 // bufferevent事件

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE); // 创建一个bufferevent事件
	if (!bev)
	{
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base); // 退出事件循环(退出循环监听)
		return;
	}
	// 设置bufferevent的回调函数，此时已经自动上树了
	bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL); // 设置bufferevent的回调函数
	bufferevent_enable(bev, EV_WRITE);								// 使能写事件
	bufferevent_disable(bev, EV_READ);								// 禁用读事件

	bufferevent_write(bev, MESSAGE, strlen(MESSAGE)); // 向客户端发送消息
}

// bufferevent的写回调函数
static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0)
	{
		printf("flushed answer\n");
		bufferevent_free(bev);
	}
}

// bufferevent的事件回调函数
static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
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

// 信号回调函数
static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	struct timeval delay = {2, 0}; // 延时2秒

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay); // 退出事件循环(退出循环监听)
}
