#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define MAX_STRING 600

typedef struct {
    char id[MAX_STRING];
    char username[MAX_STRING];
    float latitude;
    float longitude;
    char clue[MAX_STRING];
    int value;
} Treasure;

void log_action(const char *hunt_id, const char *action) {
    char log_filename[MAX_STRING];
    snprintf(log_filename, sizeof(log_filename), "hunt/%s/%s_logs/logged_hunt", hunt_id, hunt_id);

    FILE *log_file = fopen(log_filename, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", action);
        fclose(log_file);
    } else {
        perror("Error opening log file");
    }
}

void create_hunt(const char *hunt_id) {
    char path[MAX_STRING];

    snprintf(path, sizeof(path), "hunt");
    mkdir(path, 0777);

    snprintf(path, sizeof(path), "hunt/%s", hunt_id);
    if (mkdir(path, 0777) == 0) {
        snprintf(path, sizeof(path), "hunt/%s/%s_logs", hunt_id, hunt_id);
        if (mkdir(path, 0777) == 0) {
            printf("Hunt %s created successfully.\n", hunt_id);
            log_action(hunt_id, "Created hunt");
        } else {
            perror("Error creating log folder");
        }
    } else {
        perror("Error creating hunt directory");
    }
}

void add_treasure(const char *hunt_id, Treasure treasure) {
    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);

    FILE *file = fopen(filename, "ab");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }

    fwrite(&treasure, sizeof(Treasure), 1, file);
    fclose(file);

    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Added Treasure %.255s (User: %.255s)", treasure.id, treasure.username);
    log_action(hunt_id, action);

    printf("Treasure %s added.\n", treasure.id);
}

void list_treasures(const char *hunt_id) {
    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }

    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        printf("Hunt: %s\n", hunt_id);
        printf("Total File Size: %lld bytes\n", (long long)file_stat.st_size);
        printf("Last Modified: %s", ctime(&file_stat.st_mtime));
    }

    Treasure treasure;
    printf("\nTreasures:\n");
    while (fread(&treasure, sizeof(Treasure), 1, file)) {
        printf("ID: %s | User: %s | Coords: (%f, %f) | Clue: %s | Value: %d\n",
               treasure.id, treasure.username, treasure.latitude, treasure.longitude,
               treasure.clue, treasure.value);
    }

    fclose(file);
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Listed treasures in hunt %s", hunt_id);
    log_action(hunt_id, action);
}

void view_treasure(const char *hunt_id, const char *treasure_id) {
    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }

    Treasure treasure;
    int found = 0;

    while (fread(&treasure, sizeof(Treasure), 1, file)) {
        if (strcmp(treasure.id, treasure_id) == 0) {
            printf("ID: %s\nUser: %s\nCoords: (%f, %f)\nClue: %s\nValue: %d\n",
                   treasure.id, treasure.username, treasure.latitude,
                   treasure.longitude, treasure.clue, treasure.value);
            found = 1;
        }
    }

    fclose(file);
    if (!found) {
        printf("No treasure with ID %s found.\n", treasure_id);
    }

    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Viewed treasure %s", treasure_id);
    log_action(hunt_id, action);
}

void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }

    Treasure treasures[100];
    int count = 0;
    while (fread(&treasures[count], sizeof(Treasure), 1, file)) count++;
    fclose(file);

    int removed_id_num = -1;
    int new_count = 0;

    // First, filter out treasures to be removed, and track removed ID number
    for (int i = 0; i < count; i++) {
        if (strcmp(treasures[i].id, treasure_id) == 0) {
            if (removed_id_num == -1) {
                removed_id_num = atoi(&treasures[i].id[1]); // only need this once
            }
            continue; // skip this treasure
        }
        treasures[new_count++] = treasures[i]; // keep this one
    }

    if (removed_id_num == -1) {
        printf("Treasure not found.\n");
        return;
    }

    // Now adjust IDs of any treasures that had higher numbers
    for (int i = 0; i < new_count; i++) {
        int id_num = atoi(&treasures[i].id[1]);
        if (id_num > removed_id_num) {
            snprintf(treasures[i].id, MAX_STRING, "T%d", id_num - 1);
        }
    }

    // Rewrite the file with the updated treasure list
    file = fopen(filename, "wb");
    if (!file) {
        perror("Error rewriting treasure file");
        return;
    }

    fwrite(treasures, sizeof(Treasure), new_count, file);
    fclose(file);

    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Removed treasure(s) with ID %s", treasure_id);
    log_action(hunt_id, action);

    printf("Treasure(s) with ID %s removed.\n", treasure_id);
}


void remove_directory_recursive(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    char full_path[MAX_STRING];

    if (!dir) return;

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                remove_directory_recursive(full_path);
                rmdir(full_path);
            } else {
                unlink(full_path);
            }
        }
    }

    closedir(dir);
    rmdir(path);
}

void remove_symlink(const char *hunt_id) {
    char link_path[MAX_STRING];
    snprintf(link_path, sizeof(link_path), "logged_hunt-%s", hunt_id);
    unlink(link_path);
}

void remove_hunt(const char *hunt_id) {
    char hunt_path[MAX_STRING];
    snprintf(hunt_path, sizeof(hunt_path), "hunt/%s", hunt_id);
    remove_directory_recursive(hunt_path);
    remove_symlink(hunt_id);
    printf("Hunt %s removed.\n", hunt_id);
}

void create_symlink(const char *hunt_id) {
    char target[MAX_STRING], linkname[MAX_STRING];
    snprintf(target, sizeof(target), "hunt/%s/%s_logs/logged_hunt", hunt_id, hunt_id);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);
    symlink(target, linkname);

    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Created symlink: %.255s -> %.255s", linkname, target);
    log_action(hunt_id, action);
}

int main(int argc, char *argv[]) {
    if(argc == 1) {
        printf("\nTreasure Hunt Commands:\n"
               "1. create_hunt <hunt_id>\n"
               "2. add <hunt_id> <treasure_id> <username> <latitude> <longitude> <clue> <value>\n"
               "3. list <hunt_id>\n"
               "4. view <hunt_id> <treasure_id>\n"
               "5. remove_treasure <hunt_id> <treasure_id>\n"
               "6. remove_hunt <hunt_id>\n"
               "7. create_symlink <hunt_id>\n\n");
        return 0;
    }
    else if (argc < 3) {
        printf("Usage: treasure_manager <command> <hunt_id> [params]\n");
        return 1;
    }

    char *command = argv[1];
    char *hunt_id = argv[2];

    if (strcmp(command, "create_hunt") == 0) {
        create_hunt(hunt_id);
    }
    else if (strcmp(command, "add") == 0 && argc == 9) {
        Treasure t;
        strncpy(t.id, argv[3], MAX_STRING);
        strncpy(t.username, argv[4], MAX_STRING);
        t.latitude = atof(argv[5]);
        t.longitude = atof(argv[6]);
        strncpy(t.clue, argv[7], MAX_STRING);
        t.value = atoi(argv[8]);
        add_treasure(hunt_id, t);
    } 
    else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_id);
    }
    else if (strcmp(command, "view") == 0 && argc == 4) {
        view_treasure(hunt_id, argv[3]);
    } 
    else if (strcmp(command, "remove_treasure") == 0 && argc == 4) {
        remove_treasure(hunt_id, argv[3]);
    } 
    else if (strcmp(command, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } 
    else if (strcmp(command, "create_symlink") == 0) {
        create_symlink(hunt_id);
    } 
    else {
        printf("Invalid command.\n");
    }

    return 0;
}
