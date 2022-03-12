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
extern int last_sent;

const int AUX_DEV = 10;
const int CONSOLE_DEV = 2;

int fd_init_done = 0;

#ifdef DEBUG
extern int raw_trace;

#endif

_IOREC save,     /* the original Iorec is saved here for the duration of this process */
     *savep;     /* ptr returned by Iorec() */

char iobuf[IBUFSIZ]; /* RS232 receive buffer. */

/*
 * routines to make the io channel raw and restore it
 * to its normal state.
 *
 * No-ops on the Atari ST.
 */

int16_t has_bconmap(void)
{
    return (0L == Bconmap(0));
}

void fd_init(void) {

    if (!has_bconmap()) {
        fprintf(stderr, "BCONMAP not supported. That is unexpected.\r\n");
        return;
    }

    /* Map in our device to that Iorec and Rsconf act on it. */
    int32_t old_bconmap_dev = Bconmap(AUX_DEV);

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

    Bconmap(old_bconmap_dev);

    fd_init_done = 1;
}

void fd_exit(void)
{
    if (!has_bconmap()) {
        fprintf(stderr, "BCONMAP not supported. That is unexpected.\r\n");
        return;
    }

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
    int32_t old_bconmap_dev = Bconmap(AUX_DEV);
    /* Turn off hardware flow control. There's no way to restore previous value. */
    Rsconf(-1, 0, -1, -1, -1, -1);

    Bconmap(old_bconmap_dev);

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
void rd_time() {
    present_time = *hz_200;
}

/*
 * alarm() - set the alarm time
 */
void stalarm(unsigned int n) /* n is number of seconds, roughly */
{
    if(n > 0)
    {
        Supexec(rd_time);
        /* We really need n * 200 but n * 256 is close enough */
        alrm_time = present_time + ( n << 8 );
    }
    else
        alrm_time = 0L;
}

int read_modem(unsigned char *buf, int count) {
    int n = 0;

    while (1) {

        if (AUX_DEV != CONSOLE_DEV) {
            if (Bconstat(CONSOLE_DEV)) {
                if ((Bconin(CONSOLE_DEV) & 0x7f) == 0x3) { /* is console char a CTRL-C? */
                    cleanup();
                    exit(CAN);
                }
            }
        }

        if (Bconstat(AUX_DEV)) {
            /* Character available. Read it. */
            n++;
            *buf++ = Bconin(AUX_DEV);

            /* Read as many characters as available, but don't exceed buffer size. */
            while ((n < count) && Bconstat(AUX_DEV)) {
                *buf++ = Bconin(AUX_DEV);
                n++;
            }
            return n;
        }

        /* Check got a time out if required. */
        if (alrm_time != 0) {
            Supexec(rd_time);
            if (present_time > alrm_time) {
                /* timeout */
                longjmp(tohere, -1);
            }
        }
    }
}

unsigned char inputbuffer[IBUFSIZ];
size_t n_in_inputbuffer = 0;
int inputbuffer_index;

/*
 * rx_raw ; receive a single byte from the line.
 * reads as many are available and then processes them one at a time
 * check the data stream for 5 consecutive CAN characters;
 * and if you see them abort. this saves a lot of clutter in
 * the rest of the code; even though it is a very strange place
 * for an exit. (but that was wat session abort was all about.)
 */

/* inline */
int rx_raw(int timeout)
{
    static int n_cans = 0;

    if (n_in_inputbuffer == 0) {
        /*
         * change the timeout into seconds; minimum is 1
         */

        int timeout_secs = timeout / 1000;
        if (timeout_secs == 0) {
            timeout_secs = 2;
        }

        /*
          * setup an alarm in case io takes too long
         */

        if (setjmp(tohere)) {
            n_in_inputbuffer = 0;
            return TIMEOUT;
        }

        stalarm(timeout_secs);
        n_in_inputbuffer = read_modem(inputbuffer,IBUFSIZ);
        stalarm(0);

        if (n_in_inputbuffer <= 0) {
            n_in_inputbuffer = 0;
        }

        /*
         * Does ST support an errno concept?
        if (n_in_inputbuffer < 0 && (errno != 0 && errno != EINTR)) {
            fprintf(stderr,"zmdm : fatal error reading device\n");
            exit(1);
        }
        */
        if (n_in_inputbuffer == 0) {
            return TIMEOUT;
        }

        inputbuffer_index = 0;
    }

    unsigned char c = inputbuffer[inputbuffer_index++];
    n_in_inputbuffer--;

    if (c == CAN) {
        n_cans++;
        if (n_cans == 5) {
            /*
             * the other side is serious about this. just shut up;
             * clean up and exit.
             */
            cleanup();

            exit(CAN);
        }
    }
    else {
        n_cans = 0;
    }
    return c;
}
