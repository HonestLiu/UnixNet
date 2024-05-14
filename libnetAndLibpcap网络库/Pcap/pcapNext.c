#include <pcap/pcap.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    /* 查看设备名称 */
    char err_buf[128] = ""; // 记录错误信息的数组
    char *dev_name = pcap_lookupdev(err_buf);
    printf("dev_name:%s\n", dev_name);

    /* 打开网络网络设备获取Pcap句柄 */
    pcap_t *pcap = pcap_open_live(dev_name, 1500, 0, 0, err_buf);
    if (pcap == NULL) {
        perror("pcap_open_live");
        _exit(-1);
    }

    /* 获取设备网络和掩码信息 */
    bpf_u_int32 ip;
    bpf_u_int32 mask;
    pcap_lookupnet(dev_name, &ip, &mask, err_buf);
    char ip_str[16] = "";
    char mask_str[16] = "";
    // 因为获取的是网络大端数据，所以使用前要转小
    inet_ntop(AF_INET, &ip, ip_str, 16);
    inet_ntop(AF_INET, &mask, mask_str, 16);
    printf("ip=%s mask=%s\n", ip_str, mask_str);

    /* 获取网络数据 */
    while (1) {
        struct pcap_pkthdr hdr;
        const unsigned *text = pcap_next(pcap, &hdr);
        printf("len = %d\n", hdr.len);
    }
    pcap_close(pcap);
    return 0;
}
