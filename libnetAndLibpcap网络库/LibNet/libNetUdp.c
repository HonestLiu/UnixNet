#include <libnet.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

/* 代码说明:
 * 使用libNet库，编写一个UDP客户端，从标准输入读入用户输入，后通过UDP协议发出
 * 使用说明：使用前切记要修改代码中的IP地址和Mac地址，这些是专有的
 *
 * */
int main() {
    char err_buf[128] = "";
    //创建句柄并设置使用默认设备通信
    libnet_t *lb = libnet_init(LIBNET_LINK_ADV, "ens33", err_buf);
    if (lb == NULL) {
        perror("libNet_init");
        _exit(-1);
    }

    int count = 0;//循环接收标准输入的次数
    libnet_ptag_t udp_lpt = 0;//UDP数据包标签，初始值为0，表示新的数据包
    libnet_ptag_t ip_lpt = 0;//IP数据包标签，初始值为0，表示新的数据包
    libnet_ptag_t mac_lpt = 0;//MAC数据包标签，初始值为0，表示新的数据包
    while (count < 5) {
        //读取用户输入
        char data[128] = "";
        printf("请输入你要发送的msg:\n");
        scanf("%s", data);
        int dataLen = strlen(data) + strlen(data) % 2;//这里是将数据的长度凑成偶数(UDP必须)

        //构建UDP报文，下面UDP报文长度中的8是UDP的头部
        udp_lpt = libnet_build_udp(8000, 8000, 8 + dataLen, 0, data, dataLen, lb, udp_lpt);
        if (udp_lpt < 0) {
            perror("libNet_build_udp");
            _exit(-1);
        }

        //构建IP报文
        ip_lpt = libnet_build_ipv4(20 + 8 + dataLen, 0, 0, 0, 128, 17, 0, inet_addr("192.168.141.128"),
                                   inet_addr("192.168.141.1"), NULL, 0, lb, ip_lpt);
        if (ip_lpt < 0) {
            perror("libNet_build_ipv4");
            _exit(-1);
        }

        //构建mac报文
        unsigned char src_mac[] = {0x00, 0x0c, 0x29, 0xc1, 0xd3, 0x13};
        unsigned char dst_mac[] = {0x00, 0x50, 0x56, 0xC0, 0x00, 0x08};
        mac_lpt = libnet_build_ethernet(dst_mac, src_mac, 0x0800, NULL, 0, lb, mac_lpt);
        if (mac_lpt < 0) {
            perror("libNet_build_ethernet");
            _exit(-1);
        }

        //发送数据
        libnet_write(lb);
        count++;
    }

    //释放句柄
    libnet_destroy(lb);
    return 0;

}