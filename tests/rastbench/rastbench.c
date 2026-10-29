/*
 * rastbench.c - time Atari VDI text, solid fills, and lines.
 *
 * Freestanding TOS program.  The 200 Hz timer ticks only every 5 ms, so
 * one VDI call is not a sample.  Each case repeats the same primitive
 * enough times for the single timed run to last at least two seconds.
 * If that run comes back short, it is timed again with more repeats.
 * Speed is draws/ticks (higher is faster).
 *
 * The first 2KB of ST memory is supervisor-only, so the program enters
 * supervisor with GEMDOS Super(0) before it reads the timer.
 *
 * Launch as a GEM program (EMUDESK.INF #Z 01), not AUTO and not a
 * character-mode #Z 00, which closes the workstation.  Opens a virtual
 * workstation because AES already owns the physical one, and turns the
 * blitter off so solid fills take the software path.
 *
 * Build: tools/rastbench.sh
 */

#define HZ200           (*(volatile unsigned long *)0x4baL)
#define SAMPLE_TICKS    400UL         /* one run of at least two seconds */
#define N_SAMPLES       1
#define TEXT_N          16

/* vst_effects bits */
#define FX_BOLD         1
#define FX_ITALIC       4
#define FX_UNDER        8
#define FX_OUTLINE      16

#define GEMDOS_SUPER    0x0020
#define GEMDOS_CCONWS   0x0009
#define GEMDOS_DGETDRV  0x0019
#define GEMDOS_FCREATE  0x003c
#define GEMDOS_FCLOSE   0x003e
#define GEMDOS_FWRITE   0x0040
#define XBIOS_GETREZ    4
#define XBIOS_BLITMODE  64

typedef long            LONG;
typedef unsigned long   ULONG;
typedef short           WORD;
typedef unsigned short  UWORD;

static WORD contrl[12];
static WORD intin[16];
static WORD ptsin[8];
static WORD intout[48];
static WORD ptsout[16];
static WORD textin[TEXT_N];
static WORD handle;
static WORD xmax, ymax, grez;
static WORD text_x, text_y;
static int out_fd = -1;

static const char text_src[TEXT_N + 1] = "0123456789ABCDEF";

typedef struct {
    WORD *contrl;
    WORD *intin;
    WORD *ptsin;
    WORD *intout;
    WORD *ptsout;
} VDIPB;

static VDIPB vdipb;

static LONG gemdos0(WORD op)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.w %1,-(%%sp)\n\t"
        "trap #1\n\t"
        "addq.l #2,%%sp"
        : "=r"(ret)
        : "g"(op)
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
        : "=r"(ret)
        : "g"(op), "g"(a)
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
        : "=r"(ret)
        : "g"(op), "g"(a)
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
        : "=r"(ret)
        : "g"(op), "g"(a), "g"(b)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static LONG gemdos_fwrite(WORD fh, LONG count, const void *buf)
{
    register LONG ret __asm__("d0");

    __asm__ volatile (
        "move.l %3,-(%%sp)\n\t"
        "move.l %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "move.w #0x40,-(%%sp)\n\t"
        "trap #1\n\t"
        "lea 12(%%sp),%%sp"
        : "=r"(ret)
        : "g"(fh), "g"(count), "g"(buf)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static WORD xbios0(WORD op)
{
    register WORD ret __asm__("d0");

    __asm__ volatile (
        "move.w %1,-(%%sp)\n\t"
        "trap #14\n\t"
        "addq.l #2,%%sp"
        : "=r"(ret)
        : "g"(op)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static WORD xbios_w(WORD op, WORD arg)
{
    register WORD ret __asm__("d0");

    __asm__ volatile (
        "move.w %2,-(%%sp)\n\t"
        "move.w %1,-(%%sp)\n\t"
        "trap #14\n\t"
        "addq.l #4,%%sp"
        : "=r"(ret)
        : "g"(op), "g"(arg)
        : "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
    return ret;
}

static void nf_shutdown(void)
{
    const char *name = "NF_SHUTDOWN";

    __asm__ volatile (
        "move.l %0,-(%%sp)\n\t"
        "pea 1f(%%pc)\n\t"
        ".dc.w 0x7300\n\t"
        "tst.l %%d0\n\t"
        "beq 2f\n\t"
        "move.l %%d0,4(%%sp)\n\t"
        ".dc.w 0x7301\n\t"
        "2:\n\t"
        "addq.l #8,%%sp\n\t"
        "1:\n"
        :
        : "r"(name)
        : "d0", "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
}

static void nf_stderr(const char *msg)
{
    const char *name = "NF_STDERR";

    __asm__ volatile (
        "move.l %0,-(%%sp)\n\t"
        "move.l %1,-(%%sp)\n\t"
        "pea 1f(%%pc)\n\t"
        ".dc.w 0x7300\n\t"
        "tst.l %%d0\n\t"
        "beq 2f\n\t"
        "move.l %%d0,4(%%sp)\n\t"
        ".dc.w 0x7301\n\t"
        "2:\n\t"
        "lea 12(%%sp),%%sp\n\t"
        "1:\n"
        :
        : "r"(msg), "r"(name)
        : "d0", "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
}

static void vdi(WORD opcode, WORD npt, WORD nint)
{
    contrl[0] = opcode;
    contrl[1] = npt;
    contrl[3] = nint;
    contrl[6] = handle;

    __asm__ volatile (
        "move.l %0,%%d1\n\t"
        "moveq #0x73,%%d0\n\t"
        "trap #2"
        :
        : "g"(&vdipb)
        : "d0", "d1", "d2", "a0", "a1", "a2", "memory", "cc"
    );
}

static char *put_str(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
    return d;
}

static char *put_u32(char *d, ULONG v)
{
    char tmp[12];
    int i = 0;

    if (v == 0) {
        *d++ = '0';
        *d = 0;
        return d;
    }
    while (v && i < 10) {
        tmp[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i > 0)
        *d++ = tmp[--i];
    *d = 0;
    return d;
}

static char *put_i16(char *d, WORD v)
{
    if (v < 0) {
        *d++ = '-';
        v = (WORD)-v;
    }
    return put_u32(d, (ULONG)(UWORD)v);
}

static void emit(const char *s)
{
    const char *p = s;

    while (*p)
        p++;
    nf_stderr(s);
    (void)gemdos_l(GEMDOS_CCONWS, (LONG)s);
    if (out_fd >= 0)
        (void)gemdos_fwrite((WORD)out_fd, (LONG)(p - s), s);
}

static void report(const char *name, ULONG draws, ULONG ticks,
                   ULONG lo, ULONG hi, ULONG sample)
{
    char buf[160];
    char *p = buf;

    p = put_str(p, name);
    p = put_str(p, " rez=");
    p = put_i16(p, grez);
    p = put_str(p, " draws=");
    p = put_u32(p, draws);
    p = put_str(p, " ticks=");
    p = put_u32(p, ticks);
    p = put_str(p, " samples=");
    p = put_i16(p, N_SAMPLES);
    p = put_str(p, " sample=");
    p = put_u32(p, sample);
    p = put_str(p, " lo=");
    p = put_u32(p, lo);
    p = put_str(p, " hi=");
    p = put_u32(p, hi);
    p = put_str(p, "\r\n");
    emit(buf);
}

/* Ticks taken by n back-to-back calls, aligned to a timer edge. */
static ULONG time_calls(void (*fn)(void), ULONG n)
{
    ULONG t0, i;

    t0 = HZ200;
    while (HZ200 == t0)
        ;
    t0 = HZ200;
    for (i = 0; i < n; i++)
        fn();
    return HZ200 - t0;
}

/*
 * One call is usually shorter than a 5 ms tick, so a probe grows until
 * it covers several ticks.  That probe is scaled up to one run of at
 * least two seconds.  If the run comes back short, the repeat count
 * was too small and it is timed again.
 */
static void bench_case(const char *name, void (*fn)(void))
{
    ULONG burst = 1;
    ULONG dt, n, draws, ticks, lo, hi;
    WORD s, pass;

    for (;;) {
        dt = time_calls(fn, burst);
        if (dt >= 20 || burst >= 4096)
            break;
        burst <<= 2;
    }
    if (dt < 1)
        dt = 1;
    n = burst * ((SAMPLE_TICKS + dt - 1) / dt);
    if (n < 1)
        n = 1;

    draws = 0;
    ticks = 0;
    lo = 0;
    hi = 0;
    for (pass = 0; pass < 4; pass++) {
        draws = 0;
        ticks = 0;
        lo = 0xffffffffUL;
        hi = 0;
        for (s = 0; s < N_SAMPLES; s++) {
            dt = time_calls(fn, n);
            draws += n;
            ticks += dt;
            if (dt < lo)
                lo = dt;
            if (dt > hi)
                hi = dt;
        }
        if (lo >= SAMPLE_TICKS)
            break;
        if (lo < 1)
            lo = 1;
        n = (n * SAMPLE_TICKS + lo - 1) / lo;
    }
    report(name, draws, ticks, lo, hi, n);
}

static void draw_gtext(void)
{
    ptsin[0] = text_x;
    ptsin[1] = text_y;
    vdi(8, 1, TEXT_N);
}

static void draw_recfl(void)
{
    vdi(114, 2, 0);
}

static void draw_pline(void)
{
    vdi(6, 2, 0);
}

static void set_int1(WORD opcode, WORD val)
{
    intin[0] = val;
    vdi(opcode, 0, 1);
}

static void set_align(WORD h, WORD v)
{
    intin[0] = h;
    intin[1] = v;
    vdi(39, 0, 2);
}

static void open_vwk(void)
{
    WORD i;

    grez = xbios0(XBIOS_GETREZ);
    for (i = 0; i < 11; i++)
        intin[i] = 1;
    intin[0] = grez + 2;
    intin[7] = 1;       /* solid fill */
    intin[10] = 2;      /* raster coordinates */
    handle = 0;
    vdi(100, 0, 11);
    handle = contrl[6];
    xmax = intout[0];
    ymax = intout[1];
}

static void make_outpath(char *buf)
{
    WORD drv = (WORD)gemdos0(GEMDOS_DGETDRV);

    buf[0] = (char)('A' + drv);
    buf[1] = ':';
    buf[2] = '\\';
    put_str(buf + 3, "RASTBENCH.TXT");
}

static void set_int2(WORD opcode, WORD a, WORD b)
{
    intin[0] = a;
    intin[1] = b;
    vdi(opcode, 0, 2);
}

/*
 * Point 8 is the 6x6 face, point 9 the 8x8 face, point 10 the 8x16 face.
 * Effects miss the direct string blit, as does any x that is not a
 * multiple of 8.  16 cells stay inside a 320-wide screen.
 *
 * w16 is a 16-pixel boundary, b8 an 8-pixel boundary, odd neither.
 */
static const char *const face_name[3] = { "6x6", "8x8", "8x16" };
static const WORD face_point[3] = { 8, 9, 10 };
static const WORD face_cell[3] = { 6, 8, 8 };
static const char *const align_name[3] = { "w16", "b8", "odd" };
static const WORD align_x[3] = { 16, 8, 3 };

static char case_name[48];

static const char *make_name(const char *kind, const char *a,
                             const char *b, const char *c)
{
    char *p = case_name;

    p = put_str(p, kind);
    p = put_str(p, "_");
    p = put_str(p, a);
    if (b[0]) {
        p = put_str(p, "_");
        p = put_str(p, b);
    }
    if (c[0]) {
        p = put_str(p, "_");
        p = put_str(p, c);
    }
    return case_name;
}

static void text_case(const char *name, WORD x, WORD halign, WORD style, WORD point)
{
    text_x = x;
    text_y = 16;
    vdipb.intin = intin;
    set_align(halign, 5);   /* top of the cell, so DESTY is text_y */
    set_int1(106, style);
    set_int1(107, point);
    vdipb.intin = textin;
    bench_case(name, draw_gtext);
}

static void line_case(const char *name, WORD x1, WORD y1, WORD x2, WORD y2,
                      WORD style, WORD width, WORD endstyle)
{
    vdipb.intin = intin;
    set_int1(15, style);
    ptsin[0] = width;
    ptsin[1] = 0;
    vdi(16, 1, 0);              /* vsl_width */
    set_int2(108, endstyle, endstyle);  /* vsl_ends */
    ptsin[0] = x1;
    ptsin[1] = y1;
    ptsin[2] = x2;
    ptsin[3] = y2;
    bench_case(name, draw_pline);
}

int rastbench_main(void)
{
    char path[20];
    WORD i, fi, ai;
    WORD blit;
    char buf[80];
    char *p;
    WORD span, height, x0, cell, width;

    /* Super(0): stay on this stack, but in supervisor so 0x4ba is readable. */
    (void)gemdos_l(GEMDOS_SUPER, 0);

    vdipb.contrl = contrl;
    vdipb.intin = intin;
    vdipb.ptsin = ptsin;
    vdipb.intout = intout;
    vdipb.ptsout = ptsout;

    for (i = 0; i < TEXT_N; i++)
        textin[i] = (WORD)(unsigned char)text_src[i];

    make_outpath(path);
    out_fd = (int)gemdos_lw(GEMDOS_FCREATE, (LONG)path, 0);

    open_vwk();
    if (handle == 0) {
        emit("v_opnvwk failed\r\n");
        emit("RASTBENCH done\r\n");
        if (out_fd >= 0)
            (void)gemdos_w(GEMDOS_FCLOSE, (WORD)out_fd);
        nf_shutdown();
        return 1;
    }

    /* Attribute calls use intin[], text calls use textin[]. */
    vdipb.intin = intin;

    blit = xbios_w(XBIOS_BLITMODE, 0);
    p = put_str(buf, "blitmode=0 prev=");
    p = put_i16(p, blit);
    p = put_str(p, " xmax=");
    p = put_i16(p, xmax);
    p = put_str(p, " ymax=");
    p = put_i16(p, ymax);
    put_str(p, "\r\n");
    emit(buf);

    set_int1(32, 1);    /* replace */
    set_int1(22, 1);    /* text colour */
    set_int1(25, 1);    /* fill colour */
    set_int1(17, 1);    /* line colour */
    set_int1(23, 1);    /* solid interior */
    set_int1(15, 1);    /* solid line */

    /*
     * Centre and right anchors are chosen so the left edge (DESTX) lands
     * on the same pixel as the left-aligned case.  Plain text has no
     * skew, so that edge is the x passed to v_gtext.
     */
    for (fi = 0; fi < 3; fi++) {
        cell = face_cell[fi];
        width = TEXT_N * cell;
        vdipb.intin = intin;
        set_int1(107, face_point[fi]);
        p = put_str(buf, "font ");
        p = put_str(p, face_name[fi]);
        p = put_str(p, " point=");
        p = put_i16(p, face_point[fi]);
        p = put_str(p, " got=");
        p = put_i16(p, intout[0]);
        put_str(p, "\r\n");
        emit(buf);

        for (ai = 0; ai < 3; ai++) {
            x0 = align_x[ai];
            text_case(make_name("text", face_name[fi], "left", align_name[ai]),
                      x0, 0, 0, face_point[fi]);
            text_case(make_name("text", face_name[fi], "center", align_name[ai]),
                      (WORD)(x0 + width / 2), 1, 0, face_point[fi]);
            text_case(make_name("text", face_name[fi], "right", align_name[ai]),
                      (WORD)(x0 + width), 2, 0, face_point[fi]);

            text_case(make_name("text", face_name[fi], "bold", align_name[ai]),
                      x0, 0, FX_BOLD, face_point[fi]);
            text_case(make_name("text", face_name[fi], "italic", align_name[ai]),
                      x0, 0, FX_ITALIC, face_point[fi]);
            text_case(make_name("text", face_name[fi], "under", align_name[ai]),
                      x0, 0, FX_UNDER, face_point[fi]);
            text_case(make_name("text", face_name[fi], "outline", align_name[ai]),
                      x0, 0, FX_OUTLINE, face_point[fi]);
            text_case(make_name("text", face_name[fi], "bold_italic", align_name[ai]),
                      x0, 0, FX_BOLD | FX_ITALIC, face_point[fi]);
            text_case(make_name("text", face_name[fi], "bold_under", align_name[ai]),
                      x0, 0, FX_BOLD | FX_UNDER, face_point[fi]);
            text_case(make_name("text", face_name[fi], "italic_under", align_name[ai]),
                      x0, 0, FX_ITALIC | FX_UNDER, face_point[fi]);
            text_case(make_name("text", face_name[fi], "bold_italic_under", align_name[ai]),
                      x0, 0, FX_BOLD | FX_ITALIC | FX_UNDER, face_point[fi]);
            text_case(make_name("text", face_name[fi], "allfx", align_name[ai]),
                      x0, 0, FX_BOLD | FX_ITALIC | FX_UNDER | FX_OUTLINE, face_point[fi]);
        }
    }

    vdipb.intin = intin;
    set_int1(106, 0);   /* effects off for the fills and lines */
    /* Width follows the screen. Height follows it too: medium is 640x200,
     * so a 300-pixel fill would run off the bottom. */
    span = (xmax >= 600) ? 592 : 288;
    height = (ymax >= 300) ? 300 : 160;
    set_int1(23, 1);
    for (ai = 0; ai < 3; ai++) {
        x0 = align_x[ai];
        ptsin[0] = x0;
        ptsin[1] = 16;
        ptsin[2] = (WORD)(x0 + span - 1);
        ptsin[3] = (WORD)(16 + height - 1);
        bench_case(make_name("fill", "solid", align_name[ai], ""), draw_recfl);
    }

    set_int1(23, 3);    /* hatch */
    set_int1(24, 1);
    for (ai = 0; ai < 3; ai++) {
        x0 = align_x[ai];
        ptsin[0] = x0;
        ptsin[1] = 16;
        ptsin[2] = (WORD)(x0 + span - 1);
        ptsin[3] = (WORD)(16 + height - 1);
        bench_case(make_name("fill", "hatch", align_name[ai], ""), draw_recfl);
    }
    set_int1(23, 1);

    /* style 1 is solid (no mask).  style 2 is the long dash, LN_MASK
     * 0xFFF0.  endstyle 1 draws an arrow at each end.  width 3 leaves
     * the single-pixel vertical fast path.  Each line has the same
     * length; only the horizontal origin changes. */
    for (ai = 0; ai < 3; ai++) {
        x0 = align_x[ai];
        line_case(make_name("hline", "solid", align_name[ai], ""),
                  x0, ymax / 2, (WORD)(x0 + xmax - 16), ymax / 2, 1, 1, 0);
        line_case(make_name("hline", "mask", align_name[ai], ""),
                  x0, ymax / 3, (WORD)(x0 + xmax - 16), ymax / 3, 2, 1, 0);
        line_case(make_name("hline", "arrow", align_name[ai], ""),
                  x0, ymax / 4, (WORD)(x0 + xmax - 16), ymax / 4, 1, 1, 1);
        line_case(make_name("hline", "wide", align_name[ai], ""),
                  x0, (ymax * 2) / 3, (WORD)(x0 + xmax - 16), (ymax * 2) / 3, 1, 3, 0);
        line_case(make_name("vline", "solid", align_name[ai], ""),
                  x0, 0, x0, ymax, 1, 1, 0);
        line_case(make_name("vline", "mask", align_name[ai], ""),
                  x0, 0, x0, ymax, 2, 1, 0);
        line_case(make_name("vline", "arrow", align_name[ai], ""),
                  x0, 0, x0, ymax, 1, 1, 1);
        line_case(make_name("vline", "wide", align_name[ai], ""),
                  x0, 0, x0, ymax, 1, 3, 0);
        /* 45 degrees, same length on both axes, so this misses both
         * the horizontal and the vertical fast paths. */
        line_case(make_name("dline", "solid", align_name[ai], ""),
                  x0, 0, (WORD)(x0 + ymax), ymax, 1, 1, 0);
        line_case(make_name("dline", "mask", align_name[ai], ""),
                  x0, 0, (WORD)(x0 + ymax), ymax, 2, 1, 0);
        line_case(make_name("dline", "arrow", align_name[ai], ""),
                  x0, 0, (WORD)(x0 + ymax), ymax, 1, 1, 1);
        line_case(make_name("dline", "wide", align_name[ai], ""),
                  x0, 0, (WORD)(x0 + ymax), ymax, 1, 3, 0);
    }

    emit("RASTBENCH done\r\n");

    vdi(101, 0, 0);     /* v_clsvwk */

    if (out_fd >= 0)
        (void)gemdos_w(GEMDOS_FCLOSE, (WORD)out_fd);

    nf_shutdown();
    return 0;
}

/*
 * Pexec hands the process the whole free TPA.  v_opnvwk() allocates the
 * virtual workstation with Mxalloc, so give the unused tail back first.
 * The stack has to move before Mshrink: the one TOS built sits at hitpa.
 */
__asm__(
    "\t.globl\t_start\n"
    "\t.type\t_start, @function\n"
    "\t.lcomm\trastbench_stack,16384\n"
    "_start:\n"
    "\tmove.l\t4(%sp),%a0\n"
    "\tlea\trastbench_stack+16384,%sp\n"
    "\tmove.l\t12(%a0),%d0\n"
    "\tadd.l\t20(%a0),%d0\n"
    "\tadd.l\t28(%a0),%d0\n"
    "\tadd.l\t#256+1,%d0\n"
    "\tand.b\t#-2,%d0\n"
    "\tmove.l\t%d0,-(%sp)\n"
    "\tmove.l\t%a0,-(%sp)\n"
    "\tclr.w\t-(%sp)\n"
    "\tmove.w\t#0x4a,-(%sp)\n"
    "\ttrap\t#1\n"
    "\tlea\t12(%sp),%sp\n"
    "\tjsr\trastbench_main\n"
    "\tmove.w\t%d0,-(%sp)\n"
    "\tmove.w\t#0x4C,-(%sp)\n"
    "\ttrap\t#1\n"
    "1:\tbra\t1b\n"
    "\t.size\t_start, .-_start\n"
);
