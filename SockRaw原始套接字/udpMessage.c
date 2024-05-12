#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> //以太网mac报文
#include <netinet/ip.h>//IP报文
#include <netinet/udp.h>//udp报文

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

/*校验IP首部的函数*/
unsigned short checksum(unsigned short *buf, int len) {
    int nWord = len / 2;
    unsigned long sum;

    if (len % 2 == 1)
        nWord++;
    for (sum = 0; nWord > 0; nWord--) {
        sum += *buf;
        buf++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

/*UDP伪首部结构体*/
struct pseudoHeader {
    u_int32_t sAddr;//源IP地址
    u_int32_t dAddr;//目的IP地址
    u_int8_t flag;//我不知道，😂(记得查)
    u_int8_t pro;//8位协议
    u_int16_t len;//16位UDP长度
};


int main() {
    /*创建原始套接字*/
    int sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0) {
        perror("create sock_raw");
    }
    //打印创建的原始套接字
    printf("create sock_raw:%d\n", sockFd);

    /*读取要发送的数据*/
    char data[128] = "";;
    printf("请输入要发送的数据\n");
    scanf("%s", data);
    //udp输入的数据必须是偶数
    int data_len = strlen(data) + strlen(data) % 2;

    /*组包*/
    unsigned char buf[1500] = "";
    //以太网mac报文
    unsigned char des_mac[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x08};//目的mac地址
    unsigned char src_mac[] = {0x00, 0x0c, 0x29, 0xc1, 0xd3, 0x13};//源mac地址
    struct ether_header *etherHeader = (struct ether_header *) buf;//申请报文空间
    //构建以太网mac头部
    memcpy(etherHeader->ether_dhost, des_mac, 6);
    memcpy(etherHeader->ether_shost, src_mac, 6);
    etherHeader->ether_type = htons(0x0800);//协议为IP

    /*IP报文*/
    struct iphdr *ipHd = (struct iphdr *) (buf + 14);//14是以太网头部所占用的大小

    //封装报文数据
    ipHd->version = 4;//IPV4协议
    ipHd->ihl = 5;//首部长度
    ipHd->tos = 0;//服务类型
    ipHd->tot_len = htons(20 + 8 + data_len);//IP首部 + (UDP首部 + 数据长度)
    ipHd->id = htons(0);//标识
    ipHd->frag_off = htons(0);//偏移片
    ipHd->ttl = 128;//生存时间
    ipHd->protocol = 17;//即UDP协议
    ipHd->check = htons(0);//先以0填充，后继计算皇后再放置到这
    ipHd->saddr = inet_addr("192.168.141.128");//源IP
    ipHd->daddr = inet_addr("192.168.141.1");//目的IP

    //校验IP报文首部(不会)
    ipHd->check = checksum((unsigned short *) ipHd, 20);

    /*udp报文*/
    struct udphdr *udpHdr = (struct udphdr *) (buf + 14 + 20);//buf+mac报文长度+IP报文长度

    udpHdr->source = htons(8000);//非必要，除非需要对方回数据给你
    udpHdr->dest = htons(8000);//必须值，指定要发送目的地的端口
    udpHdr->uh_ulen = htons(8 + data_len);//UDP数据报长度 -> UDP首部长度 + 数据长度
    udpHdr->check = htons(0);//和IP报文一样，先置零，后计算填充

    //拷贝数据到UDP数据部分
    memcpy(buf + (14 + 20 + 8), data, data_len);

    //创建伪头部
    unsigned char checksumPackage[512] = "";
    struct pseudoHeader *udpPseudo = (struct pseudoHeader *) checksumPackage;
    udpPseudo->sAddr = inet_addr("192.168.141.128");
    udpPseudo->dAddr = inet_addr("192.168.141.1");
    udpPseudo->flag = 0;
    udpPseudo->pro = 17;//协议
    udpPseudo->len = htons(8 + data_len);//UDP头部长度 + 数据长度

    //组校验包,在伪首部后面加上UDP首部和数据
    memcpy(checksumPackage + 12, buf + 14 + 20, 8 + data_len);
    //进行校验，得到校验和并赋值
    udpHdr->check = checksum((unsigned short *) checksumPackage, 8 + 12 + data_len);//udp首部 + 伪首部 + 数据长度

    SendTo(sockFd, buf, 14 + 20 + 8 + data_len, "ens33");


    //释放资源
    close(sockFd);
    return 0;
}