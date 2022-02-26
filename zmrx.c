/******************************************************************************/
/* Project : Unite!       File : zmodem receive        Version : 1.02         */
/*                                                                            */
/* (C) Mattheij Computer Service 1994                                         */
/*                                                                            */
/* contact us through (in order of preference)                                */
/*                                                                            */
/*   email:          jacquesm@hacktic.nl                                      */
/*   mail:           MCS                                                      */
/*                   Prinses Beatrixlaan 535                                  */
/*                   2284 AT  RIJSWIJK                                        */
/*                   The Netherlands                                          */
/*   voice phone:    31+070-3936926                                           */
/******************************************************************************/

#include "version.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fileio.h"
#include "zmdm.h"
#include "zmodem.h"

#pragma printf = "%c %s %d %8ld"        // enables %c, %s, %d, %ld only

FILE *fp = NULL;     /* fp of file being received or NULL */
long mdate;          /* file date of file being received */
char filename[0x80]; /* filename of file being received */
char *name; /* pointer to the part of the filename used in the actual open */

extern int use_aux;
int opt_v = FALSE; /* show progress output */
int opt_d = FALSE; /* show debug output */
int opt_q = FALSE;
int junk_pathnames = FALSE; /* junk incoming path names or keep them */
unsigned char rx_data_subpacket[8192]; /* zzap = 8192 */

long current_file_size;
time_t transfer_start;

/*
 * show the progress of the transfer like this:
 * zmrx: receiving file "garbage" 4096 bytes ( 20%)
 * avoids the use of floating point.
 */

void show_progress(char *progress_fname, FILE *progress_fp)

{
    long percentage;

    time_t duration;
    long cps;

    if (current_file_size > 0) {
        percentage = ((long)ftell(progress_fp) * 100) / current_file_size;
    } else {
        percentage = 100;
    }

    duration = time(NULL) - transfer_start;

    if (duration == 0l) {
        duration = 1;
    }

    cps = (long)ftell(progress_fp) / duration;

    fprintf(stderr,
            "zmrx: receiving file \"%s\" %8ld bytes (%3ld %%/%5ld cps)\r",
            progress_fname, ftell(progress_fp), percentage, cps);
}

/*
 * receive a header and check for garbage
 */

/*
 * receive file data until the end of the file or until something goes wrong.
 * the receive_fname is only used to show progress
 */

int receive_file_data(char *receive_fname, FILE *receive_fp)

{
    long pos;
    int n;
    int type;

    /*
     * create a ZRPOS frame and send it to the other side
     */

    tx_pos_header(ZRPOS, ftell(receive_fp));

    /*	fprintf(stderr,"re-transmit from %d\n",ftell(receive_fp));
     */
    /*
     * wait for a ZDATA header with the right file offset
     * or a timeout or a ZFIN
     */

    do {
        do {
            type = rx_header(10000);
            if (type == TIMEOUT) {
                return TIMEOUT;
            }
        } while (type != ZDATA);

        long pos0 =  (long) rxd_header[ZP0];
        long pos1 = ((long) rxd_header[ZP1]) << 8;
        long pos2 = ((long) rxd_header[ZP2]) << 16;
        long pos3 = ((long) rxd_header[ZP3]) << 24;

        pos = pos0 | pos1 | pos2 | pos3;
    } while (pos != ftell(receive_fp));

    do {
        type = rx_data(rx_data_subpacket, &n);

        /*		fprintf(stderr,"packet len %d type %d\n",n,type);
         */
        if (type == ENDOFFRAME || type == FRAMEOK) {
            fwrite(rx_data_subpacket, 1, n, receive_fp);
        }

        if (opt_v) {
            show_progress(receive_fname, receive_fp);
        }

    } while (type == FRAMEOK);

    return type;
}

void tx_zrinit()

{
    unsigned char zrinit_header[] = {
        ZRINIT, 0, 0, 0, 4 | ZF0_CANFDX | ZF0_CANOVIO | ZF0_CANFC32};

    tx_hex_header(zrinit_header);
}

/*
 * receive a file
 * if the file header info packet was garbled then send a ZNAK and return
 * (using ZABORT frame)
 */

void receive_file()

{
    long size;
    int type;
    int l;
    int clobber = FALSE;
    int protect = FALSE;
    int newer = FALSE;
    const char *mode = "wb";

    /*
     * fetch the management info bits from the ZRFILE header
     */

    /*
     * management option
     */

    if (management_protect || (rxd_header[ZF1] & ZF1_ZMPROT)) {
        protect = TRUE;
    } else {
        if (management_clobber || (rxd_header[ZF1] & ZF1_ZMCLOB)) {
            clobber = TRUE;
        }
    }

    if (management_newer || (rxd_header[ZF1] & ZF1_ZMNEW)) {
        newer = TRUE;
    }

    /*
     * read the data subpacket containing the file information
     */

    type = rx_data(rx_data_subpacket, &l);

    if (type != FRAMEOK && type != ENDOFFRAME) {
        if (type != TIMEOUT) {
            /*
             * file info data subpacket was trashed
             */
            tx_znak();
        }
        return;
    }

    /*
     * extract the relevant info from the header.
     */

    strcpy(filename, (const char *)rx_data_subpacket);

    if (junk_pathnames) {
        name = strrchr(filename, '/');
        if (name != NULL) {
            name++;
        } else {
            name = filename;
        }
    } else {
        name = filename;
    }

    if (opt_v) {
        fprintf(stderr, "zmrx: receiving file \"%s\"\r", name);
    }

    sscanf((char *)(rx_data_subpacket +
                          strlen((char *)rx_data_subpacket) + 1),
           "%ld %lo", &size, &mdate);

    current_file_size = size;

    /*
     * decide whether to transfer the file or skip it
     */

    /* Returns 0 of the file does not exists; modification time otherwise */
    long existing_file_modification_time = fileio_get_modification_time(name);

    /*
     * if the file already exists here the management options need to
     * be checked..
     */
    if (existing_file_modification_time != 0) {
        if (mdate == existing_file_modification_time) {
            /*
             * this is crash recovery
             */
            mode = "ab";
        } else {
            /*
             * if the file needs to be protected then exit here.
             */
            if (protect) {
                tx_pos_header(ZSKIP, 0L);
                return;
            }
            /*
             * if it is not ok to just overwrite it
             */
            if (!clobber) {
                /*
                 * if the remote file has to be newer
                 */
                if (newer) {
                    if (mdate < existing_file_modification_time) {
                        fprintf(stderr,
                                "zmrx: file '%s' skipped because local file "
                                "is newer.\n",
                                name);
                        tx_pos_header(ZSKIP, 0L);
                        /*
                         * and it isn't then exit here.
                         */
                        return;
                    }
                }
            }
        }
    }

    /*
     * transfer the file
     * either not present; remote newer; ok to clobber or no options set.
     * (no options->clobber anyway)
     */

    fp = fopen(name, mode);

    if (fp == NULL) {
        tx_pos_header(ZSKIP, 0L);
        if (opt_v) {
            fprintf(stderr, "zmrx: can't open file '%s'\n", name);
        }
        return;
    }

    transfer_start = time(NULL);

    while (ftell(fp) != size) {
        type = receive_file_data(filename, fp);
        if (type == ZEOF) {
            break;
        }
    }

    /*
     * wait for the eof header
     */

    while (type != ZEOF) {
        type = rx_header_and_check(10000);
    }

    /*
     * close and exit
     */

    fclose(fp);

    fp = NULL;

    fileio_set_modification_time(name, mdate);

    /*
     * and close the input file
     */

    if (opt_v) {
        fprintf(stderr, "zmrx: received file \"%s\"                 \n", name);
    }
}

void cleanup(void)

{
    if (fp) {
        fflush(fp);
        fclose(fp);
        /*
         * set the time (so crash recovery may work)
         */
        fileio_set_modification_time(name, mdate);
    }

    fd_exit();
}

void usage(void)

{
    printf("zmrx %s %s (C) Mattheij Computer Service 1994\n", VERSION, VERSION_DATE);
    printf("    CP/M port by Rob Gowin with help from Andrew Lynch.");
    printf("usage : zmrx options\n");
    printf("    -x n        n=0: use console for transfers (default)\n");
    printf("                n=1: use aux device for transfers\n");
    printf("    -j          junk pathnames\n");
    printf("    -n          transfer if source is newer\n");
    printf("    -o          overwrite if exists\n");
    printf("    -p          protect (don't overwrite if exists)\n");
    printf("\n");
    printf("    -d          debug output\n");
    printf("    -v          verbose output\n");
    printf("    -q          quiet\n");
    printf("    (only one of -n -o or -p may be specified)\n");

    cleanup();

    exit(1);
}

int main(int argc, char **argv)

{
    int i;
    int type;

    char ch;
    int have_error = FALSE;
    use_aux = FALSE;
    while ((ch = getopt(argc, argv, "DJX:NOPVQ")) != -1) {
        switch (ch) {
            case 'D':
                opt_d = TRUE;
                break;
            case 'J':
                junk_pathnames = TRUE;
                break;
            case 'X':
                if (validate_device_choice(optarg[0])) {
                    if (optarg[0] == '0') use_aux = FALSE;
                    else if (optarg[0] == '1') use_aux = TRUE;
                } else have_error = TRUE;
                break;
            case 'N':
                management_newer = TRUE;
                break;
            case 'O':
                management_clobber = TRUE;
                break;
            case 'P':
                management_protect = TRUE;
                break;
            case 'Q':
                opt_q = TRUE;
                break;
            case 'V':
                opt_v = TRUE;
                break;
            case '?':
                printf("zmtx: bad option '-%c'\n", optopt);
                have_error = TRUE;
                break;
        }
    }

    if (have_error) usage();

    argc -= optind;
    argv += optind;

    if (opt_d) {
        opt_v = TRUE;
    }

    if (opt_q) {
        opt_v = FALSE;
        opt_d = FALSE;
    }

#if 0
	if (!opt_v) {
		freopen("/usr/src/utils/zmnew/trace","w",stderr);
		setbuf(stderr,NULL);
	}
#endif

    if ((management_newer + management_clobber + management_protect) > 1 || argc != 0) {
        usage();
    }

    /*
     * set the io device to transparent
     */

    fd_init();

    /*
     * establish contact with the sender
     */

    if (opt_v) {
        fprintf(stderr, "zmrx: establishing contact with sender\n");
    }

    /*
     * make sure we dont get any old garbage
     */

    rx_purge();

    /*
     * loop here until contact is established.
     * another packet than a ZRQINIT should be received.
     */

    i = 0;
    do {
        i++;
        if (i > 10) {
            fprintf(stderr, "zmrx: can't establish contact with sender\n");
            cleanup();
            exit(3);
        }

        tx_zrinit();
        type = rx_header(7000);
    } while (type == TIMEOUT || type == ZRQINIT);

    if (opt_v) {
        fprintf(stderr, "zmrx: contact established\n");
        fprintf(stderr, "zmrx: starting file transfer\n");
    }

    /*
     * and receive files
     * (other packets are acknowledged with a ZCOMPL but ignored.)
     */

    do {
        if (type == ZFILE)
            receive_file();
        else
            tx_pos_header(ZCOMPL, 0L);

        do {
            tx_zrinit();

            type = rx_header(7000);
        } while (type == TIMEOUT);
    } while (type != ZFIN);

    /*
     * close the session
     */

    if (opt_v) {
        fprintf(stderr, "zmrx: closing the session\n");
    }

    {
        unsigned char zfin_header[] = {ZFIN, 0, 0, 0, 0};

        tx_hex_header(zfin_header);
    }

    /*
     * wait for the over and out sequence
     */

    {
        int c;
        do {
            c = rx_raw(0);
        } while (c != 'O' && c != TIMEOUT);

        if (c != TIMEOUT) {
            do {
                c = rx_raw(0);
            } while (c != 'O' && c != TIMEOUT);
        }
    }

    if (opt_d) {
        fprintf(stderr, "zmrx: cleanup and exit\n");
    }

    cleanup();

    return 0;
}
