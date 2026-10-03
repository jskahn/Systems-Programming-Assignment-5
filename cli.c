#include "kernel.h"
#include "loader.h"
#include <string.h>

int generate_pagefault() {

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


    if (strcmp(mode, "kernel") == 0) {

        // TODO: allocate the space needed for one image and load the image
        int width = atoi(argv[3]);
        int height = atoi(argv[4]);
    
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
    }
}
