#include <stdio.h>

int main(int argc, char const *argv[])
{
    unsigned short num = 0x0102;

    // 获取低地址,因为char为一个字节，所以其获取的地址亦为一个字节，可以获取低地址
    unsigned char *p = (unsigned char *)&num;

    if (*p == 0x01) // 低地址存高位字节，大端
    {
        printf("大端\n");
    }
    else if (*p == 0x02)
    {
        printf("小端\n");
    }

    return 0;
}
