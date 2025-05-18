#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>

#define CMD_FILE "/tmp/treasure_cmd.txt"
#define RESULT_FIFO "/tmp/treasure_results.fifo"

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

void* fifo_reader_thread(void* arg) {
    while (1) {
        int fd = open(RESULT_FIFO, O_RDONLY);
        if (fd < 0) {
            perror("open fifo");
            sleep(1);
            continue;
        }
        char buf[1024];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf)-1)) > 0) {
            buf[n] = 0;
            printf("\n[Monitor Result]\n%s\n", buf);
            printf("treasure_hub> ");
            fflush(stdout);
        }
        close(fd);
        // If read returns 0 (EOF), reopen FIFO to wait for new writers
    }
    return NULL;
}

void ensure_fifo() {
    struct stat st;
    if (stat(RESULT_FIFO, &st) != 0) {
        if (mkfifo(RESULT_FIFO, 0666) < 0 && errno != EEXIST) {
            perror("mkfifo");
            exit(1);
        }
    }
}

void run_score_calculators() {
    DIR *dir = opendir("hunt");
    if (!dir) {
        printf("No hunts found.\n");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        char treasure_path[1024];
        snprintf(treasure_path, sizeof(treasure_path), "hunt/%s/treasures.dat", entry->d_name);
        if (access(treasure_path, F_OK) != 0) continue;
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            continue;
        }
        pid_t pid = fork();
        if (pid == 0) {
            // Child: run score_calculator
            close(pipefd[0]);
            dup2(pipefd[1], 1);
            execl("./score_calculator", "score_calculator", treasure_path, (char*)NULL);
            perror("execl");
            exit(1);
        } else if (pid > 0) {
            // Parent: read output
            close(pipefd[1]);
            char buf[1024];
            ssize_t n = read(pipefd[0], buf, sizeof(buf)-1);
            if (n > 0) {
                buf[n] = 0;
                printf("\n[Score for %s]\n%s\n", entry->d_name, buf);
            }
            close(pipefd[0]);
            waitpid(pid, NULL, 0);
        }
    }
    closedir(dir);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];
    ensure_fifo();
    pthread_t tid;
    pthread_create(&tid, NULL, fifo_reader_thread, NULL);

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
        } else if (strcmp(input, "calculate_score") == 0) {
            run_score_calculators();
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
