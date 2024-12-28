

/**************************************************************************
* defines and declarations for shared interface and global scope go here
* minimal declares to keep interface clean - called by sign, main and core.
* also refer to sign.h and core.h for more specific declares
*
* NOTE This code assumes INT and UNSIGNED INT are at least 32 bits (4 bytes).
* and WILL NOT WORK with 16 bit compilers.
* Sizes here (in bits) for information for different platforms
*
* OS     compiler     int     float   long   void*    double
* Win32  mingw        32      32      32     32       64
* Linux  amd64-gcc    32      32      64     64       64
*
* This code built with CodeBlocks and/or CodeLite IDE environments
* on both Win and Linux
**************************************************************************/

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <sys/stat.h>
#include  <stdarg.h>
#include  <string.h>
#include  <math.h>

#ifndef _XSHARX_H

#define _XSHARX_H 1

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;


// Banks can be 0-15, only 0,1,8,9 used. All +1 internally = 1,2,9,A
// All registers in Bank 8,  4 banks of 64k are malloced.


#define BMAX      16             // maximum bank index

#define SADVERSION "4.0.13.1"
#define SADDATE    "28 Dec 2024"

// debug defines - when switched on, this causes a LOT of output to xx_dbg file
// DBGPRT id is kept to make debug code more obvious to view

// #define XDBGX

#define NC(x)  sizeof(x)/sizeof(x[0])     // no of cells in struct 'x'

//  file order is (xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, xx_dbg.txt, SAD.ini)

typedef struct
{
 FILE *fl[7];          // file handles
 uint fillen;          // length of main binary file
 int error;            // file read error

 char bare [64];        // bare file name (root binary name)
 char path [254];       // path of root binary name
 char dpath[254];      // default path (SAD progam/exe location)
 char fn[7][256];      // full file names with path

} HDATA;


// for preset file permissions ....windows vs. Linux

typedef struct
{
 const char *suffix;
 const char *fpars;
} HOP;


// bank holder, details & related pointers.

typedef struct                // indexed by bank num (0-16)
 {
 uchar  *fbuf;                // bin file (copy) for this bank ptr.
 uint   *opcbt;               // opcode start flags for this bank (bit array ptr).

 uint  filstrt;                // start FILE OFFSET. ( = real offset, can be negative)
 uint  filend;                // end   FILE OFFSET.
 uint  minpc;                 // min PC (normally PCORG)
 uint  maxpc;                 // end PC (= max PC allowed (where fill starts)
 uint  maxbk;                 // where bank really ends (after fill)
 uint  bprt     : 1 ;         // print this bank in msg file
 uint  bok      : 1 ;         // this bank is valid (and found flag)
 uint  cmd      : 1 ;         // set by command
 uint  cbnk     : 1 ;         // code start bank ( = 8 )
 uint  P65      : 1 ;         // this is an 8065 bank
 uint  bmissing : 2 ;         // missing bytes affect filstart
 uint  bkmask   : 4 ;         // possible banks as bitmask (for 'find')
 uint  dbank    : 4 ;         // destination bank as number
 }  BANK;


#endif

