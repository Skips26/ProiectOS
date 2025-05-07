#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define CMD_FILE "/tmp/treasure_cmd.txt"
#define HUNT_DIR "hunt"
#define TREASURE_FILE "treasures.dat"
#define MAX_STRING 600

typedef struct {
    char id[MAX_STRING];
    char username[MAX_STRING];
    float latitude;
    float longitude;
    char clue[MAX_STRING];
    int value;
} Treasure;

void handle_list_hunts() {
    printf("\n");
    DIR *dir = opendir(HUNT_DIR);
    if (!dir) {
        perror("Could not open hunt directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build path to treasures.dat
        char treasure_path[1024];
        snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s", HUNT_DIR, entry->d_name, TREASURE_FILE);

        FILE *fp = fopen(treasure_path, "rb");
        int count = 0;

        if (fp) {
            Treasure t;
            while (fread(&t, sizeof(Treasure), 1, fp) == 1) {
                count++;
            }
            fclose(fp);
        }

        printf("Hunt: %s | Treasures: %d\n", entry->d_name, count);
    }

    closedir(dir);

    printf("\n");
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

    printf("\n[Monitor] Listing treasures for hunt '%s'\n\n", hunt_id);

    char exec_cmd[256];
    snprintf(exec_cmd, sizeof(exec_cmd), "./treasure_manager.exe list %s", hunt_id);
    system(exec_cmd);
    printf("\n");
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

    printf("\n[Monitor] Viewing treasure '%s' in hunt '%s'\n\n", treasure_id, hunt_id);

    char exec_cmd[512];
    snprintf(exec_cmd, sizeof(exec_cmd), "./treasure_manager.exe view %s %s", hunt_id, treasure_id);
    system(exec_cmd);
    printf("\n");
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
