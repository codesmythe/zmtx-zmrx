
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>

#include "zmdm.h"
#include "zmodem.h"

#include "cpm.h"

extern int last_sent;

int use_aux = FALSE; /* Set by command line parser in main(). */

/* BIOS functions for send/receive via the console device. */
#define CONST 2
#define CONIN 3
#define CONOUT 4

/* BIOS functions for send/receive via the auxillary device. */
#define AUXOUT  6
#define AUXIN   7
#define AUXIST 18

/* Use the console by default. */
int bios_char_avail = CONST;
int bios_write_char = CONOUT;
int bios_read_char  = CONIN;

void setup_devices() {
    bios_char_avail = use_aux ? AUXIST : CONST;
    bios_write_char = use_aux ? AUXOUT : CONOUT;
    bios_read_char  = use_aux ? AUXIN  : CONIN;
}

int console_char_available(void) { return bios(bios_char_avail, 0, 0) != 0; }

void write_console(char ch) { bios(bios_write_char, ch, 0); }

int read_console(void) { return bios(bios_read_char, 0, 0); }

/*
 * routines to make the io channel raw and restore it
 * to its normal state.
 *
 */

void fd_init(void) {
    setup_devices();
}

void fd_exit(void) {}

/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void rx_purge(void) {
    while (console_char_available())
        read_console();
}

/*
 * send the bytes accumulated in the output buffer.
 */

void tx_flush(void) { /* Not supported for CP/M. */ }

/*
 * transmit a character.
 * this is the raw modem interface
 */

void tx_raw(int c) {
#ifdef DEBUG
    if (raw_trace) {
        fprintf(stderr, "%02x ", c);
    }
#endif

    last_sent = c & 0x7f;

    write_console(c);
}

int rx_poll(void) { return console_char_available(); }

static unsigned long delay_max = 0;

int read_modem(unsigned char *buf, int count) {
    int n = 0;
    unsigned long delay = 0;

    while (1) {

        if (console_char_available()) {
            /* Character available. Read it. */
            n++;
            *buf++ = read_console();

            /* Read as many characters as available, but don't exceed buffer
             * size. */
            while ((n < count) && console_char_available()) {
                *buf++ = read_console();
                n++;
            }
            return n;
        }
        /* Check got a time out if required. */
        if (delay_max != 0) {
            delay++;
            if (delay > delay_max)
                break;
        }
    }
    return 0;
}

unsigned char inputbuffer[1024];
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
int rx_raw(int timeout) {
    static unsigned long delay_factor = 20000;
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

        delay_max = timeout_secs * delay_factor;
        n_in_inputbuffer = read_modem(inputbuffer, 1024);
        delay_max = 0;

        if (n_in_inputbuffer <= 0) {
            n_in_inputbuffer = 0;
        }

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
    } else {
        n_cans = 0;
    }
    return c;
}

int check_user_abort(void)
{
    return 0;
}
