//
// Created by Rob Gowin on 10/22/21.
//

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "zmodem.h"
#include "zmdm.h"

#include <osbind.h>

#define IBUFSIZ               32000 /* Size of Rs232 receive buffer, can't exceed 32767. */

extern int opt_d;
extern int opt_v;
extern int last_sent;

int AUX_DEV_CMD_LINE=-1;
int AUX_DEV = 1;
int CONSOLE_DEV = 2;

int fd_init_done = 0;

#ifdef DEBUG
extern int raw_trace;

#endif

_IOREC save,     /* the original Iorec is saved here for the duration of this process */
     *savep;     /* ptr returned by Iorec() */

char iobuf[IBUFSIZ]; /* RS232 receive buffer. */

#define C__MCH 0x5F4D4348L     /* Machine Type */
#define MCH_TINY68K 0x80000000L

#define RING_BUFFER_SIZE 48000 /* MAX 65535 */

struct ring_buffer {
    unsigned char buffer[RING_BUFFER_SIZE];
    uint16_t size, head, tail;
};

struct ring_buffer ring_buffer;

/*
 * routines to make the io channel raw and restore it
 * to its normal state.
 *
 * No-ops on the Atari ST.
 */

int16_t have_bconmap = 0;

static int16_t has_bconmap(void)
{
    return (0L == Bconmap(0));
}
int32_t old_bconmap_dev = -1;

typedef struct
{
    long id;
    long value;
} COOKJAR;

int get_cookie(long cookie, void *value)
{
    int result = 0;
    COOKJAR *cookiejar = (COOKJAR *)(Setexc(0x5A0/4, (const void (*)(void)) -1 ));
    if (cookiejar) {
        for (int i = 0; cookiejar[i].id; i++) {
             if (cookiejar[i].id == cookie) {
                  if (value) {
                      *(long *) value = cookiejar[i].value;
                      result = 1;
                      break;
                  }
             } 
        }
    }
    return result;
}

void fd_init(void)
{
    /* - If AUX_DEV has been set on the command line, use that.
     * - If we detect that we're running on a Tiny68K via the
     *   cookie jar, use the default value appropriate to that.
     * - Otherwise, use the hardcoded defaults given above.
     */

    if (AUX_DEV_CMD_LINE != -1) AUX_DEV = AUX_DEV_CMD_LINE;
    else {
        long cookie_value = -1;
        int result = get_cookie(C__MCH, &cookie_value);
        if (result && (cookie_value == MCH_TINY68K)) {
            if (opt_v) printf("Tiny68k detected. Using device 10 for AUX port.\n");
            AUX_DEV = 10;
        }
    }

    ring_buffer.size = RING_BUFFER_SIZE;
    ring_buffer.head = ring_buffer.tail = 0;

    if (has_bconmap()) {
        /* Map in our device so that Iorec and Rsconf act on it. */
        old_bconmap_dev = Bconmap(AUX_DEV);
        have_bconmap = 1;
    }

    /* Get pointer to Rs232 input record */
    savep = (_IOREC * )Iorec(0);

    /* Save the info */
    save.ibuf = savep->ibuf;
    save.ibufsiz = savep->ibufsiz;
    save.ibufhd = savep->ibufhd;
    save.ibuftl = savep->ibuftl;
    save.ibuflow = savep->ibuflow;
    save.ibufhi = savep->ibufhi;

    /* Install my buffer in its place */
    savep->ibuf = &iobuf[0];
    savep->ibufsiz = IBUFSIZ;
    savep->ibuflow = IBUFSIZ / 8;
    savep->ibufhi = (IBUFSIZ / 4) * 3;
    savep->ibufhd = savep->ibuftl = 0;

    /* Turn on hardware flow control. */
    const int flow_ctrl_hard = 2;
    Rsconf(-1, flow_ctrl_hard, -1, -1, -1, -1);

    if (have_bconmap) Bconmap(old_bconmap_dev);

    fd_init_done = 1;
}

void fd_exit(void)
{
    if (!fd_init_done) return;

    /* Reset to saved system buffer. */
    /* FIXME: This isn't safe in the presence of RS232 interrupts. Make it code that is executed as supervisor? */
    savep->ibuf     = save.ibuf;
    savep->ibufsiz  = save.ibufsiz;
    savep->ibuflow  = save.ibuflow;
    savep->ibufhi   = save.ibufhi;
    savep->ibufhd   = save.ibufhd;
    savep->ibuftl   = save.ibuftl;

    /* Map in our device so that Rsconf acts on it. */
    if (have_bconmap) Bconmap(AUX_DEV);
    /* Turn off hardware flow control. There's no way to restore previous value. */
    Rsconf(-1, 0, -1, -1, -1, -1);

    if (have_bconmap) Bconmap(old_bconmap_dev);
}

/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void rx_purge(void)
{

    while(Bconstat(AUX_DEV)) Bconin(AUX_DEV);
}

/*
 * send the bytes accumulated in the output buffer.
 */

void tx_flush(void)
{
    while (Bcostat(AUX_DEV) == 0)
        ;
}

/*
 * transmit a character.
 * this is the raw modem interface
 */

void tx_raw(int c)
{
#ifdef DEBUG
    if (raw_trace) {
		fprintf(stderr,"%02x ",c);
	}
#endif

    last_sent = c & 0x7f;

    Bconout(AUX_DEV, c);
}

int rx_poll(void)
{
    return Bconstat(AUX_DEV) != 0;
}

volatile long present_time;
static long alrm_time = 0L; /* Time of next timeout (200 Hz) */

static long *hz_200 =  (long *)0x000004ba; /* Yes the Hitch Hikers Guide is wrong!! */

jmp_buf tohere;

/*
 * rd_time() - read the value of the systems 200 Hz counter
 * must be called in Super Mode else you get
 * what you deserve - MUSHROOMS!!
 */
static void rd_time()
{
    present_time = *hz_200;
}

/*
 * alarm() - set the alarm time
 */
static void stalarm(long n) /* n is number of seconds, roughly */
{
    alrm_time = 0;
    if (n > 0) {
        Supexec(rd_time);
        /* We really need n * 200 but n * 256 is close enough */
        alrm_time = present_time + ( n << 8 );
    }
}

int check_user_abort()
{
    int result = 0;
    if (AUX_DEV != CONSOLE_DEV) {
        if (Bconstat(CONSOLE_DEV)) {
            if ((Bconin(CONSOLE_DEV) & 0x7f) == 0x3) { /* is console char a CTRL-C? */
                result = 1;
            }
        }
    }
    return result;
}

static int read_modem(struct ring_buffer *ringbuf)
{
    int read_count = 0;
    while (1) {
        /* See if there are characters available from the modem. If so, read them. */
        while (Bconstat(AUX_DEV)) {
            /* There is a character available. See if there is room in the buffer before accepting the character. */
            uint16_t tail = ringbuf->tail + 1;
            if (tail == ringbuf->size) tail = 0;
            if (tail == ringbuf->head) break; /* No room in the ring buffer. */
            else {
                ringbuf->buffer[tail] = Bconin(AUX_DEV);
                ringbuf->tail = tail;
                read_count++;
            }
        }
        if (read_count) return read_count;
        /* We haven't read any characters yet, check for a timeout if requested. */
        if (alrm_time != 0) {
            Supexec(rd_time);
            if (present_time > alrm_time) {
                /* timeout */
                longjmp(tohere, -1);
            }
        }
    }
    return read_count;
}

/*
 * rx_raw(): Receive a single byte from the line. Reads as many are available and then processes them one at a time.
 * Checks the data stream for five consecutive CAN characters; if seen then abort. This saves a lot of clutter in
 * the rest of the code even though it is a very strange place for an exit. (But that was what session abort was
 * all about.)
 */

int rx_raw(int timeout)
{
    static int num_cancels = 0;

    uint16_t tail = ring_buffer.tail;
    if (tail >= ring_buffer.size) tail = 0;
    /* If there isn't a character available, wait for the modem to get one. */
    if (tail == ring_buffer.head) {
        /* Change the timeout into seconds; minimum is 2. */
        int timeout_secs = timeout / 1024;
        if (timeout_secs == 0) timeout_secs = 2;

        /* Set up an alarm in case I/O takes too long. */
        if (setjmp(tohere)) {
            // If there is a timeout, perhaps something is wrong. Anyway,
            // performance is no longer a concern in this case, so convenient
            // to check for a user abort here.
            if (check_user_abort()) {
                cleanup();
                printf("Exiting due to Control-C.\r\n");
                exit(CAN);
            }
            return TIMEOUT;
        }

        stalarm(timeout_secs);
        int n = read_modem(&ring_buffer);
        stalarm(0);

        if (n == 0) return TIMEOUT;
    }
    /* At this point, there is at least one character available in the ring buffer. */
    ring_buffer.head++;
    if (ring_buffer.head >= ring_buffer.size) ring_buffer.head = 0;
    int c = ring_buffer.buffer[ring_buffer.head];

    if (c == CAN) {
        num_cancels++;
        if (num_cancels == 5) {
            /* The other side is serious about this. Just shut up, clean up and exit. */
            cleanup();
            exit(CAN);
        }
    } else num_cancels = 0;
    return c;
}
