#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    openlog("writer", LOG_PID, LOG_USER);

    if (argc != 3) {
        syslog(LOG_ERR, "Error: Two arguments are required - file path and text string");
        fprintf(stderr, "Error: Two arguments are required - file path and text string\n");
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    if (writefile == NULL || writestr == NULL) {
        syslog(LOG_ERR, "Error: Both file path and text string must be specified");
        fprintf(stderr, "Error: Both file path and text string must be specified\n");
        closelog();
        return 1;
    }

    FILE *file = fopen(writefile, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Error: Failed to create or write to the file '%s' - %s", writefile, strerror(errno));
        fprintf(stderr, "Error: Failed to create or write to the file '%s' - %s\n", writefile, strerror(errno));
        closelog();
        return 1;
    }

    if (fprintf(file, "%s", writestr) < 0) {
        syslog(LOG_ERR, "Error: Failed to write content to the file '%s' - %s", writefile, strerror(errno));
        fprintf(stderr, "Error: Failed to write content to the file '%s' - %s\n", writefile, strerror(errno));
        fclose(file);
        closelog();
        return 1;
    }

    fclose(file);

    syslog(LOG_DEBUG, "Writing '%s' to '%s'", writestr, writefile);
    printf("File '%s' successfully created with content '%s'\n", writefile, writestr);

    closelog();
    return 0;
}

