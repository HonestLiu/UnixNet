#include <pcap/pcap.h>
#include <unistd.h>

/* typedef void (*pcap_handler)(u_char *, const struct pcap_pkthdr *,  const u_char *); */
void pcapHandler(u_char *user_data, const struct pcap_pkthdr *hdr, const u_char *net_data) {
    // 分析net_data
    printf("len -> %d\n", hdr->len);
}

int main(int argc, char const *argv[]) {
    // 获取设备名
    char err_buf[128] = "";
    char *dev_name = pcap_lookupdev(err_buf);

    // 打开网络设备，和获取句柄
    pcap_t *pcap = pcap_open_live(dev_name, 1500, 0, 0, err_buf);
    if (pcap == NULL) {
        perror("pcap_open_live");
        _exit(-1);
    }

    // 循环获取网络数据
    pcap_loop(pcap, -1, pcapHandler, NULL);

    // 关闭句柄
    pcap_close(pcap);
    return 0;
}
