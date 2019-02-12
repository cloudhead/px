//
// tga.c
// TGA image encoding/decoding
//
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "color.h"
#include "tga.h"

#define TGA_TYPE_UNCOMPRESSED_RGB 2

bool tga_load(struct tga *t, const char *path)
{
	FILE *fp = fopen(path, "rb");

	if (! fp)
		return false;

	fread(&t->header.idlen, 1, 1, fp);
	fread(&t->header.colormaptype, 1, 1, fp);
	fread(&t->header.imagetype, 1, 1, fp);
	fread(&t->header.colormapoff, 2, 1, fp);
	fread(&t->header.colormaplen, 2, 1, fp);
	fread(&t->header.colormapdepth, 1, 1, fp);
	fread(&t->header.x, 2, 1, fp);
	fread(&t->header.y, 2, 1, fp);
	fread(&t->width, 2, 1, fp);
	fread(&t->height, 2, 1, fp);
	fread(&t->depth, 1, 1, fp);
	fread(&t->header.imagedesc, 1, 1, fp);

	infof("tga", "decode %s %hdx%hdx%d, type %d", path, t->width, t->height, t->depth, t->header.imagetype);

	if (t->header.imagetype != TGA_TYPE_UNCOMPRESSED_RGB) {
		return false;
	}

	t->data = calloc((size_t)t->width * (size_t)t->height, sizeof(rgba_t));

	size_t bytes = (size_t)t->depth / 8;
	rgba_t p;

	for (int i = 0; i < t->width * t->height; i++) {
		if (!fread(&p, bytes, 1, fp)) {
			fprintf(stderr, "error: unexpected EOF at offset %d\n", i);
			exit(-1);
		}
		t->data[i] = rgba(p.b, p.g, p.r, p.a);
	}
	t->size = t->width * t->height * sizeof(rgba_t);

	fclose(fp);

	return true;
}


int tga_save(rgba_t *pixels, size_t w, size_t h, char depth, const char *path)
{
	FILE *fp = fopen(path, "wb");

	if (!fp)
		return 1;

	short null = 0x0;

	fputc(0, fp); // ID length
	fputc(0, fp); // No color map
	fputc(2, fp); // Uncompressed 32-bit RGBA

	fwrite(&null, 2, 1, fp);  // Color map offset
	fwrite(&null, 2, 1, fp);  // Color map length
	fwrite(&null, 1, 1, fp);  // Color map entry size
	fwrite(&null, 2, 1, fp);  // X
	fwrite(&null, 2, 1, fp);  // Y
	fwrite(&w, 2, 1, fp);     // Width
	fwrite(&h, 2, 1, fp);     // Height
	fwrite(&depth, 1, 1, fp); // Depth
	fwrite(&null, 1, 1, fp);  // Image descriptor

	size_t bytes = (size_t)depth / 8;
	rgba_t p;

	for (uint32_t i = 0; i < w * h; i++) {
		p.r = pixels[i].b;
		p.g = pixels[i].g;
		p.b = pixels[i].r;
		p.a = pixels[i].a;

		if (! fwrite(&p, bytes, 1, fp)) {
			fprintf(stderr, "error: unexpected EOF at offset %d\n", i);
			exit(-1);
		}
	}
	fclose(fp);

	return 0;
}

void tga_release(struct tga *t)
{
	free(t->data);
}
