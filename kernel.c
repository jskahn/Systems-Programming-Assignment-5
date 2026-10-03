#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Applies a square kernel to an image (cross-correlation).
 *
 * Produces a new image where each output pixel is the weighted sum of
 * the ksize x ksize neighborhood centered on the corresponding input
 * pixel, multiplied by normalize. The kernel is applied as-is (not
 * flipped), so this is technically cross-correlation; the result is
 * identical to convolution for symmetric kernels.
 *
 * The input img is padded so that kernel operations that fall outside of the 
 * original image are multiplied by a black pixel (zero padding).
 *
 * img        Source image. Not modified.
 * kernel     Kernel weights in row-major order, containing ksize * ksize elements.
 * ksize      Width and height of the kernel. Should be odd
 * normalize  Scale factor applied to each weighted sum
 *                       (e.g., 1.0f / 9 for a 3x3 box blur).
 *
 * Returns a pointer to a newly allocated image with the same dimensions as img.
 *
 */
struct image* apply_kernel(struct image* img, int* kernel, int ksize, float normalize) {

    // check if ksize is not odd
    if (ksize % 2 == 0) {
        printf("ksize must be odd, ksize: %d\n", ksize);
        return NULL;
    }

    // allocate memory for new image
    struct image* applied_image = malloc(sizeof(struct image));

    if (applied_image == NULL) {
        printf("Allocation failure for applied image.");
        return NULL;
    }

    applied_image->width = img->width;
    applied_image->height = img->height;
    applied_image->pixels = malloc(sizeof(struct pixel) * img->width * img->height);

    if (applied_image->pixels == NULL) {
        printf("Allocation failure for applied_image->pixels.");
        free(applied_image);
        return NULL;
    }

    // iterate over each image pixel
    for (int new_r = 0;  new_r < img->height; new_r++) {
        for (int new_c = 0; new_c < img->width; new_c++) {

            float acc_r = 0.0f;
            float acc_g = 0.0f;
            float acc_b = 0.0f;


            int radius = (ksize - 1) / 2;
            int start_r = new_r - radius;
            int end_r = new_r + radius;
            int start_c = new_c - radius;
            int end_c = new_c + radius;

            // iterate through kernel
            for (int prev_r = start_r; prev_r <= new_r + radius; prev_r++) {
                for (int prev_c = start_c; prev_c <= new_c + radius; prev_c++) {

                    // skip if out of bounds for previous image
                    if (prev_r < 0 || prev_r > img->height - 1 
                        || prev_c < 0 || prev_c > img->width - 1) {
                        continue;
                    }

                    int k_r = prev_r - start_r;
                    int k_c = prev_c - start_c;

                    struct pixel prev = img->pixels[prev_r * img->width + prev_c];
                    int kern = kernel[k_r * ksize + k_c];

                    acc_r += kern * prev.r;
                    acc_g += kern * prev.g;
                    acc_b += kern * prev.b;
                }
            }

            acc_r *= normalize;
            acc_g *= normalize;
            acc_b *= normalize;

            struct pixel new_pixel;
            new_pixel.r = (int)(acc_r);
            new_pixel.g = (int)(acc_g);
            new_pixel.b = (int)(acc_b);

            applied_image->pixels[new_r * img->width + new_c] = new_pixel;
        }
    }

    return applied_image;
}

