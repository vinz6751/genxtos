/* Shadow frame buffer support.
 * On the Foenix it's not possible to read the video memory directly.
 * This can make it difficult to do effects like XOR, NOT etc on pixels.
 * The so-called Shadow Frame Buffer aims at working around this.
 * It works like this: in system RAM we have a memory which acts as video
 * memory. On VBL (a.k.a SOF), we copy that to Video RAM.
 */

#include <stdint.h>
#include "a2560u_debug.h"
#include "shadow_fb.h"
#include "vicky2.h"

/* To speed up the copy from RAM to VRAM we manage a list of dirty cells */
#define A2560U_DIRTY_CELLS_SIZE 256
volatile struct
{
    uint16_t writer;
    uint16_t reader;
    uint16_t full_copy; /* Set this to non-zero to copy the whole frame buffer and flush the ring buffer */
    uint8_t *cells[A2560U_DIRTY_CELLS_SIZE];
} a2560u_sfb_dirty_cells;


/* Size and address of the shadow frame buffer */
uint8_t  *a2560u_sfb_addr;
uint32_t a2560u_sfb_size;
uint16_t a2560u_sfb_line_size_in_bytes;
uint16_t a2560u_sfb_text_cell_height;


void a2560u_sfb_init(void)
{
    a2560u_sfb_dirty_cells.writer = a2560u_sfb_dirty_cells.reader = a2560u_sfb_dirty_cells.full_copy = 0;
}


void a2560u_sfb_setup(const uint8_t *addr, uint16_t text_cell_height)
{
    FOENIX_VIDEO_MODE mode;

    // a2560u_debug("a2560u_sfb_setup(%p,%d)", addr, text_cell_height);
    a2560u_sfb_addr = (uint8_t *)addr;
    a2560u_sfb_text_cell_height = text_cell_height;
    
    vicky2_read_video_mode(&mode);
    a2560u_sfb_size = mode.w * mode.h;
    a2560u_sfb_line_size_in_bytes = mode.w;
}


void a2560u_sfb_mark_screen_dirty(void)
{
    a2560u_sfb_dirty_cells.full_copy = -1;
}
