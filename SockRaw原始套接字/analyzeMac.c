#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    /*创建原始套接字*/
    int sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0) {
        perror("create sock_raw");
        return 0;
    }
    printf("create sock_raw:%d\n", sockFd);
    while (1) {
        /*接收完整的帧数据*/
        unsigned char buf[1500] = "";
        recvfrom(sockFd, buf, sizeof(buf), 0, NULL, NULL);//此处是在链路层接收数据，无需设置socketAddr结构体接收数据，无意义

        /*解析mac报文*/
        char dst_mac[18] = "";//10-F6-0A-5B-DE-1C，共18位
        char src_mac[18] = "";
        printf("%s\n", buf);
        //解析目的mac地址
        sprintf(dst_mac, "%02x:%02x:%02x:%02x:%02x:%02x", buf[0], buf[1], buf[2], buf[3], buf[4],
                buf[5]);
        //解析源mac地址，向后移动6位让其到达源mac位置
        sprintf(src_mac, "%02x:%02x:%02x:%02x:%02x:%02x", buf[0 + 6], buf[1 + 6], buf[2 + 6],
                buf[3 + 6], buf[4 + 6], buf[5 + 6]);

        printf("目的mac:%s,源mac:%s\n", dst_mac, src_mac);

        //获取数据报类型,buf帧向后移动12位到达mac数据报类型位置(需要从大端转为小端)
        unsigned short mac_type = ntohs(*(unsigned short *) (buf + 12));
        if (mac_type == 0x0800) {
            /*解析IP报文*/
            printf("上层为IP报文\n");
            //移动帧的位置到IP报文的区域，并使用ip记录其起始值
            unsigned char *Ip = buf + 14;

            //解析协议类型*
            unsigned char ip_type = Ip[9];
            if (ip_type == 1) {
                printf("IP报文协议类型为ICMP\n");
            }
            if (ip_type == 2) {
                printf("IP报文协议类型为IGMP\n");
            }
            if (ip_type == 6) {
                printf("IP报文协议类型为TCP\n");

                /*解析TCP报文*/
                //获取IP报文的长度
                int ip_head_len = (Ip[0] & 0x0f) * 4;
                //移动Ip报文位置到TCP报文位置
                unsigned char *Tcp = Ip + ip_head_len;

                //解析源端口和目的端口
                unsigned short src_port = ntohs(*(unsigned short *) Tcp);
                unsigned short dst_port = ntohs(*(unsigned short *) Tcp + 2);
                printf("源端口:%hu;目的端口:%hu\n", src_port, dst_port);

                //计算TCP报文的长度
                int tcp_head_len = (((Tcp[12] & 0xf0) >> 4) & 0x0f) * 4;

                //打印获取到的数据
                printf("TCP报文的数据为:%s\n", Tcp + tcp_head_len);

            }
            if (ip_type == 17) {
                printf("IP报文协议类型为UDP\n");

                /*解析UDP报文*/
                //获取IP报文的长度
                int ip_head_len = (Ip[0] & 0x0f) * 4;
                //移动Ip报文位置到UDP报文位置
                unsigned char *Udp = Ip + ip_head_len;

                //解析源端口和目的端口
                unsigned short src_port = ntohs(*(unsigned short *) Udp);
                unsigned short dst_port = ntohs(*(unsigned short *) Udp + 2);
                printf("源端口:%hu;目的端口:%hu\n", src_port, dst_port);

                //打印获取到的数据
                printf("UDP报文的数据为:%s\n", Udp + 8);
            }

            //解析IP源、目的地址
            char src_ip[16] = "";
            char dst_ip[16] = "";//192.168.141.254\0,共16位
            //解析源IP
            inet_ntop(AF_INET, Ip + 12, src_ip, 16);
            //解析目的IP
            inet_ntop(AF_INET, Ip + 16, dst_ip, 16);

            printf("源IP:%s;目的IP:%s\n", src_ip, dst_ip);

        } else if (mac_type == 0x0806) {
            printf("上层为arp报文\n");
        } else if (mac_type == 0x8035) {
            printf("上层为RARP报文\n");
        }

    }

    close(sockFd);
}