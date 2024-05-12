#include <sys/socket.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>

/*
 fd:原始套接字
 msg:要发送的帧数据
 len:发送帧数据的长度
 if_name；网卡名称
*/
int SendTo(int sockFd, unsigned char *msg, int len, char *if_name) {
    struct ifreq EthReq;//创建ifreq结构体
    strncpy(EthReq.ifr_name, if_name, IFNAMSIZ);//指定网卡名称

    int ret = ioctl(sockFd, SIOCGIFINDEX, &EthReq);//获取接口信息
    if (ret < 0) {
        perror("ioctl error");
        close(sockFd);
        _exit(-1);
    }
    //定义struct sockaddr_ll接口体，并将上面获取的接口信息赋值给其内的
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));//清0
    sll.sll_ifindex = EthReq.ifr_ifindex;//给sll赋值

    //使用sendto发送数据
    int sendNum = sendto(sockFd, msg, len, 0, (struct sockaddr *) &sll, sizeof(sll));
    return sendNum;
}

int main() {
    /*创建原始套接字*/
    int sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0) {
        perror("create sock_raw");
    }
    //打印创建的原始套接字
    printf("create sock_raw:%d\n", sockFd);

    /*发出ARP请求*/

    //组ARP包
    unsigned char ArpPack[512] = {
            //------------------mac头------------------
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff,/*目的mac地址，请求即未知，使用ff:ff:ff:ff:ff:ff*/
            0x00, 0x0c, 0x29, 0xc1, 0xd3, 0x13,/*源mac地址*/
            0x08, 0x06,/*帧类型*/
            //------------------arp头------------------
            0x00, 0x01,/*硬件类型*/
            0x08, 0x00,/*协议类型*/
            6,/*硬件地址长度*/
            4,/*协议地址长度*/
            0x00, 0x01,/*arp请求报文*/
            0x00, 0x0c, 0x29, 0xc1, 0xd3, 0x13,/*源mac地址*/
            192, 168, 141, 128,/*源IP*/
            0, 0, 0, 0, 0, 0,/*目的mac地址*/
            192, 168, 141, 1/*目的IP*/
    };

    //发出ARP帧
    int len = SendTo(sockFd, ArpPack, 42, "ens33");
    printf("发出长度为:%d的数据\n", len);

    /*接收ARP请求*/
    while (1) {
        unsigned char buf[1500] = "";
        recvfrom(sockFd, buf, sizeof(buf), 0, NULL, NULL);
        //解析数据报的类型
        unsigned short mac_type = ntohs(*(unsigned short *) (buf + 12));
        if (mac_type == 0x0806) {
            printf("上层为arp报文\n");
            //获取OP请求，判断是不是ARP应答
            unsigned short op = ntohs(*(unsigned short *) (buf + 20));
            if (op == 2) {
                //为应答，则提取回应的mac地址
                char src_mac[18] = "";
                sprintf(src_mac, "%02x:%02x:%02x:%02x:%02x:%02x", buf[0 + 6], buf[1 + 6], buf[2 + 6],
                        buf[3 + 6], buf[4 + 6], buf[5 + 6]);
                //提取回应的IP地址
                char src_ip[16] = "";
                inet_ntop(AF_INET, buf + 28, src_ip, 16);
                printf("ARP应答IP：%s;对应Mac地址:%s\n", src_ip, src_mac);
                break;
            }
        }
    }

    close(sockFd);
    return 0;
}