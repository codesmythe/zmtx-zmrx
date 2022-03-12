## ZMRX/ZMTX

This repository is a modernization of a mid 1990's implementation of the ZMODEM protocol called 'zmtx-zmrx'. The main goal is to have a version of ZMODEM that will run standalone on TOS/EmuTOS for the Atari ST and also various flavors of CP/M-80.

This is currently a work-in-progress. The executables work, but transfer speeds on receive need a lot of improvement.

## BUILDING

### UNIX

You can build for Unix/Linux by just using make. However, this version is not tested.

### CP/M

You'll need to have [Z88DK](https://z88dk.org/site) available. Ensure that `zcc` is in your path. Then to build, do:

    $ make -f makefile.cpm

or

    $ make -f makefile.cpm.8085

for the 8085 version.  The binaries, ZMRX.COM and ZMTX.COM, will work on CP/M 2.2 and CP/M 3. 

### Atari ST (TOS/EmuTOS)

You'll need to have [m68k-atari-mint cross-tools](http://vincent.riviere.free.fr/soft/m68k-atari-mint) installed and available on your PATH. You'll also need [CMake](https://cmake.org) installed. Adjust `toolchain_file.txt` for the paths to your compiler install.

After cloning this repository, you need to clone and build the libcmini submodule:

    $ git submodule init
    $ git submodule update
    $ (cd libcmini; make)

Then, make a build directory and run CMake:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain_file.txt ..

And finally from the build directoy, do the make:

    $ make

This will create the `zmrx.ttp` and `zmtx.ttp` executables in the build directory.    
## USAGE

### Receiving via ZMRX.COM

To receive files to TOS or CP/M from a PC, use ZMRX.COM. It does not take any file parameters, but accepts these command line options:

| Option                     | Description                                                  |
| -------------------------- | ------------------------------------------------------------ |
| -x n (not support for TOS) | When n=0, use the console for transfers (default).<br />When n=1, use the AUX device for transfers (not supported for CP/M 2.2) |
| -j                         | Junk pathnames.                                              |
| -n                         | Transfer only if source is newer (not currently implemented for CP/M). |
| -o                         | Overwrite if file exists.                                    |
| -p                         | Protect (don't overwrite if file exists).                    |
| -d                         | Enable debug output                                          |
| -v                         | Enable verbose output                                        |
| -q                         | Quiet; don't print anything.                                 |

 Only one of -n, -o or -p may be specified.

### Transmitting via ZMTX.COM

To transmit files from TOS or CP/M to a PC, use ZMTX.COM. This program requires at least one file name or wildcard patterns. More than one file name or wildcard expansion can be given on the command, but the maximum number of files that can be transferred in one invocation is 512 files.

For example:

    B>zmtx A:*.COM B:ZP*.* S*.* 

will transmit files on the A drive that match \*.COM, files on B: that match ZP\*.\*, and files on the current drive that match S\*.\* . File names can be lower or upper case.

ZMTX.COM accepts these command line options:

| Option                       | Description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| -x n (not supported for TOS) | When n=0, use the console for transfers (default).<br />When n=1, use the AUX device for transfers (not supported for CP/M 2.2) |
| -n                           | Transfer only if source is newer (not currently implemented for CP/M). |
| -o                           | Overwrite if file exists.                                    |
| -p                           | Protect (don't overwrite if file exists).                    |
| -d                           | Enable debug output                                          |
| -v                           | Enable verbose output                                        |
| -q                           | Quiet; don't print anything.                                 |

As with ZMRX.COM,  only one of -n, -o or -p may be specified.

## MOTIVATION AND HISTORY

Though Chuck Fosberg's ZMODEM is way past it's prime from the prospective of the 2020s, there's still a need for a decent ZMODEM implementation for retro machines such as the Atari ST and CP/M machines.

As mentioned at the end of this README, however, ZMODEM's orignal source code is a product of its time (mid 1980s), and difficult to work with in the modern era.

In the mid 1990s, Jacques Mattheij wrote a re-implementation of the ZMODEM protocol and released version 1.0 of zmtx-zmrx to comp.os.sources. The code in this repository is based on Version 1.02 of his code, as [obtained](https://ftp.openbsd.org/pub/OpenBSD/distfiles/zmtx-zmrx-1.02.tar.gz) from the OpenBSD distfiles repository. Mattheij's code is WAY a better starting place for a ZMODEM implementation.

In 2015, Mattheij 'deeded' his code to the [SyncTerm](https://syncterm.bbsdev.net) BBS terminal program, where it served as the basis for that program's ZMODEM code.

Since SyncTerm uses the GPL v2.0 license, that's the license I've assigned to this code base.

## README FROM ORIGINAL zmtx-zmrx-1.02.tar.gz

[Lightly edited for readability.]

This file was abstracted from the original zmodem.doc after one night I decided I had enough of all this marketing hype and wanted to know what zmodem was all about without having to wade hip-deep through advertising slogans.  This text is © MCS 1994.

The intended audience of this document are programmers looking for a compact reference text on how zmodem works and what you should know to be able to implement conforming zmodem send and receive software. This is definitely not an 'end user' document and the examples and data structures are strongly biased towards the 'C' language. (What? Are there other languages?)

A lot of work went into the preparation of this document; although I'm afraid I can't guarantee it's correctness. Jacques Mattheij or MCS shall not be liable for any damages whatsoever. This file is provided 'as is' for no fee whatsoever. Use is at your own risk. If these terms are unacceptable to you then delete these files NOW!

Changes relative to zmodem.doc as provided by various sources:

  - Removal of all historical information

  - Removal of all plugs relating to omen technology products etc. if something is public domain then leave it at that

  - Removal of all 'poetry'

  - Removal of all references to xmodem ymodem kermit and so on

  - Removal of all overstrike typesetting tricks which make this file practically uneditable and unviewable

  - Removal of a lot of irrelevant but nice facts about the weather and some other nice subjects for conversation

  - Removal of all implementation specific details referring to those antiques of telecom 'rz' and 'sz'

  - Manifest constants added in the text

  - Moved footnotes to the appropriate place in the text

  - Changed number base from octal to hex (welcome to the nineties). Admittedly it looks less ivory tower but it reads a lot easier for those who started programming after 1959

Some recomendations...

A lot has changed since the original zmodem came out. Not so much in the protocol as well as in the world around it. I would like to de-advertise several of zmodem's advanced features:

  - Command sending. This is the hackers dream come true. a formalized backdoor into any site supporting this file transfer protocol with a relatively easy defeated security mechanism. Don't implement it; Just refuse it.

  - File translation. The zmodem protocol specification below contains a number of facilities to change a file between one os and the next. THIS IS NOT THE PLACE! A file TRANSFER protocol should do just that and with a minimum of fuss. If you have to start worrying about whether zmodem just garbled that 4MB zip file of yours just downloaded from the states at $1 a minute you're ready for some agression. Another point may be that the file size will change which may give rise to a lot of bugs in zmodem implementations on the far side of wherever you are downloading to / from. Stick to binary. It helps.

  - Use CRC32 and not CRC16. Apart from the obvious (better error detection) the original CRC16 implementation is buggy.

  - Do not send the serial number in the ZFILE frame. This is not a very useful function.

  - In many places in the orignal zmodem.doc it was suggested that if this or that failed you should step down and attempt a ymodem transfer. Don't do that! Users know pretty good what they want and if they specify a zmodem transfer give them one or give them nothing. Don't try to be smart. Probably something is wrong and it is better to exit with some informative message than to go ahead with the wrong protocol; Apart from keeping your source clean.

  - Don't use or implement the run length encoding. It is greatly hampered by not checking if run length encoding is needed. If you specify that ability you're stuck with it. Nowadays with zip 2.0/unzip 5.0 and better there is absolutely no need for a file transfer protocol to busy itself with compression. For $200 you can buy a mnp class 10 modem which does all that and more completely transparently without possibly triggering a host of bugs in a relatively little exercised part of your hosts software.

  - LWZ encoding; See run length encoding.

  - Don't use the 'ZXSPARSE' option. Chances of finding a system that implements it are small and even then 10 : 1 that the file will be sent compressed.

  - Don't send the 'rz\r' used in the original documentation. This is a very nasty way of making a public domain protocol
    dependant on a company. (Sorry had to abide by that in the end; some implementations trigger auto downloads on this.)

In general; keep it simple! Stick to multiple file binary transfers and try to get some speed out of those boxes. Time is spent well on optimizing and cleaning your source rather than on some obscure seldomly used feature which will clutter your code.

Whenever I give an example of how not to program in 'C' I refer to the rz.c and sz.c sources. In more than one way these are true 'classics'. If you intend to implement zmodem don't bother with these dinosaurs (to use a popular term); better to write it clean from the start.
