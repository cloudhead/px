//
// tga.h
// TGA image encoding/decoding
//
struct tga {
	struct {
		char        idlen;
		char        colormaptype;
		char        imagetype;
		short       colormapoff;
		short       colormaplen;
		char        colormapdepth;
		short       x, y;
		char        imagedesc;
	} header;

	short                   width, height;
	char                    depth;
	size_t                  size;
	rgba_t                 *data;
};

bool    tga_load(struct tga *t, const char *path);
int     tga_save(rgba_t *data, size_t w, size_t h, char depth, const char *path);
void    tga_release(struct tga *t);
