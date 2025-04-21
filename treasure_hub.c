#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#define CMD_FILE "/tmp/treasure_cmd.txt"

pid_t monitor_pid = -1;
int monitor_running = 0;

void write_command(const char *command_line) {
    FILE *fp = fopen(CMD_FILE, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    fprintf(fp, "%s\n", command_line);
    fclose(fp);
}

void handle_sigchld(int sig) {
    (void)sig; // unused
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("Monitor terminated with status %d\n", status);
        monitor_running = 0;
        monitor_pid = -1;
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];
    while (1) {
        printf("treasure_hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor is already running.\n");
                continue;
            }
            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor.exe", "monitor", NULL);
                perror("execl failed");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("Monitor started with PID %d\n", monitor_pid);
            } else {
                perror("fork failed");
            }

        } else if (strncmp(input, "list_hunts", 10) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command("list_hunts");
            kill(monitor_pid, SIGUSR1);

        } else if (strncmp(input, "list_treasures", 14) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command(input);
            kill(monitor_pid, SIGUSR2);

        } else if (strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            write_command(input);
            kill(monitor_pid, SIGHUP);

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }
            kill(monitor_pid, SIGINT);
            printf("Sent stop signal to monitor...\n");

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Cannot exit: monitor still running. Use stop_monitor first.\n");
                continue;
            }
            break;
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
