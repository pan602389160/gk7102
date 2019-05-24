/******************************************************************************
** \file        adi/test/src/fb.c
**
**
** \brief       Framebuffer test.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <getopt.h>
#include <sys/ioctl.h>

#include "shell.h"
#include <linux/fb.h>
#include "adi_types.h"
#include "adi_sys.h"
#include "adi_vout.h"

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
struct fb_cmap_user {
    unsigned int start;            /* First entry    */
    unsigned int len;            /* Number of entries */
    unsigned short *red;        /* Red values    */
    unsigned short *green;
    unsigned short *blue;
    unsigned short *transp;        /* transparency, can be NULL */
};



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
static int fbHandle = -1;
static unsigned short  * fbBuffer = NULL;
static struct fb_fix_screeninfo fixInfo;
static struct fb_var_screeninfo varInfo;

static const char shortOptions[] = "hocfs";
static struct option longOptions[] =
{
    {"help",     0, 0, 'h'},
    {"open",     0, 0, 'o'},
    {"select",   0, 0, 's'},
    {"close",    0, 0, 'c'},
    {"fill",     0, 0, 'f'},
    {0,          0, 0, 0}
};
static int voutchannel = GADI_VOUT_B;

extern GADI_SYS_HandleT voHandle;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************
static void usage(void);
static GADI_ERR handle_fb_command(int argc, char* argv[]);

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//****************************************************************************
int fb_fill(void);
int fb_select(void);
int fb_open(void)
{
    if (fbHandle >= 0) {
        GADI_ERROR("Frame buffer already init.\n");
        return -1;
    }

    fb_select();

    fbHandle = open ("/dev/fb0", O_RDWR);
    if (fbHandle < 0)
    {
        perror("open /dev/fb0 error:\n");
        return -1;
    }

    if(ioctl(fbHandle, FBIOGET_FSCREENINFO, &fixInfo) < 0)
    {
        printf("Cannot get fixed screen info\n.");
        close (fbHandle);
        fbHandle = -1;
        return -1;
    }

    varInfo.xres = 720;
    varInfo.yres = 480;
    varInfo.bits_per_pixel = 16;
    if(ioctl(fbHandle, FBIOGET_VSCREENINFO, &varInfo) < 0)
    {
        GADI_ERROR("Cannot get var screen info\n.");
        close (fbHandle);
        fbHandle = -1;
        return -1;
    }
    GADI_INFO("framebuffer: %d x %d @ %d -- %dbpp\n", varInfo.xres, varInfo.yres,
                fixInfo.line_length, varInfo.bits_per_pixel);

    fbBuffer = (unsigned short *)mmap(NULL, fixInfo.smem_len,
                                     PROT_WRITE,  MAP_SHARED, fbHandle, 0);

    GADI_INFO("framebuffer addr:0x%08x, len=%d\n", (int)fbBuffer, fixInfo.smem_len);
    if (fbBuffer == MAP_FAILED)
    {
        GADI_ERROR("Cannot mmap framebuffer memory.");
        close (fbHandle);
        fbHandle = -1;
        return -1;
    }

    return 0;
}

int fb_close(void)
{
    if (fbHandle < 0) {
        GADI_ERROR("Frame buffer already closed.\n");
        return -1;
    }

    munmap(fbBuffer, fixInfo.smem_len);
    fbBuffer = NULL;

    close(fbHandle);
    fbHandle = -1;
    GADI_INFO("Close frame buffer.\n");

    return 0;
}

int fb_select(void)
{
    GADI_VOUT_SelectFbParamsT fbPar;

    fbPar.voutChannel = voutchannel;
    fbPar.fbChannel   = 0;
    return gadi_vout_select_fb(voHandle, &fbPar);
}

int fb_fill(void)
{
    int x, y;
    unsigned short *baseAddr = fbBuffer;

    /*fill rec.*/
    /*ARGB444.*/
    for(y=0; y < varInfo.yres/2; y++)
    {
        for(x=0; x < varInfo.xres/2; x++)
        {
            /*fill BLUE only.*/
            *(baseAddr + x + y*(fixInfo.line_length/2)) = 0xf00f;
        }
        for(; x < varInfo.xres; x++)
        {
            /*fill GREEN only.*/
            *(baseAddr + x + y*(fixInfo.line_length/2)) = 0xf0f0;
        }
    }
    for(; y < varInfo.yres; y++)
    {
        for(x=0; x < varInfo.xres/2; x++)
        {
            /*fill GREEN only.*/
            *(baseAddr + x + y*(fixInfo.line_length/2)) = 0x80f0;
        }
        for(; x < varInfo.xres; x++)
        {
            /*fill RED only.*/
            *(baseAddr + x + y*(fixInfo.line_length/2)) = 0xff00;
        }
    }

#if 1
    varInfo.yoffset= 0;
    if(ioctl(fbHandle, FBIOPAN_DISPLAY, &varInfo) < 0)
    {
        printf("Cannot display Pan\n.");
        return -1;
    }
#endif

    return 0;
}

int fb_register_testcase(void)
{
    int   retVal = 0;
    (void)shell_registercommand (
        "fb",
        handle_fb_command,
        "fb command",
        "---------------------------------------------------------------------\n"
        "fb -o\n"
        "   brief : open framebuffer device.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "fb -c\n"
        "   brief : close framebuffer device.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "fb -s\n"
        "   brief : select output channel.\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "fb -f\n"
        "   brief : fill RGB on vout screen.\n"
        "\n"
    );

    return retVal;
}

//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************
static void usage(void)
{
    printf("\nusage: osd [OPTION]...[Text String]...\n");
    printf("\t-h, --help            help\n"
           "\t-o, --open            open framebuffer.\n"
           "\t-s, --select          select vout channel.\n"
           "\t-c, --close           close framebuffer.\n"
           "\t-f, --fill            fill framebuffer memory.\n");

    printf("\n");
}

static GADI_ERR handle_fb_command(int argc, char* argv[])
{
    int option_index, ch;

    /*change parameters when giving input options.*/
    while ((ch = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1)
    {
        switch (ch)
        {
            case 'h':
                usage();
                goto command_exit;

            case 'o':
                fb_open();
                break;

            case 'c':
                fb_close();
                break;

            case 's':
                CREATE_INPUT_MENU(VoutChanel) {
                    ADD_SUBMENU(voutchannel,
                        "Select Vout channel [0:VoutA 1:VoutB]."),
                    CREATE_INPUT_MENU_COMPLETE();
                    if (DISPLAY_MENU() == 0) {
                        if(fb_select() != GADI_OK)
                            GADI_ERROR("gadi_isp_set_wb_type fail\n");
                        else
                            GADI_INFO("gadi_isp_set_wb_type ok\n");
                    }
                }
                break;

            case 'f':
                fb_fill();
                break;

            default:
                printf("type '--help' for more usage.\n");
                goto command_exit;
        }
    }

command_exit:
    optind = 1;
    return 0;
}

