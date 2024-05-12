#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char const *argv[])
{
    int srcfd = open("test.jpg",O_RDONLY);

    return 0;
}
