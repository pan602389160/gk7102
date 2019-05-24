#include "osd_common.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRIGONOMETRY_H
#include FT_BITMAP_H
#include FT_STROKER_H
typedef struct {
	font_lib_func		func;
	font_attribute_t 	font;
	FT_Library 			library;
	FT_Face 			face;
	save_lattice_hook_f	save_data;
	int 				flag_face;
} VECTOR_FONT_LibraryT, *VECTOR_FONT_Handle;
//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static inline void set_font_italic(FT_GlyphSlot slot, const int italic);
static FT_Error font_hori_ft_outline(FT_Outline *outline, FT_Pos strength);
static FT_Error font_vert_ft_outline(FT_Outline *outline, FT_Pos strength);
static FT_Error font_new_ft_outline(FT_Outline *outline, FT_Pos str_h, FT_Pos str_v);
static int set_font_bold(FT_GlyphSlot slot, int v_str, int h_str);
static inline void rect_scope(rect_t *rect, int a, int b);
static void raster_callback(const int y, const int count, const FT_Span *spans, void* const user);
static void render_spans(FT_Library *library, FT_Outline *outline, spans_t *spans);
static void free_spans(spans_t *spans);
static unsigned char* set_font_outline(VECTOR_FONT_Handle fhandle, const unsigned int outline_width, int* width, int* height);
static save_lattice_hook_f	set_save_lattice_hook(font_lib_handle lib_handle,
	save_lattice_hook_f hook);
static int get_vector_font_attribute(font_lib_handle lib_handle,
	font_attribute_t *font_attr);
static int set_vector_font_attribute(font_lib_handle lib_handle,
	const font_attribute_t *font_attr);
static int char_to_bitmap_by_vector_library(font_lib_handle lib_handle,
	const char *char_code, LATTICE_INFO *lattice_info, void *hook_data);
static int destory_vector_font_library(font_lib_handle lib_handle);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

font_lib_handle create_vector_font_library(void)
{
	VECTOR_FONT_Handle fhandle = NULL;
	FT_Error ft_error = 0;

	fhandle = malloc(sizeof(VECTOR_FONT_LibraryT));
	if(fhandle == NULL) {
		printf("RAM space is no enough[%04x].", sizeof(VECTOR_FONT_LibraryT));
		return NULL;
	}
	memset(fhandle, 0, sizeof(VECTOR_FONT_LibraryT));
	ft_error = FT_Init_FreeType(&(fhandle->library));
	if (ft_error) {
		free(fhandle);
		printf("initilizate freetype library failed!");
		return NULL;
	}
	fhandle->func.set_hook = set_save_lattice_hook;
	fhandle->func.set_font_attribute = set_vector_font_attribute;
	fhandle->func.get_font_attribute = get_vector_font_attribute;
	fhandle->func.char_to_bitmap = char_to_bitmap_by_vector_library;
	fhandle->func.destory = destory_vector_font_library;

	return (font_lib_handle)fhandle;
}



//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

static int destory_vector_font_library(font_lib_handle lib_handle)
{
	VECTOR_FONT_Handle fhandle = (VECTOR_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
	FT_Done_Face(fhandle->face);
	FT_Done_FreeType(fhandle->library);
	fhandle->flag_face = 0;
	free(fhandle);
	return 0;
}

static save_lattice_hook_f	set_save_lattice_hook(font_lib_handle lib_handle,
	save_lattice_hook_f hook)
{
	save_lattice_hook_f tmp = NULL;
	VECTOR_FONT_Handle fhandle = (VECTOR_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return NULL;
	}
	if(hook == NULL) {
		printf("Hook function is NULL.");
		return NULL;
	}
	tmp = fhandle->save_data;
	fhandle->save_data = hook;
	return tmp;
}

static int set_vector_font_attribute(font_lib_handle lib_handle,
	const font_attribute_t *font_attr)
{
	FT_Error ft_error;
	VECTOR_FONT_Handle fhandle = (VECTOR_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
    if (font_attr == NULL)
    {
        printf("osd_text_set_font_attribute: input bad parameters !");
        return -1;
    }
    if (fhandle->flag_face == 1)
    {
        if ((ft_error = FT_Done_Face(fhandle->face)) != 0)
        {
            printf("Error! Destroy old attribute failed.");
            return -1;
        }
        fhandle->flag_face = 0;
    }

    ft_error = FT_New_Face(fhandle->library, font_attr->all_path, 0, &(fhandle->face));

    if (ft_error == FT_Err_Unknown_File_Format)
    {
        printf("Error! The font format in %s is unsupported.", font_attr->all_path);
        return -1;
    }
    else if (ft_error)
    {
        printf("The font file %s could not be opened or read. Error:%d!",
                font_attr->all_path, ft_error);
        return -1;
    }
    strncpy(fhandle->font.all_path, font_attr->all_path, sizeof(font_attr->all_path));
    strncpy(fhandle->font.hlaf_path, font_attr->hlaf_path, sizeof(font_attr->hlaf_path));
    fhandle->font.size = font_attr->size;
    fhandle->font.outline_width = font_attr->outline_width;
    fhandle->font.hori_bold = font_attr->hori_bold;
    fhandle->font.vert_bold = font_attr->vert_bold;
    fhandle->font.italic = font_attr->italic;
    fhandle->font.disable_anti_alias = font_attr->disable_anti_alias;
    ft_error = FT_Set_Pixel_Sizes(fhandle->face, fhandle->font.size, 0);
    if (ft_error)
    {
        printf("Set font size to pixel %d*%d failed.\n", fhandle->font.size,
			fhandle->font.size);
        return -1;
    }
    fhandle->flag_face = 1;

	return 0;
}

static int get_vector_font_attribute(font_lib_handle lib_handle,
	font_attribute_t *font_attr)
{
	VECTOR_FONT_Handle fhandle = (VECTOR_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
	if (font_attr == NULL)
    {
        printf("osd_text_get_font_attribute: input bad parameters!");
        return -1;
    }
    if (fhandle->flag_face == 0)
    {
        printf("None font attribute is set.");
        return 0;
    }
    strncpy(font_attr->all_path, fhandle->font.all_path, sizeof(font_attr->all_path));
    strncpy(font_attr->hlaf_path, fhandle->font.hlaf_path, sizeof(font_attr->hlaf_path));
    font_attr->size = fhandle->font.size;
    font_attr->outline_width = fhandle->font.outline_width;
    font_attr->hori_bold = fhandle->font.hori_bold;
    font_attr->vert_bold = fhandle->font.vert_bold;
    font_attr->italic = fhandle->font.italic;
    font_attr->disable_anti_alias = fhandle->font.disable_anti_alias;
    return 0;
}

static int char_to_bitmap_by_vector_library(font_lib_handle lib_handle,
	const char *char_code, LATTICE_INFO *lattice_info, void *hook_data)
{
	int ft_error;
	void *buffer = NULL;
	wchar_t ui_char_code = L' ';
	VECTOR_FONT_Handle fhandle = (VECTOR_FONT_Handle)lib_handle;
	if(fhandle == NULL || char_code == NULL) {
		printf("No have font library.");
		return -1;
	}
	mbtowc(&ui_char_code, char_code, 1);
	FT_GlyphSlot slot = fhandle->face->glyph;
	FT_UInt ui_glyph_index = FT_Get_Char_Index(fhandle->face, ui_char_code);
	if (fhandle->font.italic)
	{
		set_font_italic(slot, fhandle->font.italic);
	}

	ft_error = FT_Load_Glyph(fhandle->face, ui_glyph_index, FT_LOAD_NO_BITMAP);
	if (ft_error != 0)
	{
		printf("Load Glyph error!\n");
		return -1;
	}

	if (fhandle->font.hori_bold || fhandle->font.vert_bold)
	{
		set_font_bold(slot, fhandle->font.vert_bold * fhandle->font.size,
			fhandle->font.hori_bold * fhandle->font.size);
	}

	lattice_info->width  = FT_PIX_INT(slot->metrics.width);
	lattice_info->height = FT_PIX_INT(slot->metrics.height);
	if ((!fhandle->font.disable_anti_alias) &&
		(fhandle->font.outline_width) &&
		(fhandle->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE))
	{
		buffer = set_font_outline(fhandle, fhandle->font.outline_width,
			&(lattice_info->width), &(lattice_info->height));
	}
	else
	{
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
		buffer = slot->bitmap.buffer;
		lattice_info->width = slot->bitmap.pitch;
	}
	lattice_info->x    = FT_PIX_INT(slot->metrics.horiBearingX);
	lattice_info->y    = fhandle->font.size - FT_PIX_INT(slot->metrics.horiBearingY);
	lattice_info->type = LATTICE_BASE_CHAR;
	if(fhandle->save_data){
		fhandle->save_data(buffer, lattice_info, hook_data);
	}

	if (fhandle->font.outline_width && (buffer != NULL))
	{
		free(buffer);
		buffer = NULL;
	}
	return 1/*return use 1byte.*/;

}

static inline void set_font_italic(FT_GlyphSlot slot, const int italic)
{
    FT_Matrix matrix;
    matrix.xx = 0x10000L;
    matrix.xy = italic * 0x10000L / 100;
    matrix.yx = 0;
    matrix.yy = 0x10000L;
    FT_Set_Transform(slot->face, &matrix, 0);
}

static FT_Error font_hori_ft_outline(FT_Outline *outline, FT_Pos strength)
{
    FT_Vector *points;
    FT_Vector v_prev, v_first, v_next, v_cur;
    FT_Angle rotate, angle_in, angle_out;
    FT_Int c, n, first;
    FT_Int orientation;

    int last = 0;
    FT_Vector in, out;
    FT_Angle angle_diff;
    FT_Pos d;
    FT_Fixed scale;

    if (!outline)
        return FT_Err_Invalid_Argument;

    strength /= 2;
    if (strength == 0)
        return FT_Err_Ok;

    orientation = FT_Outline_Get_Orientation(outline);
    if (orientation == FT_ORIENTATION_NONE)
    {
        if (outline->n_contours)
            return FT_Err_Invalid_Argument;
        else
            return FT_Err_Ok;
    }

    if (orientation == FT_ORIENTATION_TRUETYPE)
        rotate = -FT_ANGLE_PI2;
    else
        rotate = FT_ANGLE_PI2;

    points = outline->points;

    first = 0;
    for (c = 0; c < outline->n_contours; c++)
    {
        last = outline->contours[c];

        v_first = points[first];
        v_prev = points[last];
        v_cur = v_first;

        for (n = first; n <= last; n++)
        {
            if (n < last)
                v_next = points[n+1];
            else
                v_next = v_first;

            // compute the in and out vectors
            in.x = v_cur.x - v_prev.x;
            in.y = v_cur.y - v_prev.y;

            out.x = v_next.x - v_cur.x;
            out.y = v_next.y - v_cur.y;

            angle_in = FT_Atan2(in.x, in.y);
            angle_out = FT_Atan2(out.x, out.y);
            angle_diff = FT_Angle_Diff(angle_in, angle_out);
            scale = FT_Cos(angle_diff / 2);

            if (scale < 0x4000L && scale > -0x4000L)
            {
                in.x = in.y = 0;
            }
            else
            {
                d = FT_DivFix(strength, scale);
                FT_Vector_From_Polar(&in, d, angle_in + angle_diff/2 - rotate);
            }

            outline->points[n].x = v_cur.x + strength + in.x;

            v_prev = v_cur;
            v_cur = v_next;
        }

        first = last + 1;
    }

    return FT_Err_Ok;
}

static FT_Error font_vert_ft_outline(FT_Outline *outline, FT_Pos strength)
{
    FT_Vector *points;
    FT_Vector v_prev, v_first, v_next, v_cur;
    FT_Angle rotate, angle_in, angle_out;
    FT_Int c, n, first;
    FT_Int orientation;

    int last = 0;
    FT_Vector in, out;
    FT_Angle angle_diff;
    FT_Pos d;
    FT_Fixed scale;

    if (!outline)
        return FT_Err_Invalid_Argument;

    strength /= 2;
    if (strength == 0)
        return FT_Err_Ok;

    orientation = FT_Outline_Get_Orientation(outline);
    if (orientation == FT_ORIENTATION_NONE)
    {
        if (outline->n_contours)
            return FT_Err_Invalid_Argument;
        else
            return FT_Err_Ok;
    }

    if (orientation == FT_ORIENTATION_TRUETYPE)
        rotate = -FT_ANGLE_PI2;
    else
        rotate = FT_ANGLE_PI2;

    points = outline->points;

    first = 0;
    for (c = 0; c < outline->n_contours; c++)
    {
        last = outline->contours[c];

        v_first = points[first];
        v_prev = points[last];
        v_cur = v_first;

        for (n = first; n <= last; n++)
        {
            if (n < last)
                v_next = points[n+1];
            else
                v_next = v_first;

            // compute the in and out vectors
            in.x = v_cur.x - v_prev.x;
            in.y = v_cur.y - v_prev.y;

            out.x = v_next.x - v_cur.x;
            out.y = v_next.y - v_cur.y;

            angle_in = FT_Atan2(in.x, in.y);
            angle_out = FT_Atan2(out.x, out.y);
            angle_diff = FT_Angle_Diff(angle_in, angle_out);
            scale = FT_Cos(angle_diff / 2);

            if (scale < 0x4000L && scale > -0x4000L)
            {
                in.x = in.y = 0;
            }
            else
            {
                d = FT_DivFix(strength, scale);
                FT_Vector_From_Polar(&in, d, angle_in + angle_diff/2 - rotate);
            }

            outline->points[n].y = v_cur.y + strength + in.y;

            v_prev = v_cur;
            v_cur = v_next;
        }

        first = last + 1;
    }

    return FT_Err_Ok;
}

static FT_Error font_new_ft_outline(FT_Outline *outline, FT_Pos str_h, FT_Pos str_v)
{
    if (!outline)
        return FT_Err_Invalid_Argument;
    int orientation = FT_Outline_Get_Orientation(outline);
    if ((orientation == FT_ORIENTATION_NONE) && (outline->n_contours))
    {
        return FT_Err_Invalid_Argument;
    }
    font_vert_ft_outline(outline, str_v);
    font_hori_ft_outline(outline, str_h);
    return FT_Err_Ok;
}

static int set_font_bold(FT_GlyphSlot slot, int v_str, int h_str)
{
    if (v_str == 0 && h_str == 0)
        return 0;
    FT_Library ft_library = slot->library;
    FT_Error ft_error;
    FT_Pos xstr = v_str, ystr = h_str;

    if (slot->format != FT_GLYPH_FORMAT_OUTLINE &&
        slot->format != FT_GLYPH_FORMAT_BITMAP)
        return -1;
    if (slot->format == FT_GLYPH_FORMAT_OUTLINE)
    {
        FT_BBox old_box;
        FT_Outline_Get_CBox(&slot->outline, &old_box);
        ft_error = font_new_ft_outline(&slot->outline, xstr, ystr);
        if (ft_error)
            return ft_error;

        FT_BBox new_box;
        FT_Outline_Get_CBox(&slot->outline, &new_box);
        xstr = (new_box.xMax - new_box.xMin) - (old_box.xMax - old_box.xMin);
        ystr = (new_box.yMax - new_box.yMin) - (old_box.yMax - old_box.yMin);
    }
    else if (slot->format == FT_GLYPH_FORMAT_BITMAP)
    {
        xstr = FT_PIX_FLOOR(xstr);
        if (xstr == 0)
            xstr = 1 << 6;
        ystr = FT_PIX_FLOOR(ystr);

        ft_error = FT_Bitmap_Embolden(ft_library, &slot->bitmap, xstr, ystr);
        if (ft_error)
            return ft_error;
    }

    if (slot->advance.x)
        slot->advance.x += xstr;

    if (slot->advance.y)
        slot->advance.y += ystr;

    slot->metrics.width += xstr;
    slot->metrics.height += ystr;
    slot->metrics.horiBearingY += ystr;
    slot->metrics.horiAdvance += xstr;
    slot->metrics.vertBearingX -= xstr/2;
    slot->metrics.vertBearingY += ystr;
    slot->metrics.vertAdvance += ystr;

    if (slot->format == FT_GLYPH_FORMAT_BITMAP)
        slot->bitmap_top += FT_PIX_INT(ystr);

    return 0;
}

static inline void rect_scope(rect_t *rect, int a, int b)
{
    rect->xmin = MIN(rect->xmin, a);
    rect->ymin = MIN(rect->ymin, b);
    rect->xmax = MAX(rect->xmax, a);
    rect->ymax = MAX(rect->ymax, b);
}

static void raster_callback(const int y, const int count, const FT_Span *spans, void* const user)
{
    int i;
    spans_t *sptr = (spans_t *)user, *sp = NULL;
    while (sptr->next != NULL) {
        sptr = sptr->next;
    }
    for (i = 0; i < count; i++) {
        sp = (spans_t *)malloc(sizeof(spans_t));
        sp->span.x = spans[i].x;
        sp->span.y = y;
        sp->span.width = spans[i].len;
        sp->span.coverage = spans[i].coverage;
        sp->next = NULL;

        sptr->next = sp;
        sptr = sptr->next;
    }
}

static void render_spans(FT_Library *library, FT_Outline *outline, spans_t *spans)
{
    FT_Raster_Params params;
    memset(&params, 0, sizeof(params));
    params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
    params.gray_spans = raster_callback;
    params.user = spans;

    FT_Outline_Render(*library, outline, &params);
}

static void free_spans(spans_t *spans)
{
    spans_t *iter = spans, *tmp = NULL;
    while (iter != NULL)
    {
        tmp = iter;
        iter = iter->next;
        free(tmp);
    }
}


/*Generate a bigger bitmap as background, and put the original bitmap on the top.*/
static unsigned char* set_font_outline(VECTOR_FONT_Handle fhandle, const unsigned int outline_width,
                                                        int* width, int* height)
{
    unsigned int linesize = outline_width * fhandle->font.size;
    FT_GlyphSlot slot = fhandle->face->glyph;
    unsigned char *pixel = NULL;
    spans_t *spans = NULL, *outlinespans = NULL;

    // Render the basic glyph to a span list.
    spans = (spans_t *)malloc(sizeof(spans_t));
    if (spans == NULL)
    {
        printf("spans malloc error\n");
        return NULL;
    }
    spans->span.x = 0;
    spans->span.y = 0;
    spans->span.width = 0;
    spans->span.coverage = 0;
    spans->next = NULL;
    render_spans(&(fhandle->library), &(slot->outline), spans);

    // spans for the outline
    outlinespans = (spans_t *)malloc(sizeof(spans_t));
    if (outlinespans == NULL)
    {
        printf("outlinespans malloc error\n");
        return NULL;
    }
    outlinespans->span.x = 0;
    outlinespans->span.y = 0;
    outlinespans->span.width = 0;
    outlinespans->span.coverage = 0;
    outlinespans->next = NULL;

    // Set up a stroker
    FT_Stroker stroker;
    FT_Stroker_New(fhandle->library, &stroker);
    FT_Stroker_Set(stroker,
            linesize,
            FT_STROKER_LINECAP_ROUND,
            FT_STROKER_LINEJOIN_ROUND,
            0);

    FT_Glyph glyph;
    if (FT_Get_Glyph(slot, &glyph) == 0)
    {
        FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1);   // outside border. Destroyed on success.
        if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
        {
            // Render the outline spans to the span list
            FT_Outline *ol = &(((FT_OutlineGlyph)glyph)->outline);
            render_spans(&(fhandle->library), ol, outlinespans);
        }

        // Clean up afterwards.
        FT_Stroker_Done(stroker);
        FT_Done_Glyph(glyph);

        // Put them together
        if (outlinespans->next != NULL)
        {
            rect_t rect;
            rect.xmin = spans->span.x;
            rect.xmax = spans->span.x;
            rect.ymin = spans->span.y;
            rect.ymax = spans->span.y;

            spans_t *iter = NULL;
            for (iter = spans; iter != NULL; iter = iter->next)
            {
                rect_scope(&rect, iter->span.x, iter->span.y);
                rect_scope(&rect, iter->span.x+iter->span.width-1, iter->span.y);
            }

            for (iter = outlinespans; iter != NULL; iter = iter->next)
            {
                rect_scope(&rect, iter->span.x, iter->span.y);
                rect_scope(&rect, iter->span.x+iter->span.width-1, iter->span.y);
            }
            int pwidth = rect.xmax - rect.xmin + 1;
            int pheight = rect.ymax - rect.ymin + 1;
            int w = 0;
            pixel = (unsigned char *)malloc(pwidth * pheight * sizeof(unsigned char));
            if (pixel == NULL)
            {
                printf("Mem alloc error. Strings may be too long.\n");
                return NULL;
            }
            memset(pixel, 0, pwidth * pheight *sizeof(unsigned char));

            for (iter = outlinespans; iter != NULL; iter = iter->next)
            {
                for (w = 0; w < (iter->span.width); w++)
                {
                    pixel[(int)((pheight-1-(iter->span.y-rect.ymin)) * pwidth + \
                       iter->span.x-rect.xmin+w)] = fhandle->font.outline_width;
                }
            }
            for (iter = spans; iter != NULL; iter = iter->next)
            {
                for (w = 0; w < (iter->span.width); w++)
                {
                    pixel[(int)((pheight-1-(iter->span.y-rect.ymin)) * pwidth + \
                       iter->span.x-rect.xmin+w)] = 0xff;
                }

            }

            *width = pwidth;
            *height = pheight;
        }
    }
    free_spans(outlinespans);
    free_spans(spans);

    return pixel;
}


