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
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int sockfd = -1;
int client_fd = -1;
bool run_server = true;

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signo) {
    syslog(LOG_INFO, "Caught signal, exiting");

    if (client_fd != -1) close(client_fd);
    if (sockfd != -1) close(sockfd);

    remove(FILE_PATH);
    exit(0);
}

// Daemonize the process
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Parent process exits
    }

    // Create a new session
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Change the working directory
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Redirect standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    // Open syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // Check for daemon mode
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemonize();
    }

    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Socket bind failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(sockfd, 10) < 0) {
        syslog(LOG_ERR, "Socket listen failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (run_server) {
        // Accept client connection
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Open file for appending and reading
        FILE *file = fopen(FILE_PATH, "a+");
        if (!file) {
            syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        // Receive data from client
        ssize_t bytes_received;
        while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
            fwrite(buffer, 1, bytes_received, file);
            fflush(file);

            if (strchr(buffer, '\n')) break;
        }

        if (bytes_received < 0) {
            syslog(LOG_ERR, "Receive failed: %s", strerror(errno));
        }

        // Send back file contents to client
        rewind(file);
        while (fgets(buffer, sizeof(buffer), file)) {
            send(client_fd, buffer, strlen(buffer), 0);
        }

        fclose(file);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
        client_fd = -1;
    }

    // Cleanup and exit
    close(sockfd);
    closelog();
    return 0;
}

