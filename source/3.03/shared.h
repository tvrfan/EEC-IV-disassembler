
// structs for shared interface go here
// minimal declares to keep interface clean - called by distest1 and core.
// THIS is latest common version  28/07/14


#ifndef _XSHARX_H

#define _XSHARX_H 1

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

// real banks may be anywhere 0-15, 16 used for RAM data, sized at 0x2000 bytes

#define BMAX      16             // maximum bank index

#define SADVERSION "3.03"

//  file order is (xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, SAD.ini)


typedef struct
{
 FILE *fl[6];          // file handles
 int fillen;           // length of main binary file
 int error;            // file read error

 char bare[64];       // bare file name (root binary name)
 char path[254];       // path of root binary name
 char dpath[256];      // default path (SAD progam/exe location)
 char fn[6][256];      // full file names with path

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
 
 int     filstrt;             // start FILE OFFSET. ( = real offset - PCORG)
 int     filend;              // end   FILE OFFSET. 
 int     minpc;               // min PC (normally PCORG)
 int     maxpc;               // end PC (= max PC allowed (where fill starts)
 int     maxbk;               // where bank really ends
 uint    bok   : 1 ;          // this bank is valid to use
 uint    cmd   : 1 ;          // set by command
 uint    dbnk  : 4 ;          // destination bank (setup only...)
 
 uint    btype : 3 ;          // bank type (code start or not) why 3 bits ?
 }  BANK;


// define letters as bitmask of a 'int' (32 bit)

#define A  0x1
#define B  0x2
#define C  0x4
#define D  0x8
#define E  0x10
#define F  0x20
#define G  0x40
#define H  0x80
#define I  0x100
#define J  0x200
#define K  0x400
#define L  0x800
#define M  0x1000
#define N  0x2000
#define O  0x4000
#define P  0x8000
#define Q  0x10000
#define R  0x20000
#define S  0x40000
#define T  0x80000
#define U  0x100000
#define V  0x200000
#define W  0x400000
#define X  0x800000
#define Y  0x1000000
#define Z  0x2000000

#endif

