#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h> // For _mkdir
#include <sys/stat.h> // For _stat
#include <windows.h> // For Windows API

#define MAX_STRING 256

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
    snprintf(log_filename, sizeof(log_filename), "hunt/%s/%s_logs/log.txt", hunt_id, hunt_id);
    
    FILE *log_file = fopen(log_filename, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", action);  // Log the action
        fclose(log_file);
    } else {
        perror("Error opening log file");
    }
}

void create_hunt(const char *hunt_id) {
    printf("\n");

    char path[MAX_STRING];
    
    // Check if the parent 'hunt/' directory exists, if not, create it
    snprintf(path, sizeof(path), "hunt");
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (_mkdir(path) != 0) {
            perror("Error creating hunt directory");
            return;
        }
    }
    
    // Now create the specific hunt directory
    snprintf(path, sizeof(path), "hunt/%s", hunt_id);
    if (_mkdir(path) == 0) {
        // Create hunt logs folder with unique log folder name based on hunt_id
        snprintf(path, sizeof(path), "hunt/%s/%s_logs", hunt_id, hunt_id);
        if (_mkdir(path) == 0) {
            printf("Hunt %s created successfully.\n\n", hunt_id);
            // Log the creation of the hunt
            log_action(hunt_id, "Created hunt");
        } else {
            perror("Error creating hunt log folder");
        }
    } else {
        perror("Error creating hunt");
    }
}

void add_treasure(const char *hunt_id, Treasure treasure) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasure%s.dat", hunt_id, treasure.id);
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }
    fwrite(&treasure, sizeof(Treasure), 1, file);
    fclose(file);
    
    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Added Treasure %s with value %d", treasure.id, treasure.value);
    log_action(hunt_id, action);
    
    printf("Treasure %s added to hunt %s.\n\n", treasure.id, hunt_id);
}

void list_treasures(const char *hunt_id) {
    printf("\n");

    char dir_path[MAX_STRING];
    snprintf(dir_path, sizeof(dir_path), "hunt/%s", hunt_id);
    
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(strcat(dir_path, "/*.dat"), &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No treasures found in hunt %s.\n", hunt_id);
        return;
    }
    
    printf("Treasures in Hunt %s:\n", hunt_id);
    do {
        printf("%s\n", findFileData.cFileName);
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
    
    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Listed treasures in hunt %s", hunt_id);
    log_action(hunt_id, action);

    printf("\n");
}

void view_treasure(const char *hunt_id, const char *treasure_id) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasure%s.dat", hunt_id, treasure_id);
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }
    
    Treasure treasure;
    fread(&treasure, sizeof(Treasure), 1, file);
    fclose(file);
    
    printf("ID: %s\nUser: %s\nCoordinates: (%f, %f)\nClue: %s\nValue: %d\n", 
           treasure.id, treasure.username, 
           treasure.latitude, treasure.longitude, 
           treasure.clue, treasure.value);
    
    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Viewed Treasure %s", treasure_id);
    log_action(hunt_id, action);

    printf("\n");
}

void remove_treasure(const char *hunt_id, const char *treasure_id) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasure%s.dat", hunt_id, treasure_id);
    
    if (remove(filename) == 0) {
        printf("Treasure %s removed from hunt %s.\n", treasure_id, hunt_id);
        
        // Log the action
        char action[MAX_STRING];
        snprintf(action, sizeof(action), "Removed Treasure %s", treasure_id);
        log_action(hunt_id, action);
    } else {
        perror("Error removing treasure");
    }

    printf("\n");
}

void create_symlink(const char *hunt_id) {
    printf("\n");

    char target[MAX_STRING], link[MAX_STRING];
    snprintf(target, sizeof(target), "hunt/%s/%s_logs", hunt_id, hunt_id);
    snprintf(link, sizeof(link), "log/final_logs_%s", hunt_id);
    
    char command[MAX_STRING];
    snprintf(command, sizeof(command), "mklink /D \"%s\" \"%s\"", link, target);
    system(command);
    
    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Created symlink for hunt %s logs", hunt_id);
    log_action(hunt_id, action);

    printf("\n");
}


int main(int argc, char *argv[]) {
  
    if(argc == 1){
    printf("\nTreasure Hunt Commands:\n"
           "1. create_hunt <hunt_id>\n"
           "2. add <hunt_id> <treasure_id> <username> <latitude> <longitude> <clue> <value>\n"
           "3. list <hunt_id>\n"
           "4. view <hunt_id> <treasure_id>\n"
           "5. remove_treasure <hunt_id> <treasure_id>\n"
           "6. create_symlink <hunt_id>\n\n");
    }
    else if (argc < 3){
        printf("Usage: treasure_manager <command> <hunt_id> [params]\n");
        return 1;
    }
    
    char *command = argv[1];
    char *hunt_id = argv[2];
    
    if (strcmp(command, "create_hunt") == 0) {
        create_hunt(hunt_id);
    } else if (strcmp(command, "add") == 0 && argc == 9) {
        Treasure t;
        strcpy(t.id, argv[3]);
        strcpy(t.username, argv[4]);
        t.latitude = atof(argv[5]);
        t.longitude = atof(argv[6]);
        strcpy(t.clue, argv[7]);
        t.value = atoi(argv[8]);
        add_treasure(hunt_id, t);
    } else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "view") == 0 && argc == 4) {
        view_treasure(hunt_id, argv[3]);
    } else if (strcmp(command, "remove_treasure") == 0 && argc == 4) {
        remove_treasure(hunt_id, argv[3]);
    } else if (strcmp(command, "create_symlink") == 0) {
        create_symlink(hunt_id);
    } else {
        printf("Invalid command or parameters.\n");
    }
    return 0;
}