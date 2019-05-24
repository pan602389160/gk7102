#ifndef __OSD_COMMON_H__
#define __OSD_COMMON_H__
#define OSD_TEXT_FONT_PATH_LENGTH	128
#define FT_INTEGER_BIT      (6)

#define FT_PIX_FLOOR(x)     ((x) & ~((1 << FT_INTEGER_BIT) - 1))
#define FT_PIX_INT(x)       ((x) >> FT_INTEGER_BIT)

#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

typedef struct span_s
{
    int x;
    int y;
    int width;
    int coverage;
} span_t;

typedef struct spans_s
{
    span_t span;
    struct spans_s* next;
} spans_t;

typedef struct rect_s
{
    int xmin;
    int xmax;
    int ymin;
    int ymax;
} rect_t;

typedef struct font_attribute_s
{
	char all_path[OSD_TEXT_FONT_PATH_LENGTH];
	char hlaf_path[OSD_TEXT_FONT_PATH_LENGTH];
	unsigned int size;
    /*0: no outline.*/
	unsigned int outline_width;
    /*0: no bold at all; positive is bold, negetive is thin in horizontal.*/
	signed int   hori_bold;
    /*0: no bold at all; positive is bold, negetive is thin in vertical.*/
	signed int   vert_bold;
    /*0 is normal (no italic), 100 is 100% italic.*/
	unsigned int italic;
    /*0: anti-alias is enabled, 1: disable anti-alias. Default is 0.*/
	unsigned int disable_anti_alias;
} font_attribute_t;

typedef struct bitmap_info_s
{
	int width;
	int height;
} bitmap_info_t;

typedef enum {
	VECTOR_FONT_LIB = 0, //Ê¸Á¿×Ö¿â
	LATTICE_FONT_LIB,	//µãÕó×Ö¿â
	MAX_FONT_LIB
} FONT_LIBRARY_TYPE;

/*clut, if 256 bit map it needs*/
typedef struct
{
	unsigned char	b;
	unsigned char	g;
	unsigned char	r;
	unsigned char	a;
} RGBQUAD;

typedef struct pixel_type_s
{
	RGBQUAD pixel_background;
	RGBQUAD pixel_font;
	RGBQUAD pixel_outline;
	RGBQUAD reserved;
} pixel_type_t;

typedef struct {
	RGBQUAD 	    *mapaddr;
	unsigned int	size;
	unsigned int	height;
	unsigned int	width;
	unsigned int 	startx;
	unsigned int 	starty;
	pixel_type_t    color;
} MAP_INFO;

typedef enum {
	LATTICE_BASE_BIT = 0,
	LATTICE_BASE_CHAR,
	LATTICE_BASE_INT
} LATTICE_BASE_TYPE;

typedef struct {
	LATTICE_BASE_TYPE type;	//lattice type
	int 	width;			//shape attribute.
	int 	height;
	int 	x;				//offset
	int 	y;
} LATTICE_INFO;

typedef void (*save_lattice_hook_f)(const void *, LATTICE_INFO *,
	void *);
typedef void * font_lib_handle;
typedef struct {
	save_lattice_hook_f	(*set_hook)(font_lib_handle , save_lattice_hook_f);
	int 				(*set_font_attribute)(font_lib_handle ,const font_attribute_t *);
	int 				(*get_font_attribute)(font_lib_handle ,font_attribute_t *);
	int 				(*char_to_bitmap)(font_lib_handle , const char *, LATTICE_INFO *, void *);
	int 				(*destory)(font_lib_handle);
} font_lib_func;

#define GET_FUNC(n) ((font_lib_func *)n)
#endif
