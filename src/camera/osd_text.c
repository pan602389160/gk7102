/******************************************************************************
** \file        adi/test/src/osd_test.c
**
** \brief       ADI layer osd test.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ft2build.h>
#include "osd_text.h"
#include "vector_font.h"
#include "lattice_font.h"

#include FT_FREETYPE_H
#include FT_TRIGONOMETRY_H
#include FT_BITMAP_H
#include FT_STROKER_H


//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** Local structures
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Local Data
//*****************************************************************************
//*****************************************************************************
static font_lib_handle lib_handle = NULL;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void fill_area_map_data(const void *buffer, LATTICE_INFO *lattice_info,
		void *hook_data);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
/* Return 0 if initializing the freetype library successfully, -1 otherwise.*/
int osd_text_lib_init(FONT_LIBRARY_TYPE flib_type)
{
	int ret = 0;
	if (flib_type < 0 || flib_type >= MAX_FONT_LIB){
		printf("Not the font library\n");
		return -1;
	}
    if (lib_handle != NULL)
    {
        printf("osd_text_lib_init has been called.\n");
        return 0;
    }
	switch(flib_type) {
		case VECTOR_FONT_LIB:
		    lib_handle = create_vector_font_library();
			if(lib_handle == NULL)
				return -1;
			break;
		case LATTICE_FONT_LIB:
			lib_handle = create_lattice_font_library();
			if(lib_handle == NULL)
				return -1;
			break;
		default:
			printf("Not the font library\n");
			break;
	}
	//set hook function, to fill area map.
	GET_FUNC(lib_handle)->set_hook(lib_handle, fill_area_map_data);
    return ret;
}

int osd_text_lib_exit(void)
{
    if (lib_handle == NULL)
    {
        printf("osd_text_lib_exit hasn't been initilized.");
        return 0;
    }
	GET_FUNC(lib_handle)->destory(lib_handle);
    lib_handle = NULL;
    return 0;
}

int osd_text_set_font_attribute(const font_attribute_t *font_attr)
{
	if(font_attr == NULL || lib_handle == NULL) {
		printf("set font attribute param is error or no font library.");
		return -1;
	}
	return (GET_FUNC(lib_handle)->set_font_attribute(lib_handle, font_attr));
}

int osd_text_get_font_attribute(font_attribute_t * font_attr)
{
    if(font_attr == NULL || lib_handle == NULL) {
		printf("set font attribute param is error or no font library.");
		return -1;
	}
	return (GET_FUNC(lib_handle)->get_font_attribute(lib_handle, font_attr));
}

/*
 * Return 0 if font bitmap buffer is filled successfully, -1 otherwise.
 *
 * bmp_addr is the buffer bitmap data is to write.
 * buf_height and buf_pitch for bitmap alignment with the buffer.
 * offset_x is to set  horizontal offset of the character bitmap to the buffer.
 * The bitmap info including height and width will be given out in
 * bitmap_info_t *bmp_info.
 */


int osd_text_convert_character(const char *char_code,
                                  LATTICE_INFO *lattice_info, void *hook_data)
{
	return (GET_FUNC(lib_handle)->char_to_bitmap(lib_handle, char_code,
				lattice_info, hook_data));
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void fill_area_map_per_bit(const unsigned char *buffer, LATTICE_INFO *lattice_info,
		void *hook_data)
{
	unsigned char row, col;
	int dst_index, src_index;
	RGBQUAD *ptr = NULL;
	MAP_INFO *map_info = (MAP_INFO*)hook_data;
	for (row = 0; row < lattice_info->height; row++)
	{
	    dst_index = map_info->width * (row + map_info->starty + lattice_info->y)
			//add offset base and offset
			+ map_info->startx + lattice_info->x;
	    src_index = (row * lattice_info->width) >> 3;
	    for (col = 0; (col < lattice_info->width) && (col + map_info->startx < map_info->width);
	            col++, dst_index++)
	    {
			unsigned int index_offset = col/8;
	        if (dst_index < 0)
	        {
	            continue;
	        }
	        if ((buffer[src_index + index_offset]) == 0)
	        {
	            continue;
	        }
	        ptr = map_info->mapaddr + dst_index;
	        if((buffer[src_index + index_offset] & (0x80 >> (col & 0x07))) > 0)
				*ptr = map_info->color.pixel_font;
	    }
	}
}
static void fill_area_map_per_char(const unsigned char *buffer, LATTICE_INFO *lattice_info,
		void *hook_data)
{
	int row, col;
	int dst_index, src_index;
	RGBQUAD *ptr = NULL;
	MAP_INFO *map_info = (MAP_INFO*)hook_data;
	for (row = 0; row < lattice_info->height; row++)
	{
	    dst_index = map_info->width * (row + map_info->starty + lattice_info->y)
			//add offset base and offset
			+ map_info->startx + lattice_info->x;
	    src_index = row * lattice_info->width;
	    for (col = 0; (col < lattice_info->width) && (col + map_info->startx < map_info->width);
	            col++, dst_index++, src_index++)
	    {
	        if (dst_index < 0)
	        {
	            continue;
	        }
	        if ((buffer[src_index]) == 0)
	        {
	            continue;
	        }
	        ptr = map_info->mapaddr + dst_index;
            if(buffer[src_index] == 0xff) {
                *ptr = map_info->color.pixel_font;
			} else {
                *ptr = map_info->color.pixel_outline;
				ptr->a = buffer[src_index];//use auto generation color.
			}
	    }
	}
}

static void fill_area_map_data(const void *buffer, LATTICE_INFO *lattice_info,
		void *hook_data)
{
	if(buffer == NULL
		|| lattice_info == NULL
		|| hook_data == NULL )
		return;
	switch(lattice_info->type) {
		case LATTICE_BASE_BIT:
			fill_area_map_per_bit(buffer, lattice_info, hook_data);
			break;
		case LATTICE_BASE_CHAR:
			fill_area_map_per_char(buffer, lattice_info, hook_data);
			break;
		case LATTICE_BASE_INT:
			break;
		default:
			break;
	}
}

