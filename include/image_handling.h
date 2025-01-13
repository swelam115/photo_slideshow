#ifndef IMAGE_HANDLING_H
#define IMAGE_HANDLING_H

int load_images(const char *directory, char ***image_paths);
void shuffle_images(char **image_paths, int count);
void free_images(char **image_paths, int count);

#endif // IMAGE_HANDLING_H