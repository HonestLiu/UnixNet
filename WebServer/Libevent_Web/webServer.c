#include <stdio.h>
#include <stdlib.h>
#include <event.h>
#include <string.h>
#include <unistd.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pub.h"
#include <dirent.h>


void listener_cb(struct evconnlistener *evl, int fd, struct sockaddr *addr, int socklen, void *ptr);//监听函数

void read_cb(struct bufferevent *bev, void *ctx);//读事件回调函数

void processingRequests(struct bufferevent *bev, char path[256]);

void sendFile(struct bufferevent *bev, const char *path);

void sendHeader(struct bufferevent *bev, int code, const char *msg, char *type, int length);

int main() {
    /*切换工作目录到存放网页的目录*/
    char pwd_path[256] = "";//要切换的目录
    char *currentDirectory = getenv("PWD");//从环境变量PWD中获取当前的目录
    strcpy(pwd_path, currentDirectory);
    strcat(pwd_path, "/WebData");//将到切换的目录追加到其后，组成要切换的路径
    chdir(pwd_path);//切换到指定的目录

    /*忽略SIGPIPE信号的处理(保证当浏览器端切断连接后服务器不被杀死)*/
    //signal(SIGPIPE, SIG_IGN);

    /*设置监听*/
    struct event_base *base = event_base_new();//创建事件监听器
    struct sockaddr_in ServerAddr;
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(8889);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //当监听到客户端的时候会调用listener_cb回调函数，同时会将监听到的cfd传入到listener_cb函数
    struct evconnlistener *listener = evconnlistener_new_bind(base, listener_cb, base,
                                                              LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                                              (struct sockaddr *) &ServerAddr, sizeof(ServerAddr));
    if (!listener) {
        printf("Create Lister Error\n");
        return 1;
    }

    //进入事件循环
    event_base_dispatch(base);

    /*释放资源*/
    evconnlistener_free(listener);//释放监听器资源
    event_base_free(base);//释放事件管理器

}


//监听函数，负责将新连接的cfd包装成bufferEvent事件后上树
void listener_cb(struct evconnlistener *evl, int cfd, struct sockaddr *addr, int socklen, void *ptr) {
    //从实参中获取main函数的base事件管理器
    struct event_base *base = (struct event_base *) ptr;
    //创建bufferEvent事件
    struct bufferevent *bev = bufferevent_socket_new(base, cfd, BEV_OPT_CLOSE_ON_FREE);
   /* if (!bev) {
        //创建bufferEvent事件失败
        fprintf(stderr, "Error constructing bufferEvent!");
        event_base_loopbreak(base); // 退出事件循环(退出循环监听)
        return;
    }*/

    //设置bufferEvent的回调函数，设置完毕后会自动上树
    bufferevent_setcb(bev, read_cb, NULL, NULL, base);
    //使能读事件
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

//读事件回调函数，当客户端往缓冲区中写数据的时候会调用
void read_cb(struct bufferevent *bev, void *ctx) {
    char buf[256] = "";//存储客户端写入数据的数组
    int n = bufferevent_read(bev, buf, sizeof(buf));
    if (n < 0) {
        printf("bufferEvent_read error");
    }
    //提取请求方式、请求文件、HTTP版本
    char method[10] = "";
    char path[256] = "";
    char protocol[10] = "";
    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
    printf("method:%s;path:%s;protocol:%s\n", method, path, protocol);

    //将多余的数据读完
    while ((n = bufferevent_read(bev, buf, sizeof(buf))) > 0) {
        printf("%s\n", buf);
    }

    //如果是Get请求，就处理这个请求
    if (strcmp(method, "GET") == 0) {
        printf("%s\n",method);
        processingRequests(bev, path);
    }
}

/* bev:bufferEvent事件对象
 * path:要处理文件的路径
 * */
void processingRequests(struct bufferevent *bev, char path[256]) {
    //拼接一下路径
    char repath[256] = "";
    snprintf(repath, sizeof(repath), ".%s", path); //"./文件名"
    //检查服务器请求文件是否存在
    struct stat fileStat;
    if (stat(repath, &fileStat) < 0) {//请求文件不存在
        printf("请求的文件不存在");
        //先发送响应头
        sendHeader(bev, 404, "Not Found", get_mime_type("*.html"), 0);
        //再发送错误网页
        sendFile(bev, "./error.html");
        bufferevent_free(bev);
        return;
    } else if (S_ISREG(fileStat.st_mode)) {
        printf("请求的是普通文件\n");
        //先发送响应头
        sendHeader(bev, 200, "OK", get_mime_type(path), fileStat.st_size);
        //再发送实际文件
        sendFile(bev, repath);
        bufferevent_free(bev);
        return;
    } else if (S_ISDIR(fileStat.st_mode)) {
        printf("请求的是文件夹\n");
        //发送响应行
        sendHeader(bev, 200, "OK", get_mime_type("*.html"), 0);
        //发送目录页头
        sendFile(bev, "./SanfDir/front.html");
        //扫描目录
        struct dirent **namelist = NULL;
        int num = scandir(repath, &namelist, NULL, alphasort);
        if (num < 0) {
            perror("scandir error");
            return;
        }
        //遍历扫描到的目录。拼接成列表后发送
        char cache[2048] = "";
        int n = 0;
        for (int i = 0; i < num; i++) {
            memset(cache, 0, sizeof(cache));
            printf("ScanDir:%s\n", namelist[i]->d_name);
            //当前扫描到的是目录
            if ((namelist[i]->d_type & DT_DIR) == DT_DIR) {
                n = snprintf(cache, sizeof(cache), "<li><a href = \"%s/%s/\">%s</a></li>", repath,
                             namelist[i]->d_name, namelist[i]->d_name);
            } else if ((namelist[i]->d_type & DT_REG) == DT_REG) {
                n = snprintf(cache, sizeof(cache), "<li><a href = \"%s/%s\">%s</a></li>", repath,
                             namelist[i]->d_name, namelist[i]->d_name);
            }
            //发送拼接好的li标签
            bufferevent_write(bev, cache, n);
            //释放当前的namelist节点
            free(namelist[i]);

        }
        free(namelist);
        //发送目录页页尾
        sendFile(bev, "./SanfDir/behind.html");

        //释放资源
        bufferevent_free(bev);
        return;
    }


}

/* bev:bufferEvent事件对象
 * code:状态码
 * msg:状态信息
 * type:响应文件类型
 * length:响应文件长度(非必须)
 * */

/*void sendHeader(struct bufferevent *bev, int code, const char *msg, char *type, int length) {
    char buf[1024] = "";
    int state = 0;

    //组响应行
    int len = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", code, msg);
    state = bufferevent_write(bev, buf, len);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("%s\n", buf);

    //组信息报头
    len = snprintf(buf, sizeof(buf), "Content-Type:%s\r\n", type);
    state = bufferevent_write(bev, buf, len);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("%s\n", buf);

    //组消息报头(告知浏览器发送文件的大小),只有length大于0的时候发出
    if (length > 0) {
        len = snprintf(buf, sizeof(buf), "Content-Length:%d\r\n", length);
        state = bufferevent_write(bev, buf, len);
        if (state < 0) {
            printf("send error\n");
            return;
        }
        printf("%s\n",buf);
    }
    //发送空行，千万不要用strlen计算长度，否则会多一个字节，进而导致出错⭐
    state = bufferevent_write(bev, "\r\n", 2);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("标头发送成功\n");

}*/
void sendHeader(struct bufferevent *bev, int code, const char *msg, char *type, int length) {
    char buf[1500];
    int state = 0;

    // 组响应行
    int len = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", code, msg);
    state = bufferevent_write(bev, buf, len);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("%s\n", buf);

    // 组信息报头
    len = snprintf(buf, sizeof(buf), "Content-Type:%s\r\n", type);
    state = bufferevent_write(bev, buf, len);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("%s\n", buf);

    // 组消息报头(告知浏览器发送文件的大小),只有length大于0的时候发出
    if (length > 0) {
        len = snprintf(buf, sizeof(buf), "Content-Length:%d\r\n", length);
        state = bufferevent_write(bev, buf, len);
        if (state < 0) {
            printf("send error\n");
            return;
        }
        printf("%s\n", buf);
    }
    // 发送空行
    state = bufferevent_write(bev, "\r\n", 2);
    if (state < 0) {
        printf("send error\n");
        return;
    }
    printf("标头发送成功\n");
}


/* bev:bufferEvent事件对象
 * path:要发送文件的路径
 * */
void sendFile(struct bufferevent *bev, const char *path) {
    /*打开要发送的文件*/
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open sendFile Error");
        //释放资源
        close(fd);
        //bufferevent_free(bev);
        return;
    }
    char buf[5060] = "";
    int len = read(fd, buf, sizeof(buf));
    if (len < 0) {
        perror("read");
        //释放资源
        close(fd);
        //bufferevent_free(bev);
        return;
    }

    bufferevent_write(bev, buf, len);
    printf("文件已推送\n");
    //释放资源
    close(fd);
    //bufferevent_free(bev);
}
