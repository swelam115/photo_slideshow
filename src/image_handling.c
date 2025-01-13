#include "image_handling.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Function to load image paths from a directory
int load_images(const char *directory, char ***image_paths) {
    DIR *dir = opendir(directory); // Open the directory
    if (!dir) { // Check if directory was opened successfully
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    char **paths = NULL;
    int count = 0;

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Check if entry is a regular file and has a valid image extension
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".bmp") == 0)) {
                // Allocate memory for the new path
                paths = realloc(paths, sizeof(char *) * (count + 1));
                if (!paths) {
                    perror("realloc");
                    closedir(dir);
                    return -1;
                }

                // Allocate memory for the full path and copy it
                paths[count] = malloc(strlen(directory) + strlen(entry->d_name) + 2);
                if (!paths[count]) {
                    perror("malloc");
                    closedir(dir);
                    return -1;
                }

                // Construct the full path
                sprintf(paths[count], "%s/%s", directory, entry->d_name);
                count++;
            }
        }
    }

    closedir(dir); // Close the directory
    *image_paths = paths; // Set the output parameter
    return count; // Return the number of images loaded
}

// Function to shuffle image paths
void shuffle_images(char **image_paths, int count) {
    srand((unsigned int)time(NULL)); // Seed the random number generator
    for (int i = count - 1; i > 0; --i) {
        int j = rand() % (i + 1); // Generate a random index
        // Swap the images at indices i and j
        char *temp = image_paths[i];
        image_paths[i] = image_paths[j];
        image_paths[j] = temp;
    }
}

// Function to free the memory allocated for image paths
void free_images(char **image_paths, int count) {
    for (int i = 0; i < count; ++i) {
        free(image_paths[i]); // Free each path
    }
    free(image_paths); // Free the array of paths
}