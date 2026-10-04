#include "kernel.h"
#include "loader.h"
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

int generate_pagefault() {

    struct image img;
    img.height = 1024 * 256;
    img.width = 1024;
    img.pixels = NULL;

    saveimage_mmap("fault.bin", &img);

    return 0;
}

/*
 * Converts bmp file to binary.
 */
int convert_bmp_to_bin(int width, int height, char *in_filepath, char *out_filepath) {
	struct image* in_image = malloc(sizeof(struct image));

	if (!in_image) {
        return -1;
    }

	in_image->width = width;
	in_image->height = height;
	in_image->pixels = NULL;

	// Load BMP and check for failure
    if (loadimage(in_filepath, in_image) != 0) {
        free(in_image);
        return -1;
    }

	// Open binary file for writing
	FILE *fp = fopen(out_filepath, "wb");

	if (!fp) {
        free(in_image->pixels);
        free(in_image);
        return -1;
    }

	// write header
	fwrite(in_image, sizeof(struct image), 1, fp);

	// write pixels
	fwrite(in_image->pixels, sizeof(struct pixel), width * height, fp);

	fclose(fp);
    free(in_image->pixels);
    free(in_image);

	return 0;
}

struct image* loadimage_from_bin(char *in_filepath) {
	FILE *fp = fopen(in_filepath, "rb");

	if (!fp) return NULL;
	
	struct image* in_image = malloc(sizeof(struct image));

	if (!in_image) {
		fclose(fp);
        return NULL;
    }

	fread(in_image, sizeof(struct image), 1, fp);

	in_image->pixels = malloc(sizeof(struct pixel) * in_image->width * in_image->height);

	if (!in_image->pixels) {
        fclose(fp);
        free(in_image);
        return NULL;
    }

	size_t pixels_read = fread(in_image->pixels, sizeof(struct pixel), in_image->width * in_image->height, fp);
    fclose(fp);

	if (pixels_read != in_image->width * in_image->height) {
        free(in_image->pixels);
        free(in_image);
        return NULL;
    }

	return in_image;
}

/*
 * Converts binary file to bmp.
 */
int convert_bin_to_bmp(char *in_filepath, char *out_filepath) {

	struct image* in_image = loadimage_from_bin(in_filepath);

	if (in_image == NULL) {
		return -1;
	}

	saveimage(out_filepath, in_image);

	free(in_image->pixels);
	free(in_image);

	return 0;
}

int main(int argc, char** argv){
    // TODO: parse the arguments in argv. 
    // You can expect argv[1] to be the mode
    // You can expect argv[2] to be the filepath
    // You can expect argv[3] to be the integer width
    // You can expect argv[4] to be the integer height
    // You can expect argv[5] to be the output filepath.

    if(argc != 6) {
        printf("Incorrect number of arguments. Expected: ./cli <MODE=kernel|mmap|convert|uconvert|fault> <input_image> <width> <height> <output_image_path>\n");
        return -1;
    }

    // TODO: call correct function based on mode
    char *mode = argv[1];
    char *in_filepath = argv[2];
    char *out_filepath = argv[5];

    int width = atoi(argv[3]);
    int height = atoi(argv[4]);


    if (strcmp(mode, "kernel") == 0) {

        // TODO: allocate the space needed for one image and load the image
    
        struct image* in_image = malloc(sizeof(struct image));
        in_image->width = width;
        in_image->height = height;
    
        loadimage(in_filepath, in_image);
    
        int kernel[3][3] = {{1,1,1},{1,1,1},{1,1,1}};
    
        // TODO: call apply kernel with 1/9 (as a float) as the normalization value
        struct image* out_image = apply_kernel(in_image, *kernel, 3, 1.0f / 9.0f);

        if (out_image == NULL) {
            printf("Unsuccessful application of kernal.\n");
            free(in_image->pixels);
            free(in_image);    
            return -1;
        }
        
        saveimage(out_filepath, out_image);

        free(out_image->pixels);
        free(out_image);
        free(in_image->pixels);
        free(in_image);

        return 0;
    } else if (strcmp(mode, "mmap") == 0) {

        struct image in_image;

        in_image.width = width;
        in_image.height = height;

        if (loadimage_mmap(in_filepath, &in_image) != 0) {
            printf("Failed to load image via mmap\n");
            return -1;
        }

        int kernel[3][3] = {{1,1,1},{1,1,1},{1,1,1}};
    
        // TODO: call apply kernel with 1/9 (as a float) as the normalization value
        struct image* out_image = apply_kernel(&in_image, *kernel, 3, 1.0f / 9.0f);

        if (out_image == NULL) {
            printf("Unsuccessful application of kernal.\n");
            
            void* mapping_start = (char*)in_image.pixels - sizeof(struct image);
            size_t mapping_len = (size_t)in_image.width * in_image.height * sizeof(struct pixel) + sizeof(struct image);
            if (munmap(mapping_start, mapping_len) == -1) {
                return -1;
            }

            return -1;
        }
        
        saveimage(out_filepath, out_image);

        free(out_image->pixels);
        free(out_image);
    
        void* mapping_start = (char*)in_image.pixels - sizeof(struct image);
        size_t mapping_len = (size_t)in_image.width * in_image.height * sizeof(struct pixel) + sizeof(struct image);
        if (munmap(mapping_start, mapping_len) == -1) {
            return -1;
        }

        return 0;

    } else if (strcmp(mode, "convert") == 0) {
        return convert_bmp_to_bin(width, height, in_filepath, out_filepath);
    } else if (strcmp(mode, "uconvert") == 0) {
        return convert_bin_to_bmp(in_filepath, out_filepath);
    }
}
