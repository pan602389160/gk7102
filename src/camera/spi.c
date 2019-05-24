/*!
*****************************************************************************
** \file        /src/spi.c
**
** \brief        ADI layer SPI test
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**               ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**               OMMISSIONS
**
** (C) Copyright 2012-2013 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/epoll.h>

#include "adi_types.h"
#include "shell.h"
#include "adi_sys.h"
#include "adi_spi.h"
#include "spi.h"

//*****************************************************************************
//*****************************************************************************
//** Local Defines
//*****************************************************************************
//*****************************************************************************

#define ADI_TEST_DEBUG_LEVEL    GADI_SYS_LOG_LEVEL_INFO


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
static GADI_SYS_HandleT spiHandle;

//*****************************************************************************
//*****************************************************************************
//** Local Functions Declaration
//*****************************************************************************
//*****************************************************************************

static GADI_ERR spi_p_handle_cmd(int argc, char *argv[]);


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
GADI_ERR spi_register_testcase(void)
{

    GADI_ERR   retVal =  GADI_OK;
    (void)shell_registercommand (
        "spi",
        spi_p_handle_cmd,
        "spi command",
        "---------------------------------------------------------------------\n"
        "spi open [nodePATH] [mode] [bits] [speed] [lsb]\n"
        "   brief : export I/O port and set parameter.\n"
        "   param : nodePATH    -- device node path\n"
        "   param : mode        -- 0 low level at idle state, collect data at the first edge of clock\n"
        "                       -- 1 low level at idle state, collect data at the second edge of clock\n"
        "                       -- 2 high level at idle state, collect data at the first edge of clock\n"
        "                       -- 3 high level at idle state, collect data at the second edge of clock\n"
        "   param : bits        -- 4-16 temporary override of the device's wordsize\n"
        "   param : speed       -- 10000-10000000 temporary override of the device's bitrate\n"
        "   param : lsb         -- 1 transmit least significant bit first\n"
        "                       -- 0 transmit most significant bit first\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "spi write [data] [data length]\n"
        "   brief : output data only.\n"
        "   param : data        -- 0-4 bytes, HEX, write last byte first\n"
        "   param : data length -- length of data\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "spi read  [data] [data length]\n"
        "   brief : collect data only.\n"
        "   param : data        -- character string\n"
        "   param : data length -- length of data\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "spi WAR [data] [data length]\n"
        "   brief : write and collect data at the same time.\n"
        "   param : data        -- 0-4 bytes, HEX, write last byte first\n"
        "   param : data length -- data written and read length\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "spi WTR [data string] [write data length] [read data length]\n"
        "   brief : write data and then collect data.\n"
        "   param : data string       -- 0-4 bytes, HEX, write last byte first\n"
        "   param : write data length -- character string written length\n"
        "   param : read data length  -- character string read length\n"
        "\n"
        "---------------------------------------------------------------------\n"
        "spi close \n"
        "   brief : close SPI port.\n"
        "\n"
    );

    return retVal;

}

static GADI_ERR spi_p_handle_cmd(int argc, char *argv[])
{

    GADI_ERR errorCode = GADI_OK;
    GADI_U8 i;
    if(argc == 7)
    {
        if(strcmp(argv[1], "open") == 0)
        {
            GADI_SPI_OpenParam config;
            config.nodePath = argv[2];
            config.mode     = (GADI_U8)(atoi(argv[3]));
            config.bits     = (GADI_U8)(atoi(argv[4]));
            config.speed    = (GADI_U32)(atoi(argv[5]));
            config.lsb      = (GADI_U8)(atoi(argv[6]));
            spiHandle = gadi_spi_open(&errorCode, &config);
            if (errorCode != GADI_OK)
            {
                printf("gadi_spi_open() failed.\n");
                return errorCode;
            }
        }
        else
        {
            goto bad_parameter;
        }

    }
    else if(argc == 5)
    {
        if(strcmp(argv[1], "WTR") == 0)
        {
            GADI_U8 *dataRD = (GADI_U8 *)malloc(atoi(argv[6]));
            memset(dataRD, 0, atoi(argv[4]));
            GADI_U32 dataWR = strtoul(argv[2], 0, 16);
            errorCode = gadi_spi_write_then_read(spiHandle, &dataWR, atoi(argv[3]), dataRD, atoi(argv[4]));
            if (errorCode != GADI_OK)
            {
                free(dataRD);
                dataRD = NULL;
                printf("gadi_spi_write_then_read() failed.\n");
                return errorCode;
            }
            else
            {
                for(i = 0; i < atoi(argv[4]); i++)
                {
                    GADI_INFO("read data: 0x%.2X\n", dataRD[i]);
                    free(dataRD);
                    dataRD = NULL;
                }
                if(dataRD != NULL)
                {
                    free(dataRD);
                    dataRD = NULL;
                }
            }
        }
        else
        {
            goto bad_parameter;
        }
    }
    else if(argc == 4)
    {
        if(strcmp(argv[1], "WAR") == 0)
        {
            GADI_U8 *dataRD = (GADI_U8 *)malloc(atoi(argv[3]));
            memset(dataRD, 0, atoi(argv[3]));
            GADI_U32 dataWR = strtoul(argv[2], 0, 16);
            errorCode = gadi_spi_write_and_read(spiHandle, &dataWR, dataRD, atoi(argv[3]));
            if (errorCode != GADI_OK)
            {
                free(dataRD);
                dataRD = NULL;
                printf("gadi_spi_write_and_read() failed.\n");
                return errorCode;
            }
            else
            {
                for(i = 0; i < atoi(argv[3]); i++)
                {
                    GADI_INFO("read data: 0x%.2X\n", dataRD[i]);
                }
                if(dataRD != NULL)
                {
                    free(dataRD);
                    dataRD = NULL;
                }
            }
        }
        else if(strcmp(argv[1], "write") == 0)
        {
            GADI_U32 dataWR = strtoul(argv[2], 0, 16);
            errorCode = gadi_spi_write_only(spiHandle, &dataWR, atoi(argv[3]));
            if (errorCode != GADI_OK)
            {
                printf("gadi_spi_write_only() failed.\n");
                return errorCode;
            }
        }
        else if(strcmp(argv[1], "read") == 0)
        {
            GADI_U8 *dataRD = (GADI_U8 *)malloc(atoi(argv[3]));
            memset(dataRD, 0, atoi(argv[3]));
            errorCode = gadi_spi_read_only(spiHandle, dataRD, atoi(argv[3]));
            if (errorCode != GADI_OK)
            {
                free(dataRD);
                dataRD = NULL;
                printf("gadi_spi_read_only() failed.\n");
                return errorCode;
            }
            else
            {
                for(i = 0; i < atoi(argv[3]); i++)
                {
                    GADI_INFO("read data: 0x%.2X\n", dataRD[i]);
                }
                if(dataRD != NULL)
                {
                    free(dataRD);
                    dataRD = NULL;
                }
            }
        }
        else
        {
            goto bad_parameter;
        }
    }
    else if(argc == 2)
    {
        if(strcmp(argv[1], "close") == 0)
        {
            errorCode = gadi_spi_close(spiHandle);
            if (errorCode != GADI_OK)
            {
                printf("gadi_spi_close() failed.\n");
                return errorCode;
            }
            else
            {
                printf("spi closed.\n");
                spiHandle = NULL;
            }
        }
        else
        {
            goto bad_parameter;
        }
    }
    else
    {
         goto bad_parameter;
    }

    return 0;

bad_parameter:
    return -1;
}




//*****************************************************************************
//*****************************************************************************
//** Local Functions
//*****************************************************************************
//*****************************************************************************




