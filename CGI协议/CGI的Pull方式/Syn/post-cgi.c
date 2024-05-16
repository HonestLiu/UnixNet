#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    printf("Content-Type:text/html\n\n");

    // 获取浏览器发送来数据的长度(比实际长度短1字节)
    char *len = getenv("CONTENT_LENGTH");
    // 通过标准输入得到浏览器发来的数据
    char data[512] = "";
    fgets(data, atoi(len) + 1, stdin);

    int data1 = 0;
    int data2 = 0;
    char ch = '\0';
    // 对数据进行解包
    sscanf(data, "%d%c%d", &data1, &ch, &data2);

    if (ch == '+')
    {
        printf("%d\n", data1 + data2);
    }
    else if (ch == '-')
    {
        printf("%d\n", data1 - data2);
    }

    return 0;
}
