/* This is standalone and raw logger for the A2560X/K/GenX channel A
 * It is meant to be standalone and pretty raw so to minimize chances of failing, as it is used for debugging. */

#include "sys_general.h"

#if MODEL == MODEL_FOENIX_A2560K || MODEL == MODEL_FOENIX_A2560X || MODEL == MODEL_FOENIX_GENX

#include <stdint.h>
#include <stdio.h> /* debug */
#include <string.h>

#include "interrupt.h"

/*#include "rsrc/font/BM437_IBM_Model3_Alt4.h"*/
#include "rsrc/font/foenix_st_8_8.h"

#define COL_COUNT (800/8)
#define LINE_COUNT (600/8)

#define VICKY3_A_BASE      ((uint8_t*)0xfec40000L)
#define CURSOR_POSITION    (*((uint32_t*)0xfec40014))
#define TEXT_MEMORY        ((uint8_t*)0xfec60000)
#define TEXT_COLOR_MEMORY  ((uint8_t*)0xfec68000)
#define FONT_MEMORY        ((uint8_t*)0xfec48000)
#define LUT_FG             ((uint32_t*)0xfec6c400)
#define LUT_BG             ((uint32_t*)0xfec6c440)

static void scroll();
void channel_A_cls(void);
void channel_A_write(const char *c);

char buffer[110]; /* this is in the BSS so not a stack space problem */

void do_nothing() {
    /**((uint32_t*)0xfec40008) = 0x00770077; */ /* Purple color indicates an exception */
    for(;;);
}

void channel_A_logger_init(void)
{
    /* Setup the channel */
    *((uint32_t*)0xfec40000) = 1; /* Text enable, enable video, 800x600, no gamma */
    *((uint32_t*)0xfec40004) = 0x00080801; /* Border 8 pixels enabled */
    *((uint32_t*)0xfec40008) = 0x00770000; /* Border color RGB */
    *((uint32_t*)0xfec4000c) = 0x00222222; /* Background color */
    *((uint32_t*)0xfec40010) = 0x10640001; /* Cursor (pas sûr du X offset) ! */
    CURSOR_POSITION = 0x00000000; /* Cursor position YYYYXXXX */
    *((uint32_t*)0xfec40020) = 0x08080808; /* Font manager 0 */
    *((uint32_t*)0xfec40024) = (LINE_COUNT & 0xff) << 8 | (COL_COUNT & 0xff);

    /* Load the font */
    uint8_t* font_memory = FONT_MEMORY;
    uint8_t* font = (uint8_t*)foenix_st_8x8;
    for (int i=0; i<0x1000; i++)
        font_memory[i] = font[i];

    /* Setup colors */
    uint32_t *lut_fg = LUT_FG;
    uint32_t *lut_bg = LUT_BG;
    lut_fg[0] = lut_bg[0] = 0x00444444;
    lut_fg[1] = lut_bg[1] = 0x0000ff00;

    channel_A_cls();
    channel_A_write("Bonjour le monde voila plein de joli texte !\n");
    channel_A_write("Et une autre ligne\n");
    channel_A_write("Ici on test que ca wrappe correctmentABCDEFGHIJKLMNOPQRSTWXYZABCDEFGHIJKLMNOPQRSTWXYZABCDEFGHIJKLMNOPQRSTWXYZABCDEFGHIJKLMNOPQRSTWXYZABCDEFGHIJKLMNOPQRSTWXYZ\n");

    /* If there's one of these exceptions, we'll change the border color to purple */
    *((uint32_t*)0x08) = (uint32_t)do_nothing; /* Bus error */
    *((uint32_t*)0x0c) = (uint32_t)do_nothing; /* Address */
    *((uint32_t*)0x10) = (uint32_t)do_nothing; /* Illegal instruction */
    *((uint32_t*)0x20) = (uint32_t)do_nothing;

    channel_A_write("Hello world encore du baratin\n");

        for (short a=0;a<80;a++)
        {
            for (char x=0; x<=a; x++)
            {
                char *c = &buffer[x];
                *c++ = 'A' + (x&0xf);
                *c++ = '\n';
                *c++ = '\0';
            }
            channel_A_write(buffer);
        }
    
    *((uint32_t*)0xfec40008) = 0x00FFFFFF; /* White border indicates we got here */
}

void channel_A_cls(void)
{
    /* Clear screen */
    uint8_t *text = TEXT_MEMORY;
    uint8_t *color = TEXT_COLOR_MEMORY;
    for (int i=0; i<0x4000; i++)
    {
        *text++ = 0;
        *color++ = 0x10;
    }
}

void channel_A_write(const char *c)
{
    uint32_t x,y;

    x = y = CURSOR_POSITION;
    x &= 0xffff;
    y = (y >> 16);
    int16_t y_offset;

    y_offset = COL_COUNT * y;

    while (c && *c)
    {
        if (*c == '\r')
        {
            /* Carriage return */
            x = 0;
        }
        else if (*c == '\n')
        {
            /* Line feed */
            if (y == LINE_COUNT-1)
                scroll();
            else
            {
                y++;
                y_offset += COL_COUNT;
            }

            x = 0; /* Assume that \n = \r\n */
        }
        else
        {
            TEXT_MEMORY[y_offset + x] = *c;
            TEXT_COLOR_MEMORY[y_offset + x] = 0x10;

            if (++x == COL_COUNT) {
                /* Wrap */
                if (y == LINE_COUNT-1)
                    scroll();
                else {
                    y++;
                    y_offset += COL_COUNT;
                }

                x = 0; /* Assume that \n = \r\n */
            }
        }

        c++;
    }

    CURSOR_POSITION = (y << 16) | x;
}


static void scroll()
{
#define COPY_BYTE_BY_BYTE
#ifdef COPY_BYTE_BY_BYTE
    char *src = &TEXT_MEMORY[1*COL_COUNT];
    char *dst = &TEXT_MEMORY[0];
    for (int i=0 ; i<((LINE_COUNT-1)*COL_COUNT)/4; i++)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
    }
    for (int i=0; i < COL_COUNT/4; i++)
    {
        *dst++ = ' ';
        *dst++ = ' ';
        *dst++ = ' ';
        *dst++ = ' ';
    }
#else
    /* This doesn't work because it uses 16/32 byte writes */
    memcpy(&TEXT_MEMORY[COL_COUNT], &TEXT_MEMORY[0], COL_COUNT*LINE_COUNT-1);
    memset(&TEXT_MEMORY[COL_COUNT*LINE_COUNT-1], COL_COUNT, 0); /* VBCC doesn't know bzero */
#endif    
    /* If we used background colours we would have to scroll them as well, but we don't. */
}

#endif