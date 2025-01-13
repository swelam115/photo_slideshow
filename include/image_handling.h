#ifndef IMAGE_HANDLING_H
#define IMAGE_HANDLING_H

// Function to load image paths from a directory
int load_images(const char *directory, char ***image_paths);

// Function to shuffle the loaded image paths
void shuffle_images(char **image_paths, int count);

// Function to free the memory allocated for image paths
void free_images(char **image_paths, int count);

#endif // IMAGE_HANDLING_H