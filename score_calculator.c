#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING 600

typedef struct {
    char id[MAX_STRING];
    char username[MAX_STRING];
    float latitude;
    float longitude;
    char clue[MAX_STRING];
    int value;
} Treasure;

typedef struct {
    char username[MAX_STRING];
    int score;
} UserScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s treasures.dat\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("No treasures found.\n");
        return 0;
    }
    UserScore users[100];
    int user_count = 0;
    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, fp) == 1) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, t.username) == 0) {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found) {
            strncpy(users[user_count].username, t.username, MAX_STRING);
            users[user_count].score = t.value;
            user_count++;
        }
    }
    fclose(fp);
    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", users[i].username, users[i].score);
    }
    return 0;
}