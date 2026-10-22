#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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
        if (c == '\n') {
            s[i] = '\0';
            return 0;
        }
        s[i++] = c;
    }
    return -1;
}

static void *handle_client(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    char request[64], reply[64];
    if (recv_line(fd, request, sizeof request) == 0) {
        char *end;
        errno = 0;
        long x = strtol(request, &end, 10);
        if (errno != 0 || end == request || *end != '\0' ||
            x > LONG_MAX / 5 || x < LONG_MIN / 5) {
            send_all(fd, "ERR\n", 4);
        } else {
            int len = snprintf(reply, sizeof reply, "%ld\n", x * 5);
            send_all(fd, reply, (size_t)len);
            printf("Received %ld, replied %ld\n", x, x * 5);
        }
    }
    close(fd);
    return NULL;
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof address) < 0 ||
        listen(server_fd, 3) < 0) {
        perror("bind/listen");
        close(server_fd);
        return 1;
    }
    printf("Listening on port %d\n", PORT);

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) { perror("accept"); continue; }
        int *arg = malloc(sizeof *arg);
        if (arg == NULL) { close(client_fd); continue; }
        *arg = client_fd;
        pthread_t worker;
        int error = pthread_create(&worker, NULL, handle_client, arg);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            free(arg);
            close(client_fd);
            continue;
        }
        pthread_join(worker, NULL);  // Base exercise: one client at a time
    }
}
