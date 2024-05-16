#include <stdio.h>
#include <stdlib.h>
/*
代码说明:这是一个CGI处理程序，配合boa服务器使用，当浏览器请求本程序时，其会通过QUERY_STRING环境变量获取浏览器
发送来的请求，并解析数据，做出相应数据后通过标准输出输出，CGI会重定向该标准输入，将其发送给浏览器
日期:2024.5.16
作者:HonestLiu
 */

int main(int argc, char const *argv[])
{
    printf("Content-Type:text/html\n\n"); // 使用CGI传输HTML文件必须先输出这个

    // 获取浏览器传输过来的数据(即url中?号后面的数据)
    char *text = getenv("QUERY_STRING"); // 通过CGI环境变量获取(GET方式)

    int data1 = 0;  // 数据2
    int data2 = 0;  // 数据1
    char ch = '\0'; // 运算符

    sscanf(text, "%d%c%d", &data1, &ch, &data2); // 解析数据

    // 根据运算符的不同执行不同的程序
    if (ch == '+') // 加法
    {
        printf("%d\n", data1 + data2);
    }
    else if (ch == '-') // 减法
    {
        printf("%d\n", data1 - data2);
    }

    return 0;
}
