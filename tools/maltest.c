/*
 * maltest.c - GEMDOS heap test from AUTO folder (before AES/desktop).
 *
 * Freestanding Atari TOS PRG: no mint libc/CRT.  Custom _start + trap #1
 * GEMDOS helpers only.  Keeps OSM use tiny so AUTO Pexec does not exhaust
 * the memory-descriptor pool (FOLDR100 / "Out of internal memory").
 *
 * Reports TOS variables and exercises Mxalloc(-1), Malloc(-1), and a real
 * 896-byte allocation (desktop CSAVE size).  Writes bootdrv:\MALTEST.TXT.
 *
 * Build: tools/build-maltest.sh
 */

/* ---- TOS system variables ---- */
#define PHYSTOP (*(void * const *)0x42eL)
#define MEMBOT  (*(void * const *)0x432L)
#define MEMTOP  (*(void * const *)0x436L)
#define END_OS  (*(void * const *)0x4faL)

/* sizeof(desk CSAVE) from DESK-MS 19 */
#define TEST_SIZE 896L

/* GEMDOS opcodes */
#define GEMDOS_PTERM0   0x0000
#define GEMDOS_CRAWIN   0x0007
#define GEMDOS_CCONWS   0x0009
#define GEMDOS_DGETDRV  0x0019
#define GEMDOS_FCREATE  0x003c
#define GEMDOS_FCLOSE   0x003e
#define GEMDOS_FWRITE   0x0040
#define GEMDOS_MXALLOC  0x0044
#define GEMDOS_MALLOC   0x0048
#define GEMDOS_MFREE    0x0049
#define GEMDOS_PTERM    0x004c

typedef long            LONG;
typedef unsigned long   ULONG;
typedef short           WORD;
typedef unsigned short  UWORD;

static int out_fd = -1;

/* ---- trap #1 helpers (cdecl stack; clobber call-clobbered regs) ---- */

static LONG gemdos0(WORD op)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "addq.l #2,%%sp"
        : "=r" (ret)
        : "g" (op)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static LONG gemdos_w(WORD op, WORD a)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.w %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "addq.l #4,%%sp"
        : "=r" (ret)
        : "g" (op), "g" (a)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static LONG gemdos_l(WORD op, LONG a)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.l %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "addq.l #6,%%sp"
        : "=r" (ret)
        : "g" (op), "g" (a)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static LONG gemdos_lw(WORD op, LONG a, WORD b)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.w %3,-(%%sp)\n\t"
        "move.l %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "lea 8(%%sp),%%sp"
        : "=r" (ret)
        : "g" (op), "g" (a), "g" (b)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static LONG gemdos_llw(WORD op, LONG a, LONG b, WORD c)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.w %4,-(%%sp)\n\t"
        "move.l %3,-(%%sp)\n\t"
        "move.l %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "lea 12(%%sp),%%sp"
        : "=r" (ret)
        : "g" (op), "g" (a), "g" (b), "g" (c)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static void Cconws(const char *s)
{
    (void) gemdos_l(GEMDOS_CCONWS, (LONG) s);
}

static LONG Crawcin(void)
{
    return gemdos0(GEMDOS_CRAWIN);
}

static WORD Dgetdrv(void)
{
    return (WORD) gemdos0(GEMDOS_DGETDRV);
}

static WORD Fcreate(const char *name, WORD attr)
{
    return (WORD) gemdos_lw(GEMDOS_FCREATE, (LONG) name, attr);
}

static LONG Fwrite(WORD handle, LONG count, const void *buf)
{
    return gemdos_llw(GEMDOS_FWRITE, (LONG) count, (LONG) buf, handle);
}

static WORD Fclose(WORD handle)
{
    return (WORD) gemdos_w(GEMDOS_FCLOSE, handle);
}

static void *Malloc(LONG bytes)
{
    return (void *) gemdos_l(GEMDOS_MALLOC, bytes);
}

static void *Mxalloc(LONG bytes, WORD mode)
{
    return (void *) gemdos_lw(GEMDOS_MXALLOC, bytes, mode);
}

static WORD Mfree(void *addr)
{
    return (WORD) gemdos_l(GEMDOS_MFREE, (LONG) addr);
}

/* ---- tiny libc substitutes ---- */

static long slen(const char *s)
{
    const char *p = s;

    while (*p)
        p++;
    return p - s;
}

static void cputs(const char *s)
{
    Cconws(s);
    if (out_fd >= 0)
        (void) Fwrite((WORD) out_fd, slen(s), s);
}

static void u32hex(ULONG v)
{
    char buf[11];
    char *p = buf + 10;
    int i;

    *p = '\0';
    for (i = 0; i < 8; i++)
    {
        unsigned n = (unsigned) (v & 0x0fUL);

        *--p = (char) ((n < 10) ? ('0' + n) : ('A' + n - 10));
        v >>= 4;
    }
    cputs("0x");
    cputs(p);
}

static void u32dec(ULONG v)
{
    char buf[12];
    char *p = buf + 11;

    *p = '\0';
    do
    {
        *--p = (char) ('0' + (char) (v % 10UL));
        v /= 10UL;
    } while (v);

    cputs(p);
}

static void show_bytes(LONG bytes)
{
    if (bytes < 0L)
    {
        cputs(" error ");
        u32dec((ULONG) (-bytes));
        cputs("\r\n");
        return;
    }

    u32dec((ULONG) bytes);
    cputs(" bytes (");
    u32dec((ULONG) (bytes / 1024L));
    cputs(" KB)\r\n");
}

static void show_ptr(const char *label, void *p)
{
    cputs(label);
    u32hex((ULONG) p);
    if (!p)
        cputs(" FAIL");
    else
        cputs(" OK");
    cputs("\r\n");
}

static void show_free_query(const char *label, LONG (*query)(void))
{
    cputs(label);
    show_bytes(query());
}

static LONG query_mx_stram(void)    { return (LONG) Mxalloc(-1L, 0); }
static LONG query_mx_ttram(void)    { return (LONG) Mxalloc(-1L, 1); }
static LONG query_mx_prefst(void)   { return (LONG) Mxalloc(-1L, 2); }
static LONG query_mx_preftt(void)   { return (LONG) Mxalloc(-1L, 3); }
static LONG query_malloc(void)      { return (LONG) Malloc(-1L); }

static void show_tos_vars(void)
{
    cputs("phystop="); u32hex((ULONG) (ULONG) PHYSTOP);
    cputs(" membot="); u32hex((ULONG) (ULONG) MEMBOT);
    cputs(" memtop="); u32hex((ULONG) (ULONG) MEMTOP);
    cputs("\r\nend_os="); u32hex((ULONG) (ULONG) END_OS);
    cputs("\r\n");

    if (MEMBOT && MEMTOP && MEMTOP > MEMBOT)
    {
        cputs("memtop-membot (TPA span): ");
        show_bytes((LONG) ((char *) MEMTOP - (char *) MEMBOT));
    }
}

static void make_outpath(char *buf)
{
    int drv = Dgetdrv();

    buf[0] = (char) ('A' + drv);
    buf[1] = ':';
    buf[2] = '\\';
    buf[3] = 'M';
    buf[4] = 'A';
    buf[5] = 'L';
    buf[6] = 'T';
    buf[7] = 'E';
    buf[8] = 'S';
    buf[9] = 'T';
    buf[10] = '.';
    buf[11] = 'T';
    buf[12] = 'X';
    buf[13] = 'T';
    buf[14] = '\0';
}

int maltest_main(void)
{
    char outfile[16];
    void *p_mx_st, *p_mx_tt, *p_ml;
    LONG free_after;

    make_outpath(outfile);
    out_fd = (int) Fcreate(outfile, 0);
    if (out_fd < 0)
    {
        cputs("Warning: cannot create output file on boot drive\r\n");
    }
    else
    {
        cputs("Writing ");
        cputs(outfile);
        cputs("\r\n");
    }

    cputs("\r\n========================================\r\n");
    cputs(" MALTEST - GEMDOS heap at AUTO boot\r\n");
    cputs("========================================\r\n");

    cputs("--- TOS variables ---\r\n");
    show_tos_vars();

    cputs("--- free RAM queries ---\r\n");
    show_free_query("Mxalloc(-1, ST-RAM):      ", query_mx_stram);
    show_free_query("Mxalloc(-1, TT-RAM):      ", query_mx_ttram);
    show_free_query("Mxalloc(-1, pref ST-RAM): ", query_mx_prefst);
    show_free_query("Mxalloc(-1, pref TT-RAM): ", query_mx_preftt);
    show_free_query("Malloc(-1):               ", query_malloc);

    cputs("--- allocate ");
    u32dec((ULONG) TEST_SIZE);
    cputs(" bytes (CSAVE size) ---\r\n");

    p_mx_st = Mxalloc(TEST_SIZE, 0);
    show_ptr("Mxalloc(896, ST-RAM):     ", p_mx_st);

    p_mx_tt = Mxalloc(TEST_SIZE, 3);
    show_ptr("Mxalloc(896, pref TT-RAM):", p_mx_tt);

    p_ml = Malloc(TEST_SIZE);
    show_ptr("Malloc(896):            ", p_ml);

    if (p_mx_st)
        (void) Mfree(p_mx_st);
    if (p_mx_tt)
        (void) Mfree(p_mx_tt);
    if (p_ml)
        (void) Mfree(p_ml);

    cputs("--- after Mfree ---\r\n");
    show_free_query("Mxalloc(-1, pref TT-RAM): ", query_mx_preftt);
    free_after = (LONG) Mxalloc(-1L, 3);
    cputs("Expected ~same as first query; got ");
    show_bytes(free_after);

    cputs("----------------------------------------\r\n");
    cputs("Done. Press a key in Hatari (AUTO waits here before desktop starts).\r\n");

    if (out_fd >= 0)
    {
        (void) Fclose((WORD) out_fd);
        out_fd = -1;
    }

    (void) Crawcin();
    return 0;
}

/*
 * TOS Pexec entry: longword at 4(sp) is the basepage pointer.
 * Keep stack as provided; no mint CRT.  (naked attr ignored on mintelf.)
 */
__asm__(
    "\t.globl\t_start\n"
    "\t.type\t_start, @function\n"
    "_start:\n"
    "\tmove.l\t4(%sp),%a0\n"      /* basepage (TOS ABI; unused here) */
    "\tjsr\tmaltest_main\n"
    "\tmove.w\t%d0,-(%sp)\n"
    "\tmove.w\t#0x4C,-(%sp)\n"  /* Pterm */
    "\ttrap\t#1\n"
    "1:\tbra\t1b\n"
    "\t.size\t_start, .-_start\n"
);
