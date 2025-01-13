#include "image_handling.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int load_images(const char *directory, char ***image_paths) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    char **paths = NULL;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0)) {
                paths = realloc(paths, sizeof(char *) * (count + 1));
                if (!paths) {
                    perror("realloc");
                    closedir(dir);
                    return -1;
                }

                paths[count] = malloc(strlen(directory) + strlen(entry->d_name) + 2);
                if (!paths[count]) {
                    perror("malloc");
                    closedir(dir);
                    return -1;
                }

                sprintf(paths[count], "%s/%s", directory, entry->d_name);
                count++;
            }
        }
    }

    closedir(dir);
    *image_paths = paths;
    return count;
}

void shuffle_images(char **image_paths, int count) {
    srand((unsigned int)time(NULL));
    for (int i = count - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        char *temp = image_paths[i];
        image_paths[i] = image_paths[j];
        image_paths[j] = temp;
    }
}

void free_images(char **image_paths, int count) {
    for (int i = 0; i < count; ++i) {
        free(image_paths[i]);
    }
    free(image_paths);
}