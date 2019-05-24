#include "osd_common.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ft2build.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
typedef struct {
	font_lib_func		func;
	font_attribute_t 	font;
	save_lattice_hook_f	save_data;
	int 				flag_attr;
	unsigned char		*asc_lib;
	unsigned char		*gb2312_lib;
} LATTICE_FONT_LibraryT, *LATTICE_FONT_Handle;
static int load_font_to_mem(const char* file_name, unsigned char **ret_mem);
static save_lattice_hook_f	set_save_lattice_hook(font_lib_handle lib_handle,
	save_lattice_hook_f hook);
static int get_lattice_font_attribute(font_lib_handle lib_handle,
	font_attribute_t *font_attr);
static int set_lattice_font_attribute(font_lib_handle lib_handle,
	const font_attribute_t *font_attr);
static int char_to_bitmap_by_lattice_library(font_lib_handle lib_handle,
	const char *char_code, LATTICE_INFO *lattice_info, void *hook_data);
static int destory_lattice_font_library(font_lib_handle lib_handle);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

font_lib_handle create_lattice_font_library(void)
{
	LATTICE_FONT_Handle fhandle = NULL;

	fhandle = malloc(sizeof(LATTICE_FONT_LibraryT));
	if(fhandle == NULL) {
		printf("RAM space is no enough[%04x].", sizeof(LATTICE_FONT_LibraryT));
		return NULL;
	}
	memset(fhandle, 0, sizeof(LATTICE_FONT_LibraryT));
	fhandle->func.set_hook = set_save_lattice_hook;
	fhandle->func.set_font_attribute = set_lattice_font_attribute;
	fhandle->func.get_font_attribute = get_lattice_font_attribute;
	fhandle->func.char_to_bitmap = char_to_bitmap_by_lattice_library;
	fhandle->func.destory = destory_lattice_font_library;

	return (font_lib_handle)fhandle;
}



//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************

static int destory_lattice_font_library(font_lib_handle lib_handle)
{
	LATTICE_FONT_Handle fhandle = (LATTICE_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
	if(fhandle->asc_lib){
		free(fhandle->asc_lib);
		fhandle->asc_lib = NULL;
	}
	if(fhandle->gb2312_lib) {
		free(fhandle->gb2312_lib);
		fhandle->gb2312_lib = NULL;
	}
	free(fhandle);
	return 0;
}

static save_lattice_hook_f	set_save_lattice_hook(font_lib_handle lib_handle,
	save_lattice_hook_f hook)
{
	save_lattice_hook_f tmp = NULL;
	LATTICE_FONT_Handle fhandle = (LATTICE_FONT_Handle)lib_handle;
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

static int set_lattice_font_attribute(font_lib_handle lib_handle,
	const font_attribute_t *font_attr)
{
	LATTICE_FONT_Handle fhandle = (LATTICE_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
    if (font_attr == NULL)
    {
        printf("osd_text_set_font_attribute: input bad parameters !");
        return -1;
    }
    if (fhandle->flag_attr == 1)
    {
        fhandle->flag_attr = 0;
    }
	if(fhandle->asc_lib){
		free(fhandle->asc_lib);
		fhandle->asc_lib = NULL;
	}
	if(fhandle->gb2312_lib) {
		free(fhandle->gb2312_lib);
		fhandle->gb2312_lib = NULL;
	}
	load_font_to_mem(font_attr->hlaf_path, &(fhandle->asc_lib));
	load_font_to_mem(font_attr->all_path, &(fhandle->gb2312_lib));
    strncpy(fhandle->font.hlaf_path, font_attr->hlaf_path, sizeof(font_attr->hlaf_path));
    strncpy(fhandle->font.all_path, font_attr->all_path, sizeof(font_attr->all_path));
    fhandle->font.size = font_attr->size;
    fhandle->font.outline_width = font_attr->outline_width;
    fhandle->font.hori_bold = font_attr->hori_bold;
    fhandle->font.vert_bold = font_attr->vert_bold;
    fhandle->font.italic = font_attr->italic;
    fhandle->font.disable_anti_alias = font_attr->disable_anti_alias;
    fhandle->flag_attr = 1;

	return 0;
}

static int get_lattice_font_attribute(font_lib_handle lib_handle,
	font_attribute_t *font_attr)
{
	LATTICE_FONT_Handle fhandle = (LATTICE_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
	if (font_attr == NULL)
    {
        printf("osd_text_get_font_attribute: input bad parameters!");
        return -1;
    }
    if (fhandle->flag_attr == 0)
    {
        printf("None font attribute is set.");
        return 0;
    }
    strncpy(font_attr->hlaf_path, fhandle->font.hlaf_path, sizeof(font_attr->hlaf_path));
    strncpy(font_attr->all_path, fhandle->font.all_path, sizeof(font_attr->all_path));
    font_attr->size = fhandle->font.size;
    font_attr->outline_width = fhandle->font.outline_width;
    font_attr->hori_bold = fhandle->font.hori_bold;
    font_attr->vert_bold = fhandle->font.vert_bold;
    font_attr->italic = fhandle->font.italic;
    font_attr->disable_anti_alias = fhandle->font.disable_anti_alias;
    return 0;
}

static int char_to_bitmap_by_lattice_library(font_lib_handle lib_handle,
	const char *char_code, LATTICE_INFO *lattice_info, void *hook_data)
{
	const char *ch = char_code;
	void *buffer = NULL;
	int asc_width, gb2312_width;
	int used_nbyte = 1;//default use 1byte

	LATTICE_FONT_Handle fhandle = (LATTICE_FONT_Handle)lib_handle;
	if(fhandle == NULL) {
		printf("No have font library.");
		return -1;
	}
	if(ch == NULL) {
		printf("No have char code.");
		return -1;
	}
	gb2312_width = fhandle->font.size;
	asc_width = fhandle->font.size >> 1;
	if((*ch < 0x7f) && (fhandle->asc_lib != NULL))
	{
		// ascii
		int const stride_bytes = (asc_width + 7) / 8;
		int const asc_code = *ch;
		off_t const offset_byte = asc_code * fhandle->font.size * stride_bytes;
		buffer = &(fhandle->asc_lib[offset_byte]);
		lattice_info->width  = asc_width;
		lattice_info->height = fhandle->font.size;
		used_nbyte = 1;/*use 1 byte*/
	}
	else if((*ch > 0xa0) && (fhandle->gb2312_lib != NULL))
	{
		int const stride_bytes = (gb2312_width + 7) / 8;
		// get qu code and wei code
		int const qu_code = ch[0] - 0xa0 - 1; // 87
		int const wei_code = ch[1] - 0xa0 - 1; // 94

		if(6 == qu_code){
			// russian
		}
		else
		{
			off_t const offset_byte = (qu_code * 94 + wei_code) * fhandle->font.size * stride_bytes;
			buffer = &(fhandle->gb2312_lib[offset_byte]);
		}
		lattice_info->width  = gb2312_width;
		lattice_info->height = fhandle->font.size;
		used_nbyte = 2;/*used 2 byte*/
	}

	lattice_info->x    = 0;
	lattice_info->y    = 0;
	lattice_info->type = LATTICE_BASE_BIT;
	if(fhandle->save_data){
		fhandle->save_data(buffer, lattice_info, hook_data);
	}
	return used_nbyte;

}

static int load_font_to_mem(const char* file_name, unsigned char **ret_mem)
{
	struct stat file_stat={0};
	FILE* fid = NULL;
	char font=0;
	if(0 == stat(file_name, &file_stat)){
		fid = fopen(file_name, "rb");
		if(NULL != fid){
			if(NULL != *ret_mem){
				free(*ret_mem);
			}
			if(strstr(file_name, "hzk")!=NULL)
			{
				fseek(fid,0,SEEK_SET);
				fread(&font, 1, 1, fid);
				if(font=='M'){
					fseek(fid,16,SEEK_SET);
					file_stat.st_size -= 16;
				} else {
					fseek(fid,0,SEEK_SET);
				}
			}
			*ret_mem = calloc(file_stat.st_size, 1);
			fread(*ret_mem, 1, file_stat.st_size, fid);
			fclose(fid);
			fid = NULL;
			return 0;
		}
	}
	return -1;
}

