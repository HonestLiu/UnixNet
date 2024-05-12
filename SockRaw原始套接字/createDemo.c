#include <sys/socket.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

int main() {
    //创建原始套接字
    int sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0) {
        perror("create sock_raw");
    }
    printf("create sock_raw:%d\n", sockFd);
    close(sockFd);
}