#include "loader.h"
#include <sys/mman.h>
#include <errno.h>

/*
 * Loads an image from a raw image file using memory-mapped I/O.
 *
 * The file is expected to be in the raw format written by
 * saveimage_mmap: a struct image header followed immediately by the
 * pixel data. This is not a BMP file. The file is mapped into memory
 * and the header is copied into image. The pixel data is not copied:
 * image->pixels is set to point directly into the mapping.
 *
 * The size of the mapping is computed from image->width and
 * image->height, so the caller must set these to the expected
 * dimensions before calling. They are then overwritten with the
 * values stored in the file.
 *
 * The mapping is read-only, so the pixel data must not be modified
 * through image->pixels. The mapping remains valid after this function
 * returns and must be released by the caller with munmap(), not free().
 * The mapping starts sizeof(struct image) bytes before image->pixels
 * and its length is width * height * sizeof(struct pixel) +
 * sizeof(struct image), using the dimensions passed in.
 *
 * Returns 0 on success, or -1 if the file cannot be opened or mapped.
 */
int loadimage_mmap(char* filename, struct image* image) {

	size_t alloc_size = (size_t)image->height * image->width * sizeof(struct pixel) + sizeof(struct image);

	int fd = open(filename, O_RDONLY);
	if (fd == -1) return -1;

	void *mapped = mmap(NULL,       // Desired start address (NULL lets OS choose)
         alloc_size, // Length of the mapping
         PROT_READ,  // Memory protection: readable
         MAP_PRIVATE, // Visibility: private to the process
         fd,                      // File descriptor: -1 for anonymous mapping
         0);

	close(fd);

	if (mapped == MAP_FAILED) return -1;

	struct image *header_in_file = (struct image *)mapped;
    image->width = header_in_file->width;
    image->height = header_in_file->height;

    image->pixels = (struct pixel *)((char *)mapped + sizeof(struct image));

    return 0;
}

/*
 * Saves an image to a raw image file using memory-mapped I/O.
 *
 * Creates the file with permissions 0644, or truncates it if it
 * already exists, then resizes it to hold a struct image header
 * followed by the pixel data. The file is mapped into memory, the
 * header and pixels are copied into the mapping, and the mapping is
 * synchronously flushed to disk.
 *
 * The output is a raw dump of in-memory structures, not a BMP file,
 * and is intended to be read back with loadimage_mmap on the same
 * platform.
 *
 * Returns 0 on success, or -1 if the file cannot be opened or mapped.
 * A failed flush to disk is reported but still returns 0.
 */
int saveimage_mmap(char* filename, struct image* image) {

	size_t header_size = sizeof(struct image);
	size_t alloc_size = (size_t)image->height * image->width * sizeof(struct pixel) + header_size;

	int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) return -1;

	if (ftruncate(fd, alloc_size) == -1) {
		close(fd);
		return -1;
	}

	void* mapped = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	close(fd);

	if (mapped == MAP_FAILED) return -1;

	struct image* image_header = (struct image*) mapped;
	image_header->width = image->width;
	image_header->height = image->height;

	struct pixel* image_pixels = (struct pixel*)((char*)mapped + header_size);
	size_t num_pixels = (size_t)image->width * image->height;

	for (size_t i = 0; i < num_pixels; i++) {
		image_pixels[i] = image->pixels[i];
	}

	msync(mapped, alloc_size, MS_SYNC);
	if (munmap(mapped, alloc_size) == -1) {
		return -1;
	}

	return 0;
}


/*
 * Loads an uncompressed 24-bit BMP file into an image.
 *
 * Returns 0 on success, or -1 if the file cannot be opened or is not
 * a 24-bit BMP.
 */
int loadimage(char* filename, struct image* image) {
    int fd = open(filename, O_RDONLY);
	uint32_t x, y;
	BMPHeader header;
	BMPInfoHeader infoHeader;

	if (fd == -1) return -1;

	header.type = 0;

	read(fd, &header, sizeof(BMPHeader));
	read(fd, &infoHeader, sizeof(BMPInfoHeader));

	if (header.type != 0x4D42 || infoHeader.bits != 24) {
        printf("corrupted header\n");
		close(fd);
		return -1;
	}
	int padding = (4 - (infoHeader.width * 3) % 4) % 4;

	lseek(fd, header.offset, SEEK_SET);

	/* Start from the last row */
	y = infoHeader.height - 1;

	image->pixels = malloc(sizeof(struct pixel) * image->width * image->height);

	do {
		for (x = 0; x < infoHeader.width; x++) {
			unsigned char color[3];
			read(fd, color, sizeof(unsigned char) * 3);
			image->pixels[x + y * image->width].r = color[0];
            image->pixels[x + y * image->width].g = color[1];
            image->pixels[x + y * image->width].b =  color[2];
		}
		lseek(fd, padding, SEEK_CUR);
	} while (y-- > 0); /* The post-increment here is important not
			    * to miss the last row. */

	close(fd);
	return 0;
}

/*
 * Saves an image to disk as an uncompressed 24-bit BMP file.
 * Returns 0 on success, or 1 if the file cannot be opened.
 */
int saveimage(char* filename, struct image* image) {
    /* Create if the file does not exist, overwrite otherwise. Set
	 * file permissions: 0644 */
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	int x, y;

	if (fd == -1) return 1;

	BMPHeader header = { 0x4D42, 54 + image->width * image->height * 3, 0, 0, 54 };
	BMPInfoHeader infoHeader = { 40, image->width, image->height, 1, 24, 0,
				     image->width * image->height * 3, 0, 0, 0, 0 };

	write(fd, &header, sizeof(BMPHeader));
	write(fd, &infoHeader, sizeof(BMPInfoHeader));

	int padding = (4 - (image->width * 3) % 4) % 4;

	/* Start by serializing the last row. */
	y = image->height - 1;
	do {
		for (x = 0; x < image->width; x++) {
			struct pixel p = image->pixels[x + y * image->width];
			unsigned char color[3] = {((uint32_t) p.r) & 0xFF,
						    ((uint32_t) p.g) & 0xFF,
						   ((uint32_t) p.b) & 0xFF};
			write(fd, color, sizeof(unsigned char) * 3);
		}
		for (int i = 0; i < padding; i++) {
			unsigned char pad = 0;
			write(fd, &pad, 1);
		}
	} while ( y-- > 0); /* The post-increment here is important
			    * not to miss the last row. */

	close(fd);
	return 0;
}