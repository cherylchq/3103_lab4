#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5000

static int send_all(int fd, const char *s, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, s, len, 0);
        if (n <= 0) return -1;
        s += n;
        len -= (size_t)n;
    }
    return 0;
}

static int recv_line(int fd, char *s, size_t size) {
    size_t i = 0;
    while (i + 1 < size) {
        char c;
        if (recv(fd, &c, 1, 0) != 1) return -1;
        if (c == '\n') { s[i] = '\0'; return 0; }
        s[i++] = c;
    }
    return -1;
}

int main(int argc, char **argv) {
    const char *ip = argc > 1 ? argv[1] : "192.168.3.1";
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP address: %s\n", ip);
        close(fd);
        return 1;
    }
    if (connect(fd, (struct sockaddr *)&address, sizeof address) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    long x;
    printf("Enter an integer: ");
    fflush(stdout);
    if (scanf("%ld", &x) != 1) { close(fd); return 1; }

    char request[64], reply[64];
    int len = snprintf(request, sizeof request, "%ld\n", x);
    if (send_all(fd, request, (size_t)len) != 0 ||
        recv_line(fd, reply, sizeof reply) != 0) {
        fprintf(stderr, "Communication failed\n");
        close(fd);
        return 1;
    }
    printf("Server replied: %s\n", reply);
    close(fd);
    return 0;
}
