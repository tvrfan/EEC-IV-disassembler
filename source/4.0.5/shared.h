

/**************************************************************************
* structs and defines for shared interface and global scope go here
* minimal declares to keep interface clean - called by sign, main and core.
* also refer to sign.h and core.h for more specific declares
*
* NOTE ---  
* This code assumes INT and UNSIGNED INT is at least 32 bits.
* Win32 Codeblocks (mingw)     int = long = void* = 32 bits.
* amd64 CodeBlocks (linux-gcc) int = 32 bits, long = void* = 64 bits.
*  source code uses the define "__linux__" to be present for linux build 
*  and "__MINGW32__" for Windows build (for CodeBlocks mingw)
*  and "__MINGW64__" for 64 Windows builds -- NOT IMPLEMENTED YET
**************************************************************************/ 

#ifndef _XSHARX_H

#define _XSHARX_H 1

#include  <stdio.h>

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;


// True banks may be anywhere 0-15,although only 0,1,8,9 used
// Bank 16 is used for RAM data, sized at 0x2000 bytes

#define BMAX      16             // maximum bank index

#define SADVERSION "4.0.5"
#define SADDATE    "25/02/2020"

// debug defines - when switched on, this causes a LOT of output to _wrn file
// DBGPRT is kept to make debug code more obvious to view
// DBG_xxx subrs are also for debug printout and ignored if XDBGX not defined

 #define XDBGX


//  file order is (xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, SAD.ini, xx_dbg.txt)



typedef struct
{
 FILE *fl[7];          // file handles
 uint fillen;          // length of main binary file
 int error;            // file read error

 char bare[64];        // bare file name (root binary name)
 char path[254];       // path of root binary name
 char dpath[256];      // default path (SAD progam/exe location)
 char fn[7][256];      // full file names with path

} HDATA;


// for file permissions ....win vs. Linux

typedef struct
{
 const char *suffix;
 const char *fpars;
} HOP;


// this may be overridden by user commands....

typedef struct                 // indexed by REAL bank id (0-16)
 {
 uchar  *fbuf;                // bin file data pointer for bank
 uint   *opbt;                // bit array pointer.
 
 int   filstrt;               // start FILE OFFSET. ( = real offset - PCORG can be -ve)
 int   filend;                // end   FILE OFFSET. 
 uint  minpc;                 // min PC (normally PCORG)
 uint  maxpc;                 // end PC (= max PC allowed (where fill starts)
 uint  maxbk;                 // where bank really ends
 uint  bok     : 1 ;          // this bank is valid to use
 uint  cmd     : 1 ;          // set by command, only used in set_bnk
 int   dbnk    : 5 ;          // destination bank (setup only...also temp 8065 marker)
 uint  btype   : 3 ;          // bank type (code start or not) only ever 1 or 2
 }  BANK;


#endif

