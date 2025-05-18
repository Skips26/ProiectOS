#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CMD_FILE "/tmp/treasure_cmd.txt"
#define HUNT_DIR "hunt"
#define TREASURE_FILE "treasures.dat"
#define RESULT_FIFO "/tmp/treasure_results.fifo"
#define MAX_STRING 600

typedef struct {
    char id[MAX_STRING];
    char username[MAX_STRING];
    float latitude;
    float longitude;
    char clue[MAX_STRING];
    int value;
} Treasure;

void write_to_fifo(const char* msg) {
    int fd = open(RESULT_FIFO, O_WRONLY | O_NONBLOCK);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

void handle_list_hunts(int sig) {
    (void)sig;
    char result[4096] = "";
    DIR *dir = opendir(HUNT_DIR);
    if (!dir) {
        snprintf(result, sizeof(result), "Could not open hunt directory\n");
        write_to_fifo(result);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char treasure_path[1024];
        snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", HUNT_DIR, entry->d_name, TREASURE_FILE);
        FILE *fp = fopen(treasure_path, "rb");
        int count = 0;
        if (fp) {
            Treasure t;
            while (fread(&t, sizeof(Treasure), 1, fp) == 1) count++;
            fclose(fp);
        }
        char line[256];
        // Limit hunt name to 200 chars to avoid truncation
        snprintf(line, sizeof(line), "Hunt: %.200s | Treasures: %d\n", entry->d_name, count);
        strncat(result, line, sizeof(result)-strlen(result)-1);
    }
    closedir(dir);
    write_to_fifo(result);
}

void handle_list_treasures(int sig) {
    (void)sig;
    FILE *fp = fopen(CMD_FILE, "r");
    if (!fp) return;
    char cmd[256];
    fgets(cmd, sizeof(cmd), fp);
    fclose(fp);
    char hunt_id[128];
    sscanf(cmd, "list_treasures %s", hunt_id);
    char exec_cmd[256], buf[4096];
    snprintf(exec_cmd, sizeof(exec_cmd), "./treasure_manager.exe list %s", hunt_id);
    FILE *p = popen(exec_cmd, "r");
    if (!p) return;
    size_t n = fread(buf, 1, sizeof(buf)-1, p);
    buf[n] = 0;
    pclose(p);
    write_to_fifo(buf);
}

void handle_view_treasure(int sig) {
    (void)sig;
    FILE *fp = fopen(CMD_FILE, "r");
    if (!fp) return;
    char cmd[256];
    fgets(cmd, sizeof(cmd), fp);
    fclose(fp);
    char hunt_id[128], treasure_id[128];
    sscanf(cmd, "view_treasure %s %s", hunt_id, treasure_id);
    char exec_cmd[512], buf[4096];
    snprintf(exec_cmd, sizeof(exec_cmd), "./treasure_manager.exe view %s %s", hunt_id, treasure_id);
    FILE *p = popen(exec_cmd, "r");
    if (!p) return;
    size_t n = fread(buf, 1, sizeof(buf)-1, p);
    buf[n] = 0;
    pclose(p);
    write_to_fifo(buf);
}

void handle_stop_monitor(int sig) {
    (void)sig;
    printf("\n[Monitor] Shutting down after delay...\n\n");
    usleep(2000000);
    exit(0);
}

int main() {
    struct sigaction sa1 = { .sa_handler = handle_list_hunts, .sa_flags = SA_RESTART };
    struct sigaction sa2 = { .sa_handler = handle_list_treasures, .sa_flags = SA_RESTART };
    struct sigaction sa3 = { .sa_handler = handle_view_treasure, .sa_flags = SA_RESTART };
    struct sigaction sa4 = { .sa_handler = handle_stop_monitor, .sa_flags = SA_RESTART };

    sigemptyset(&sa1.sa_mask);
    sigemptyset(&sa2.sa_mask);
    sigemptyset(&sa3.sa_mask);
    sigemptyset(&sa4.sa_mask);

    sigaction(SIGUSR1, &sa1, NULL); // list_hunts
    sigaction(SIGUSR2, &sa2, NULL); // list_treasures
    sigaction(SIGHUP,  &sa3, NULL); // view_treasure
    sigaction(SIGINT,  &sa4, NULL); // stop_monitor

    printf("\n[Monitor] Running... PID: %d\n\n", getpid());
    while (1) {
        pause();
    }

    return 0;
}
