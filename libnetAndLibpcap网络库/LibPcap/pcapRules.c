#include <pcap/pcap.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

void pcapHandler(u_char *user_data, const struct pcap_pkthdr *hdr, const u_char *net_data) {
    // 分析net_data
    printf("len -> %d\n", hdr->len);
}

int main() {
    //打开网络设备，创建句柄
    char errBuf[128] = "";
    pcap_t *pcap = NULL;
    pcap = pcap_open_live("eth0", 1500, 0, 0, errBuf);
    if (pcap == NULL) {
        perror("pcap_open_live");
        _exit(-1);
    }

    //编译规则
    struct bpf_program fp;//存放编译后规则的结构体
    char *str = "udp port 8000";//要编译的规则
    int ret = 0;
    ret = pcap_compile(pcap, &fp, str, 1, 0xffffff00);
    if (ret == -1) {
        perror("pcap_compile");
        _exit(-1);
    }

    //设置过滤规则
    ret = pcap_setfilter(pcap, &fp);
    if (ret == -1) {
        perror("pcap_setfilter");
        _exit(-1);
    }

    //循环捕获数据(阻塞)
    pcap_loop(pcap, -1, pcapHandler, NULL);

    //关闭句柄
    pcap_close(pcap);
    return 0;
}