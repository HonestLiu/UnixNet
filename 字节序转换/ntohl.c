#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    char ip[4] = {2,1,168,192};
    int num = *(int *)ip;//将char类型的数组转换成ntohl函数需要的int类型
    int sum = ntohl(num);//将ip地址转换成网络字节序
    unsigned char *p = (unsigned char *)&sum;//将sum的值转换为unsigned char类型
    printf("%d.%d.%d.%d\n", *p, *(p + 1), *(p + 2), *(p + 3));//输出转换后的值
    return 0;
}
