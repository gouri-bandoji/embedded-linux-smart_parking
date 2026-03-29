#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_FILE "/dev/parking_sensor"

int main()
{
    int fd;
    char buffer[256];

    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device file");
        return -1;
    }

    while (1) {
        printf("Press Enter to check slot status...");
        getchar();

        if (read(fd, buffer, sizeof(buffer)) < 0) {
            perror("Failed to read data");
            close(fd);
            return -1;
        }
        
        printf("%s", buffer);
    }

    close(fd);
    return 0;
}
