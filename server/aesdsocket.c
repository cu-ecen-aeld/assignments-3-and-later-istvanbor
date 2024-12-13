#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define PORT 9000
#define FILENAME "/var/tmp/aesdsocketdata.txt"
#define BUFFER_SIZE 50000

int sockfd = -1;
int running = 1;

// Signal handler for SIGINT and SIGTERM
void handle_signal(int signo) {
    running = 0;
    if (sockfd != -1) close(sockfd);
    syslog(LOG_INFO, "Server shutting down...");

    if (remove(FILENAME) == 0) {
        syslog(LOG_INFO, "Deleted file: %s", FILENAME);
    } else {
        syslog(LOG_ERR, "Failed to delete file: %s", FILENAME);
    }
    exit(0);
}

void setup_signal_handling() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive message from client
    bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        syslog(LOG_ERR, "Error receiving data from client");
        close(client_fd);
        return;
    }

    // Save message to file
    int file_fd = open(FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd < 0) {
        syslog(LOG_ERR, "Error opening file for writing");
        close(client_fd);
        return;
    }

    if (write(file_fd, buffer, bytes_received) < 0) {
        syslog(LOG_ERR, "Error writing to file");
        close(file_fd);
        close(client_fd);
        return;
    }
    close(file_fd);

    // Read the content of the file and send it back to the client
    file_fd = open(FILENAME, O_RDONLY);
    if (file_fd < 0) {
        syslog(LOG_ERR, "Error opening file for reading");
        close(client_fd);
        return;
    }

    while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        if (send(client_fd, buffer, bytes_received, 0) < 0) {
            syslog(LOG_ERR, "Error sending data to client");
            close(file_fd);
            close(client_fd);
            return;
        }
    }

    close(file_fd);
    syslog(LOG_INFO, "Connection closed with client.");
    close(client_fd);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("Second fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    openlog("aesdserver", LOG_PID | LOG_CONS, LOG_USER);
    setup_signal_handling();

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Socket bind failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        syslog(LOG_INFO, "Running as daemon");
        daemonize();
    }

    if (listen(sockfd, 10) < 0) {
        syslog(LOG_ERR, "Socket listen failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "Server listening on port %d", PORT);

    while (running) {
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        handle_client(client_fd);
    }

    closelog();
    close(sockfd);
    return EXIT_SUCCESS;
}

