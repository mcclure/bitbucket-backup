#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);
extern int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes, int yflip);
}

// This program recreates a botched attempt to write a PNG exporter that flipped the Y axis as it encoded.
// The attempt failed because the PNG encoder was using a form of compression where colors were stored
// with values relative to their neighbors, but the flipping was confusing the notion of "neighbors".
// To produce the effect, we take three steps:

// 1. Pad the image with one row of black pixels, since the glitched encoder commits a memory access violation.
#define SAFE 1

// 2. Manually reverse the order of the pixel rows, so the flipped image looks right side up.
#define PREFLIP 1

// 3. Instruct the encoder to run its broken y-flip routine.
#define YFLIP 1

int main (int argc, const char * argv[]) {
    if (argc < 3) {
		fprintf(stderr, "Usage:\n\t%s infile.png outfile.png\n", argv[0]);
		return 1;
	}
	
	int iwidth, iheight;
	uint32_t *image = (uint32_t *)stbi_load(argv[1], &iwidth, &iheight, NULL, 4);

	if (!image) {
		fprintf(stderr, "Could not open image %s\n", argv[1]);
		return 1;
	}

#if SAFE
	uint32_t *oldimage = image;
	image = (uint32_t *)malloc(4*iwidth*(iheight+1));
	memset(image, 0, 4*iwidth);
	image += iwidth;
	memcpy(image, oldimage, 4*iwidth*iheight);
	free(oldimage);
#endif

#if PREFLIP
	for(int y = 0; y < iheight/2; y++) {
		for(int x = 0; x < iwidth; x++) {
			int c = x+y*iwidth;
			int c2 = x+(iheight-y-1)*iwidth;
			uint32_t temp = image[c];
			image[c] = image[c2];
			image[c2] = temp;
		}
	}
#endif

	stbi_write_png(argv[2], iwidth, iheight, 4, &image[0], 4*iwidth, YFLIP);
	
	return 0;
}
