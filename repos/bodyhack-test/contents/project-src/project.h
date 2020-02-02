/*
 *  project.h
 *  "c part"
 *
 *  Created by Andi McClure on 8/22/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _PROJECT_H
#define _PROJECT_H

#ifdef _WINDOWS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

#include <stdio.h>
#include "support/sdl.h"

// Functions needed in common to C files go here.
// If visibility to Lua is needed, put it in project.lua.

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)>(b))?(b):(a))

// image

EXPORT void set_color(SDL_Surface *surface, int x, int y, uint32_t color);
EXPORT uint32_t get_color(SDL_Surface *surface, int x, int y);

// ---- stb (images, oggs)

	// Load image. Remember comp / req_comp are writeback/request for # of image components
typedef unsigned char stbi_uc;
EXPORT stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
EXPORT stbi_uc *stbi_load            (char const *filename,     int *x, int *y, int *comp, int req_comp);
EXPORT stbi_uc *stbi_load_from_file  (FILE *f,                  int *x, int *y, int *comp, int req_comp);

	// Image cruft
EXPORT const char *stbi_failure_reason  (void); 
EXPORT void     stbi_image_free      (void *retval_from_stbi_load);
EXPORT int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
EXPORT int      stbi_info            (char const *filename,     int *x, int *y, int *comp);
EXPORT int      stbi_info_from_file  (FILE *f,                  int *x, int *y, int *comp);

	// Save image.
EXPORT int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
#define STB_IMAGE_WRITE_IMPLEMENTATION

	// Load ogg.
EXPORT int stb_vorbis_decode_memory(unsigned char *mem, int len, int *channels, short **output);
EXPORT int stb_vorbis_decode_filename(const char *filename, int *channels, short **output);

#endif /* _PROJECT_H */