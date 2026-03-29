#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

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
        if (write(fd, buffer, sizeof(buffer)) < 0) {
            perror("Failed to trigger sensor");
            close(fd);
            return -1;
        }
        
        usleep(1000000);  // Wait for 1 second
        
        if (read(fd, buffer, sizeof(buffer)) < 0) {
            perror("Failed to read data");
            close(fd);
            return -1;
        }
        
        printf("Slot Status: %s", buffer);
    }

    close(fd);
    return 0;
}
