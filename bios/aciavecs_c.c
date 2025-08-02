
#include "emutos.h"
#include "acia.h"
#include "aciavecs.h"
#include "amiga.h"
#include "bios.h"
#include "clock.h"
#include "ikbd.h"
#include "iorec.h"
#include "mfp.h"
#include "midi.h"
#include "string.h"

void just_rts(void);
void int_acia(void);   // ACIA interrupt handler
void int_acia_c(void); // C portion of the ACIA interrupt handler
void callKbdvecs_wrapper(void); // Wrapper for do_callKbdvecs, saving all registers that could be clobbered by the vectors
void callKbdvecs(void); // Called from callKbdvecs_wrapper

// Vectors from KBDVECS have to be called with data in registers, so these are the adapters
static void call_kbdvecs_b(void (*vector)(UBYTE), UBYTE);   // Call the given vector with the given byte in d0
static void call_kbdvecs_p(void (*vector)(UBYTE*), UBYTE*); // Call the given vector with the given pointer in d0
static void call_kbdvecs_bp(void *vector, UBYTE byte, void *ptr); // Call the given vector with the given byte in d0 and given pointer in a0

// MIDI stuff
#if CONF_WITH_MIDI_ACIA
#define MIDI_ACIA_CTRL 0xfffffc04
#define MIDI_ACIA_DATA 0xfffffc06
#endif
#define MIDI_BUFSIZE 128
UBYTE midiibufbuf[MIDI_BUFSIZE];
IOREC midiiorec;
static void midisys_handler(void);
static void midivec(UBYTE data);

// Keyboard stuff*
#if CONF_WITH_IKBD_ACIA
#define IKBD_ACIA_CTRL 0xfffffc00
#define IKBD_ACIA_DATA 0xfffffc02
#endif
#define IKBD_BUFSIZE 256
IOREC ikbdiorec;                 // Circular buffer information
UBYTE ikbdibufbuf[IKBD_BUFSIZE]; // Buffer to store data 

extern UBYTE kbdlength; // Decounter for the length of IKBD packet being received
WORD  kbdindex;  // Position of next byte in the kbdbuf buffer
UBYTE kbdbuf[8]; // Buffer where IKBD packets are reconstructed
// This kind of maps to the KBDVECS vector to call 
static BOOL in_packet;


// Message length per message type (first entry = length of 0xf6 message)
const UBYTE kbdlength_table[] = { 7, 5, 2, 2, 2, 2, 6, 2, 1, 1 };

static void ikbdsys_handler(void);
void call_kbdint(UBYTE data);

#if CONF_WITH_EXTENDED_MOUSE
    void *mousexvec(void); // mouse-routine for additional buttons
#endif

// Called to handle a single byte packet (typically a key up/down message) from the IKBD

const IOREC iorec_templates[] = {
    { ikbdibufbuf, IKBD_BUFSIZE, 0, 0, IKBD_BUFSIZE/4, 3*IKBD_BUFSIZE/4 },
    { midiibufbuf, MIDI_BUFSIZE, 0, 0, MIDI_BUFSIZE/4, 3*MIDI_BUFSIZE/4 }
};


void init_acia_vecs(void) {

#if CONF_WITH_EXTENDED_MOUSE
    mousexvec = (void (*)(WORD))just_rts;
#endif

    kbdvec = (void (*)(UBYTE,IOREC*))call_kbdint;
    kbdvecs.midivec = (void (*)(UBYTE))midivec;
    kbdvecs.vkbderr = (void (*)(UBYTE))just_rts;
    kbdvecs.vmiderr = (void (*)(UBYTE))just_rts;
    kbdvecs.statvec = (void (*)(UBYTE*))just_rts;

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    // The mousevec vector is already installed when initializing the screen/line-A
#else
    kbdvecs.mousevec = (void(*)(UBYTE*))just_rts;
#endif

#if CONF_WITH_IKBD_CLOCK
    kbdvecs.clockvec = clockvec;
#else
    kbdvecs.clockvec = (void(*)(UBYTE*))just_rts;
#endif

    kbdvecs.joyvec = (void(*)(UBYTE*))just_rts;
    kbdvecs.midisys = midisys_handler;
    kbdvecs.ikbdsys = ikbdsys_handler;

    // Initialize the IOREC circular buffers that store incoming data from the IKBD and MIDI
    // FIXME: this initialization should be done by the data segment if EmuTOS had a functional one
    memmove(&ikbdiorec, iorec_templates, sizeof(iorec_templates));
    
    // Initialise state machines
    in_packet = FALSE;

    // Flush input data
#if CONF_WITH_MIDI_ACIA
    *((volatile UBYTE* const)IKBD_ACIA_DATA);
#endif

    // Install the interrupt handler and enable the interrupt
#if CONF_WITH_IKBD_ACIA || CONF_WITH_MIDI_ACIA
    mfpint(6,(LONG)int_acia);
#endif
#ifdef MACHINE_AMIGA
    amiga_init_keyboard_interrupt();
#endif
}


// Interrupt handler called from int_acia
void int_acia_c(void) {
#if CONF_WITH_MFP
    do {

#if CONF_WITH_MIDI_ACIA
        kbdvecs.midisys();
#endif

#if CONF_WITH_IKBD_ACIA
        kbdvecs.ikbdsys();
#endif

    // Repeat as long as ACIA interrupt pending
    } while (!(MFP_BASE->gpip & 0b000010000));
    
    // Software end of interrupt
    MFP_BASE->isrb = ~0b01000000; // Bit 4 of the GPIP (ACIA IRQs)
#endif
}




// Handle MIDI interrupt
static void midisys_handler(void) {
#if CONF_WITH_MIDI_ACIA
    SBYTE status = *((UBYTE*)MIDI_ACIA_CTRL);

    if (!(status & ACIA_IRQ) || !(status & ACIA_RDRF))
        return; // No IRQ or nothing received (was TX interrupt ?)

    call_kbdvecs_b(kbdvecs.midivec, *((UBYTE*)MIDI_ACIA_DATA));

    if (status & ACIA_OVRN) {
        // Overrun
        call_kbdvecs_b(kbdvecs.vmiderr, *((UBYTE*)MIDI_ACIA_DATA));
    }
#endif /* CONF_WITH_MIDI_ACIA */
}

// Store the data in the circular buffer, if not full
static void midivec(UBYTE data) {
// FIXME this is not correct to condition this to CONF_WITH_MIDI_ACIA
// because we don't have an ACIA that we don't have MIDI.
    UWORD tail;
    IOREC *iorec = &midiiorec;
 
    tail = iorec->tail + 1;
 
    if (tail >= iorec->size)
        tail = 0;

    if (tail == iorec->head)
        return; // buffer full

    iorec->buf[tail] = data;
    iorec->tail = tail;
}

// Handle keyboard/mouse interrupt
static void ikbdsys_handler(void) {
#if CONF_WITH_IKBD_ACIA
    SBYTE status = *((UBYTE*)IKBD_ACIA_CTRL);

    if (!(status & ACIA_IRQ) || !(status & ACIA_RDRF))
        return; // No IRQ or nothing received

    call_ikbdraw(*((UBYTE*)IKBD_ACIA_DATA));

    if (status & ACIA_OVRN) {
        // Overrun
        call_kbdvecs_b(kbdvecs.vkbderr, *((UBYTE*)IKBD_ACIA_DATA));
    }
#endif /* CONF_WITH_IKBD_ACIA */
}


// Packets received from the IKBD are accumulated into the kbdbuf buffer.
// The packet header (F6 to FF) determines the packet length and the
// action to be taken once the packet has been received completely.
// During the reception of a packet, variable in_packet contains the
// action number, and variable kbdlength contains the number of bytes
// not received yet.
//
// action <--whole IKBD packet-->  Comment
// number    <-given to routine->
//
//  1     F6 a1 a2 a3 a4 a5 a6 a7 (miscellaneous, 7 bytes)
//  2     F7 0b xh xl yh yl       (absolute mouse)
//  3     F8 dx dy                (relative mouse, no button)
//  3     F9 dx dy                (relative mouse, button 1)
//  3     FA dx dy                (relative mouse, button 2)
//  3     FB dx dy                (relative mouse, both buttons)
//  4     FC yy MM dd hh mm ss    (date and time)
//  5     FD j0 j1                (both joysticks)
//  6     FE bj                   (joystick 0)
//  7     FF bj                   (joystick 1)

/* Handles the reception of a single byte from the IKBD */
void ikbdraw(UBYTE byte) {
    if (in_packet) {
        // Receiving a multi-byte packet
        kbdbuf[kbdindex] = byte;
        if (--kbdlength == 0) {
            // We're done receiving, now dispatch the info to the handlers
            in_packet = FALSE;
            callKbdvecs();
            return;
        }
        kbdindex++;
    }
    else if (byte >= 0xf6) {
        // Packet header -> new (multi-byte) packet
        kbdlength = kbdlength_table[byte - 0xf6];
        kbdbuf[0] = byte;
        in_packet = TRUE;
        kbdindex = 1;
    }
    else {
        // Single-byte message
        kbdbuf[0] = 0xf5; // Made-up code for key up/down action
        kbdbuf[1] = byte;
        callKbdvecs();
    }
}

// Calls a KBDVECS vector depending on the received IKBD packet type
void callKbdvecs(void) {
    switch (kbdbuf[0]) {
        case 0xf5: call_kbdvecs_bp(kbdvec, kbdbuf[1], &ikbdiorec); break; // 0xf5 is a made up value by ikbdraw for single byte key events
        case 0xf6: call_kbdvecs_p(kbdvecs.statvec, &kbdbuf[1]); break;    // Multi-byte packet
        case 0xf7: call_kbdvecs_p(kbdvecs.mousevec, &kbdbuf[1]); break;   // Absolute mouse position
        case 0xf8: // Fall through                                        // Relative mouse position
        case 0xf9: // Fall through                                        // Relative mouse position, button 1
        case 0xfa: // Fall through                                        // Relative mouse position, button 2
        case 0xfb: call_kbdvecs_p(kbdvecs.mousevec, kbdbuf); break;       // Relative mouse position both buttons
        case 0xfc: call_kbdvecs_p(kbdvecs.clockvec, &kbdbuf[1]); break;   // Real time clock
        // IKBD packet 0xFD (the response to command 0x16) is treated the same as
        // most packets: the buffer passed to the joyvec function just contains
        // the data (joystick 0, joystick 1).
        //
        // However, for IKBD packets 0xFE and 0xFF, the buffer will contain
        // (package header, joystick 0, joystick 1).
        // For TOS compatibility (true up to at least TOS 3.06), we support the
        // following undocumented feature: on entry to the 'joyvec' routine,
        // d0.b contains the last data byte received.
        case 0xfd: call_kbdvecs_bp(kbdvecs.joyvec, kbdbuf[2], &kbdbuf[1]); break; // Joystick 0 and 1
        case 0xfe: call_kbdvecs_bp(kbdvecs.joyvec, kbdbuf[1], kbdbuf); break;     // Header and joystick 0
        case 0xff: kbdbuf[1] = kbdbuf[2];                                         // Header and joystick 1
                   call_kbdvecs_bp(kbdvecs.joyvec, kbdbuf[2], kbdbuf); break;
    }
}


// Call the vector with the byte in d0 and the vector in a1
static ALWAYS_INLINE void call_kbdvecs_b(void (*vector)(UBYTE),UBYTE byte) {
    register const UBYTE regbyte __asm__("d0") = byte;
    register void (*regvector)(UBYTE)  __asm__("a1") = vector;
    __asm__ volatile (
       "\n\tmove.w %1,-(sp)"
       "\n\tjsr (%0)"
       "\n\taddq.l #2,sp"
        : // No output
        : "a"(regvector), "d" (regbyte) // Keep that here otherwize things will be optimized out
        : "d1","d2","a0","a2"
        );
}


// Call the vector with the byte in d0, the pointer in a0 and the vector in a1
static ALWAYS_INLINE void call_kbdvecs_bp(void *vector, UBYTE byte, void *ptr) {
    register const UBYTE regbyte __asm__("d0") = byte;
    register void *regptr __asm__("a0") = ptr;
    register void *regvector __asm__("a1") = vector;
    __asm__ volatile (
       "\n\tmove.w %1,-(sp)"
       "\n\tjsr (%0)"
       "\n\taddq.l #2,sp"
        : // No output
        : "a"(regvector), "d" (regbyte), "a" (regptr) // Keep that here otherwize things will be optimized out
        : "d1","d2","a2"
        );
}


// Call the vector with the pointer in a0 and the vector in a1
static ALWAYS_INLINE void call_kbdvecs_p(void (*vector)(UBYTE*),UBYTE *byte) {
    register const UBYTE *regbyte __asm__("a0") = byte;
    register void (*regvector)(UBYTE*)  __asm__("a1") = vector;
    __asm__ volatile (
       "\n\tmove.l a0,-(sp)" 
       "\n\tjsr (a1)"
       "\n\taddq.l #4,sp"
        : // No output
        : "a"(regvector), "a" (regbyte) // Keep that here otherwize things will be optimized out
        : "d0","d1","d2","a2"
        );
}