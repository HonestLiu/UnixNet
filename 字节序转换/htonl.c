#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char ip[4] = {192, 168, 1, 2};
    int num = *(int *)ip;//将ip的值转换为int类型
    int sum = htonl(num);//将主机字节序转换为网络字节序
    unsigned char *p = (unsigned char *)&sum;//将sum的值转换为unsigned char类型
    printf("%d.%d.%d.%d\n", *p, *(p + 1), *(p + 2), *(p + 3));//输出转换后的值
    return 0;
}

