#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>

#define PORT 9000
#define BUFFER_SIZE 50000
#define FILENAME "/var/tmp/aesdsocketdata.txt"

int server_fd;
int running = 1;

void handle_signal(int signal) {
    // Restarts accepting connection from new client until SIGINT or SIGTERM is received
    running = 0;
    close(server_fd);
    syslog(LOG_INFO, "Server shutting down...");

    // Delete the file before exiting
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

    // Receive message from client
    ssize_t received_bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (received_bytes <= 0) {
        syslog(LOG_ERR, "Error receiving data from client");
        perror("Error receiving data from client");
        close(client_fd);
        return;
    }

    // Don't automatically add a newline
    // buffer[received_bytes] = '\n'; // Remove this line to avoid adding an extra newline

    syslog(LOG_INFO, "Received message from client.");

    // Save message to file
    int file_fd = open(FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd < 0) {
        syslog(LOG_ERR, "Error opening file for writing");
        perror("Error opening file");
        close(client_fd);
        return;
    }

    // Write the message to the file without an extra newline
    if (write(file_fd, buffer, received_bytes) < 0) {
        syslog(LOG_ERR, "Error writing to file");
        perror("Error writing to file");
        close(file_fd);
        close(client_fd);
        return;
    }
    close(file_fd);

    // Read the content of the file and send it back to the client
    file_fd = open(FILENAME, O_RDONLY);
    if (file_fd < 0) {
        syslog(LOG_ERR, "Error opening file for reading");
        perror("Error opening file for reading");
        close(client_fd);
        return;
    }

    // Return the contents of the file to the client
    while ((received_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        if (send(client_fd, buffer, received_bytes, 0) < 0) {
            syslog(LOG_ERR, "Error sending data to client");
            perror("Error sending data to client");
            close(file_fd);
            close(client_fd);
            return;
        }
    }

    close(file_fd);

    // Log the connection closure
    syslog(LOG_INFO, "Connection closed with client.");
    close(client_fd);
}

void daemonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create a new session
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Fork again to ensure the daemon cannot acquire a terminal
    pid = fork();
    if (pid < 0) {
        perror("Second fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Change the working directory to root
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard file descriptors to /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_RDWR);  // stdout
    open("/dev/null", O_RDWR);  // stderr
}

int main(int argc, char *argv[]) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Open connection to syslog
    openlog("aesdserver", LOG_PID | LOG_CONS, LOG_USER);

    setup_signal_handling();

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        syslog(LOG_ERR, "Socket creation failed");
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed");
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to port 9000
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Bind failed");
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Check for daemon mode
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        syslog(LOG_INFO, "Running as daemon");
        daemonize();
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        syslog(LOG_ERR, "Listen failed");
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "Server listening on port %d", PORT);
    printf("Server listening on port %d\n", PORT);

    while (running) {
        int client_fd;

        // Accept a new client connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            syslog(LOG_ERR, "Accept failed");
            perror("Accept");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), ip_str, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "New connection established from %s.", ip_str);

        // Handle client communication
        handle_client(client_fd);
    }

    // Close syslog
    closelog();

    close(server_fd);
    return EXIT_SUCCESS;
}

