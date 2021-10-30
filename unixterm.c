/*
 * This file implement these functions for UNIX:
 *
 * 1. Terminal setup and tear down
 * 2. Flushing the transmit and receive channels.
 * 3. "Raw" transmit and receive on stdout and stdin.
 * 4. Timeout handling when receiving.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "zmdm.h"
#include "zmodem.h"

extern int last_sent;

/*
 * routines to make the io channel raw and restore it
 * to its normal state.
 */

struct termios old_termios;

void fd_init(void) {
    struct termios t;

    tcgetattr(0, &old_termios);

    tcgetattr(0, &t);

    t.c_iflag = 0;

    t.c_oflag = 0;

    t.c_lflag = 0;

    t.c_cflag |= CS8;

    tcsetattr(0, TCSANOW, &t);
}

void fd_exit(void) { tcsetattr(0, TCSANOW, &old_termios); }

/*
 * read bytes as long as rdchk indicates that
 * more data is available.
 */

void rx_purge(void) {
    struct timeval t;
    fd_set f;
    unsigned char c;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(0, &f);

    while (select(1, &f, NULL, NULL, &t)) {
        read(0, &c, 1);
    }
}

/*
 * send the bytes accumulated in the output buffer.
 */

void tx_flush(void) { fflush(stdout); }

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

    putchar(c);
}

/*
 * receive any style header within timeout milliseconds
 */

void alrm(int a) { signal(SIGALRM, SIG_IGN); }

int rx_poll(void) {
    struct timeval t;
    fd_set f;

    t.tv_sec = 0;
    t.tv_usec = 0;

    FD_ZERO(&f);
    FD_SET(0, &f);

    if (select(1, &f, NULL, NULL, &t)) {
        return 1;
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
    unsigned char c;
    static int n_cans = 0;

    if (n_in_inputbuffer == 0) {
        /*
         * change the timeout into seconds; minimum is 1
         */

        timeout /= 1000;
        if (timeout == 0) {
            timeout++;
        }

        /*
         * setup an alarm in case io takes too long
         */

        signal(SIGALRM, alrm);

        timeout /= 1000;

        if (timeout == 0) {
            timeout = 2;
        }

        alarm(timeout);

        n_in_inputbuffer = read(0, inputbuffer, 1024);

        if (n_in_inputbuffer <= 0) {
            n_in_inputbuffer = 0;
        }

        /*
         * cancel the alarm in case it did not go off yet
         */

        signal(SIGALRM, SIG_IGN);

        if (n_in_inputbuffer < 0 && (errno != 0 && errno != EINTR)) {
            fprintf(stderr, "zmdm : fatal error reading device\n");
            exit(1);
        }

        if (n_in_inputbuffer == 0) {
            return TIMEOUT;
        }

        inputbuffer_index = 0;
    }

    c = inputbuffer[inputbuffer_index++];
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
