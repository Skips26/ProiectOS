#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h> // For _mkdir
#include <sys/stat.h> // For _stat
#include <windows.h> // For Windows API
#include <time.h>


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
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);  // Single file for all treasures
    
    FILE *file = fopen(filename, "ab");  // Append mode
    if (!file) {
        perror("Error opening treasure file");
        return;
    }
    
    fwrite(&treasure, sizeof(Treasure), 1, file);
    fclose(file);
    
    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Added Treasure %s (User: %s)", treasure.id, treasure.username);
    log_action(hunt_id, action);
    
    printf("Treasure %s (User: %s) added to hunt %s.\n\n", treasure.id, treasure.username, hunt_id);
}


void list_treasures(const char *hunt_id) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }
    
    Treasure treasure;
    struct stat file_stat;
    
    if (stat(filename, &file_stat) == 0) {
        printf("Hunt: %s\n", hunt_id);
        printf("Total File Size: %I64d bytes\n", (long long)file_stat.st_size);
        printf("Last Modified: %s\n", ctime((time_t *)&file_stat.st_mtime));
    }

    printf("\nTreasures:\n");
    while (fread(&treasure, sizeof(Treasure), 1, file)) {
        printf("\nID: %s\nUser: %s\nCoordinates: (%f, %f)\nClue: %s\nValue: %d\n", 
               treasure.id, treasure.username, 
               treasure.latitude, treasure.longitude, 
               treasure.clue, treasure.value);
    }
    
    fclose(file);

    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Listed treasures in hunt %s", hunt_id);
    log_action(hunt_id, action);

    printf("\n");
}


void view_treasure(const char *hunt_id, const char *treasure_id) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }
    
    Treasure treasure;
    int found = 0;
    printf("Treasure(s) with ID: %s in Hunt %s:\n", treasure_id, hunt_id);
    
    while (fread(&treasure, sizeof(Treasure), 1, file)) {
        if (strcmp(treasure.id, treasure_id) == 0) {
            printf("\nID: %s\nUser: %s\nCoordinates: (%f, %f)\nClue: %s\nValue: %d\n", 
                   treasure.id, treasure.username, 
                   treasure.latitude, treasure.longitude, 
                   treasure.clue, treasure.value);
            found = 1;
        }
    }
    
    fclose(file);
    
    if (!found) {
        printf("No treasure found with ID %s.\n", treasure_id);
    }

    // Log the action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Viewed Treasure %s", treasure_id);
    log_action(hunt_id, action);

    printf("\n");
}


void remove_treasure(const char *hunt_id, const char *treasure_id) {
    printf("\n");

    char filename[MAX_STRING];
    snprintf(filename, sizeof(filename), "hunt/%s/treasures.dat", hunt_id);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening treasure file");
        return;
    }

    // Read all treasures
    Treasure treasures[100];
    int count = 0;
    while (fread(&treasures[count], sizeof(Treasure), 1, file)) {
        count++;
    }
    fclose(file);

    // Find the index of the treasure to remove and its numeric ID
    int indexToRemove = -1;
    int removed_id_num = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(treasures[i].id, treasure_id) == 0) {
            indexToRemove = i;
            removed_id_num = atoi(&treasures[i].id[1]); // Extract numeric part (T3 -> 3)
            break;
        }
    }

    if (indexToRemove == -1) {
        printf("Treasure %s not found in hunt %s.\n", treasure_id, hunt_id);
        return;
    }

    // Shift remaining elements
    for (int i = indexToRemove; i < count - 1; i++) {
        treasures[i] = treasures[i + 1];
    }
    count--; // Reduce treasure count

    // Re-index: Decrease IDs **greater than** the removed one
    for (int i = 0; i < count; i++) {
        int current_id_num = atoi(&treasures[i].id[1]); // Extract numeric part (T4 -> 4)
        if (current_id_num > removed_id_num) {
            snprintf(treasures[i].id, MAX_STRING, "T%d", current_id_num - 1);
        }
    }

    // Rewrite the updated treasures back to file
    file = fopen(filename, "wb");
    if (!file) {
        perror("Error rewriting treasure file");
        return;
    }

    fwrite(treasures, sizeof(Treasure), count, file);
    fclose(file);

    // Log action
    char action[MAX_STRING];
    snprintf(action, sizeof(action), "Removed Treasure %s and re-indexed remaining", treasure_id);
    log_action(hunt_id, action);

    printf("Treasure %s removed. Re-indexed remaining treasures.\n\n", treasure_id);
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