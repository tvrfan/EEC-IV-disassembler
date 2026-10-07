

/**************************************************************************
* defines and declarations for shared and global scope go here.
* called by all '.c' modules.
* also refer to sign.h,core.h, debug.h, etc. for specific/local declares
*
* BUILD NOTES
*
* This code assumes INT and UNSIGNED INT are at least 32 bits (4 bytes).
* It will FAIL with 16 bit compilers. (SAD uses 20 bit addresses for bin files)

* Sizes here (in bits) for information for different platforms
*
* OS       Compiler     int     float   long   void*    double
* Windows  mingw32      32      32      32     32       64
* Linux    amd64-gcc    32      32      64     64       64
*
* 806x bit numbering - bit 0 is least significant (rightmost),
* bit 7 (15 etc) is most significant.
*
* This code built with CodeLite IDE environments on Windows and Linux.
* Other IDES (e.g. CodeBlocks) probably fine too.
*
**************************************************************************/

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <sys/stat.h>
#include  <stdarg.h>
#include  <string.h>
#include  <math.h>                 // for floating, Nan etc

// #include <wchar.h>             //  for extended char.......
// #include  <time.h>             //  if printing date and time

#ifndef _XSHARX_H

#define _XSHARX_H 1

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef const char     cchar;


// Banks can be 0-15, only 0,1,8,9 used.
// Banks are set as +1 internally = 1,2,9,A to allow easy handling of 'no bank' in addresses.
// 4 banks of 64k are malloced.

// All CPU registers are 'no bank' and stored in Bank 8 (9 internally).

// All addresses, except registers, expect bank included, as 4 bits+16 bits, =
// 0xFFFFF.  Single bank bins treated as a single bank 8.
// This makes address operations identical between single and multibank bins

// Data accesses default to bank 8 for single banks and bank 1 for multibanks.


#define BMAX      16             // number of banks

#define SADVERSION "6.0.1"
#define SADDATE    "8 Oct 2026"

// debug - when defined, this causes a LOT of output to xx_dbg file
// DBGxxx subr names kept uppercase to make debug prints more obvious to read

// #define XDBGX



#define NC(x)  sizeof(x)/sizeof(x[0])     // no of cells in struct 'x'

#define g_bank(x)    (x & 0xf0000)                            // get bank
#define nobank(x)    (x & 0xffff)                             // strip bank (and wrap)
#define bankeq(x,y)  ((x & 0xf0000) == (y & 0xf0000))         // address banks equal


#ifdef __linux__             // linux (gcc)

 #define PATHCHAR  '/'

#endif

#if defined __MINGW32__ || defined __MINGW64__     // windows (mingw)

 #define PATHCHAR  '\\'

#endif

#define PCORG  0x2000     // standard offset for all EEC bins

// various sizes
#define ANLPRT    5                  // print phase (anlpass)
#define STKSZ     16                 // size of fake stack(s)
#define EMUGSZ    5                  // size of emulate arguments list
#define SYMSZE    96                 // max size of symbol name

#define NSGV      32                 // num sig values 32
#define MAXSEQ    254                // max seq number

// global command options - index into cmdopts

#define OPTDPL     0         // decimal places
#define OPTSRC     1         // source code
#define OPTTBL     2         // auto table names (+func on old system)
#define OPTFUNC    3         // auto function names
#define OPTLU      4         // special subroutine names for function and table lookups
#define OPTLAB     5         // auto label names (lab)
#define OPTMAN     6         // full manual mode
#define OPTINTH    7         // auto intrp func naming
#define OPTSUB     8         // auto proc names (xfunc, vfunc, intfunc)
#define OPTSIG     9         // auto Lookup Signature subroutine detect
#define OPTPRE     10         // do preset symbols
#define OPT8065    11        // 8065 codeset (can be set internally)
#define OPTCMPA    12        // set compact layout for subr arguments (in lbk)
#define OPTCMPD    13        // set compact layout for data structs   (in adt)
#define OPTBKT     14        // do brackets, if then else

//#define OPTACM     15     // do autocomments

// file index into file holders
//  xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, xx_dbg.txt, SAD.ini

#define BINFILE 0
#define LSTFILE 1
#define MSGFILE 2
#define DIRFILE 3
#define CMTFILE 4
#define DBGFILE 5
#define INIFILE 6

// data types for printing, math calcs etc. (15 max)
//dptype = 0  means NO PRINT  (for bank only )

#define DP_HEX      1               // hex          integer
#define DP_DADD     2               // hex+bank     data address (16 bit wrap)
#define DP_BIN      3               // binary       integer
#define DP_DEC      4               // decimal      integer
#define DP_FLT      5               // float        float
#define DP_REF      6               // 'x' ref      extract from ADT definition (= data field) maths only
#define DP_SUB      7               //  -           subterm to calc, maths only
#define DP_CADD     8               // hex+bank     CODE address (16 bit wrap)



//specials above 7 ??
#define DP_MSKN     9               // TEST double for timers - binary mask with next byte - makes a symbol bx_A (timers etc)


// flages for param (number) getters
#define HXRLM     0x3ff             // read length mask
#define HXLZ      0x400             // lead zero single bit (bank 0)
#define HXNG      0x800             // '-' NEG before number (for offsets)

// symbol and command flags.  cmd index limited to 32

#define C_SIGN     0x20     // signed flag (symbols and fields)
#define C_WRITE    0x40     // write flag
#define C_NOBIT    0x80     // 'whole' nobits flag for symbols

#define C_USER     0x200     // by user command (can't change or merge)
#define C_SYS      0x400     // for system 'base generated' cmds
#define C_RENAME   0x800     // rename of symbol is allowed, even if duplicate

#define C_NOMERGE  0x1000      // inhibit merge for command options (as ADT added after..)
#define C_TEST     0x200000    // for verify scan from VECT, GAP, etc) add to addresses


// command indexes (flags above). index limited to 32

#define C_DFLT   0   // preset size data types.
#define C_BYTE   1
#define C_WORD   2
#define C_TRPL   3
#define C_LONG   4

#define C_TEXT   5
#define C_VECT   6

#define C_TABLE  7     //variable size
#define C_FUNC   8
#define C_STCT   9
#define C_TIMR   10

#define C_CODE   11    // separate code command chain

#define C_ARGS   12   // struct data types

#define C_XCODE  13

#define C_SUBR   14
#define C_SCAN   15
// rbase

#define C_SYM    17

//#define C_CALC   22
//bank
//setopts
//clropts
//set-psw


#define P_NOSYM  256     // inhibit sym print in header (pp_hdr, for args)

// jump and scan request types (and can have C_CMD for user commanded)

#define J_RET    0      // return opcodes
#define J_INIT   1
#define J_COND   2      // conditional jump ('if')
#define J_STAT   3      // 'static' jump (goto)
#define J_SUB    4      // subroutine calls
// #define J_VECT   5      // vect calls



/* code operand types ->optypes

 contents of operand
 type       reg         addr        val
 0          reg         reg         [reg]
 OPIMD       0          imd+bank    imd (16 bit max)
 OPBIT      reg         bit
 OPADDR                 addr+bank
 OPOFF                              offset (as + or -)
 OPIND      reg         [reg]       [[reg]]
 OPINX      reg         [reg+off]   [[reg+off]]


 int  val  ;            // value of read or write, or an immediate value   (go int 16; or short ??)
 uint addr ;            // actual address read from or written to (= register most of the time)

// (24 bits)
 uint reg     : 10;     // original register address (as in opcode) only 8, but allow for rambank.
 uint fend    : 9;      // field size +0x20 (sign) +0x40 (write) +0x100 show sign and size (for print)
 uint optype  : 3;      // reg = 0, imd = 1, i

*/



 // #define  OPREG  0   register is always present
#define  OPIMD  1
#define  OPBIT  2        // with register
#define  OPADDR 3        //
#define  OPOFF  4        //only op [0] ?
#define  OPIND  5
#define  OPINX  6
#define  OPAINC 7       // autoinc and indirect



















//error numbers for chains
#define E_DUPL  1          // duplicate
#define E_INVA  2          // invalid address
#define E_BKNM  3          // banks no match
#define E_ODDB  4          //odd address (WORD etc)
#define E_OVLP  5          //overlap
#define E_OVRG  6          //range overlap
#define E_XCOD  7          // XCODE bans
//#define E_SYMNAM   8       // syname replaced
//#define E_PAR   9


// chain index defines into datach

 #define CHJPF     0
 #define CHJPT     CHJPF  + 1
 #define CHSYM     CHJPT  + 1
 #define CHBASE    CHSYM  + 1
 #define CHSIG     CHBASE + 1

 #define CHCMD     CHSIG  + 1      // 5
 #define CHAUX     CHCMD  + 1
 #define CHSCAN    CHAUX  + 1
 #define CHEMUL    CHSCAN + 1
 #define CHSUBR    CHEMUL + 1

 #define CHADNL    CHSUBR + 1     // 10
 #define CHSPF     CHADNL + 1
 #define CHPSW     CHSPF  + 1
 #define CHDTDO    CHPSW  + 1
 #define CHDTDD    CHDTDO + 1

 #define CHBKF     CHDTDD + 1      // 15
 #define CHOPC     CHBKF  + 1
 #define CHRGST    CHOPC  + 1
 #define CHRGSTA   CHRGST + 1
 #define CHMATHN   CHRGSTA + 1

 #define CHMATHX   CHMATHN + 1      // 20
 #define CHLINK    CHMATHX + 1




//  file folders - 1 'header' 7 file holders.
//  xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, xx_dbg.txt, SAD.ini

typedef struct
{
 uint fillen;          // length of main binary file
 char bare    [64];       // bare file name (root binary name)
 char path    [254];      // path of root binary name
 char exepath [254];      // default path (SAD progam/exe location)
} HDATA;

typedef struct
{
 FILE     *fh;           // file handles for each file
 uint     pcol;          // print is at now (no of chars = colno)
 uint     lastpad;       // last padded to (colno)
// uint   cmntcol;       // where comments start, allows for big structures done in size array !!
 char fn  [256];      // full file names with path
} FDATA;


/* preset positions of print columns in xx.lst  printout
*
* 26      opcode mnemonic starts (after header)
* 32      operands start             (+6)
* 49      pseudo source code starts  (+17)
* 86      comment starts             (+37)
* 180     max width of page - wrap here
*/

#define MNEMPOSN   26
#define OPNDPOSN   32
#define PSCEPOSN   49
#define CMNTPOSN   83
#define WRAPPOSN   180


// Preset file permissions and names .... Windows and Linux

typedef struct
{
 cchar *suffix;
 cchar *fpars;
} HOP;


// banks holder, details & related pointers.

typedef struct                // indexed by bank num (0-16)
 {
 uchar  *fbuf;                // bin file (copy) for this bank ptr.

 uint  filstrt;               // start FILE OFFSET. ( = real offset, can be negative)
 uint  filend;                // end   FILE OFFSET.

 uint  minromadd : 20;        // bank start PC (normally PCORG)
 uint  bkmask   : 4 ;         // possible banks as bitmask (for 'find')
 uint  dbank    : 4 ;         // destination bank as number

 uint  maxromadd;             // bank end PC (= max PC allowed)
 uint  bprt     : 1 ;         // print this bank in msg file
 uint  bok      : 1 ;         // this bank is valid
 uint  usrcmd   : 1 ;         // set by user command
 uint  cbnk     : 1 ;         // global 'code start'/boot bank ( = 8 )
 uint  P65      : 1 ;         // this is an 8065 bank
 uint  noints   : 2 ;         // missing bytes affect filstart

 }  BANK;


//temp holder for find banks, includes interrupt handler counts etc.

typedef struct bnkfnd
{
 int  filestart;          // file start address
 uint pcstart;            // program start  (not always 0x2000)
 uint jdest;              // jump destination

 uint vc61   : 8;         // valid vector  count 8061 max 8
 uint ih61   : 8;         // valid handler count 8061 max 8
 uint vc65   : 8;         // valid vector  count 8065 max 40
 uint ih65   : 8;         // valid handler count 8065 max 40

 uint tbnk    : 5;        // temp bank number
 uint skip    : 5;        // max 15 (in code bank)
 uint lpstp   : 1;        // loopstop jump at start (not start bank)
 uint cbnk    : 1;        // real     jump at start (this is codebank)
 uint noints  : 1;        // bank with no interrupt vectors (FMO206)
}  BNKF;



// main CHAIN struct to hold data for binary chop searches
// void* provides for different structs to be used in each chain
// with defined handlers for free mem, compare
// chain ptr allows same subrs to work for different chains


typedef struct chain                           // done like a class with subroutines by chain type
{

uint asize;                                    // allocation size for pointer blocks
uint esize;                                    // entry (struct) size  - ZERO if remote chain

uint num;                                      // no of (used) elements, max index;
uint pnum;                                     // no of actual pointers in array.
uint next;                                     // next chmem index

uint lastix;                                   // last search index, for shortcuts and 'get_next_...'
uint lowins;                                   // lowest inserted index for loops to restart at
uint lasterr;                                  // for user commands etc

void *schptr;                                  // single block for searching, allocated on first use
void  **ptrs;                                  // ptr to array of void pointers (the index)

void (*cfree) (void*);                         // block free (by struct pointer)
int  (*comp)  (struct chain *, uint, void*);   // compare,  for binary chop  long for char* compares

} CHAIN;




/******************************************
* Additional data items
* used by data structs and subrs for arguments
**********************************************/
typedef struct xadt {

  void *fid;                       //  foreign key

//30

  uint fend    : 7 ;     // 0-31 end   of sub field (bit number), sign 0x20, write 0x40    - what about  WHOLE (0x80) ??
  uint pfw     : 6 ;     // (P) print min fieldwidth (0-31)
  uint fstart  : 5 ;     // 0-31 start of sub field (bit number).
  uint cnt     : 5 ;     // (O) repeat count, 31 max
  uint pfd     : 3 ;     // 0-7 print decimal places
  uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
  uint fnam    : 1 ;     // (N) look for symbol name
  uint sbix    : 1 ;     // this is a subchain
  uint sub     : 1 ;     // saves search....has ADT subdefinitions - don't print if only size declared ?? front or back ??


//8
  uint bank    : 4 ;     // (K) holds a bank override, zero if none
  uint dptype  : 4 ;     // replaces (X,R,*) and V <float V> radix and print mode.  as in defines at top 'DP___'
                          // and bits get replaced by 'calcs'
  uint xprt : 1;          //temp
} ADT;



// 'calc' structures.........

// variable number union with type

typedef struct  mlnum    // multiple number holder
{
 union
 {
   float fval;            // actual value (as floating) - may need double ?
   int  ival;             // actual value (as signed int)
 };

// maybe more stuff to go here ?

 uint dptype : 4 ;         // data/print/calc type - see above

}  MNUM;


// math 'hold'  structure for calcs for each cell, and nested cells


typedef struct mathld
{
  void *fid;               // current math block (starts with ADT ptr)

  MNUM total;              // running total
  MNUM par[3];             // params as variable, in case they are subcalcs

  uint ofst     : 20;      // current ofst from base ADT (need this for ranges with rbase, etc)
  uint calctype : 4;       // matches dptype, calc type for this term.
  uint npars    : 2;       // num pars (max 3) copied from mathx
  uint bank     : 4;       // if set, overrides data bank
  uint prt      : 1;       // printed (debug)
// error ?

} MHLD;


//calc name holder

typedef struct mthn
{
  // mathname is unique key
  char *mathname;          // name.  wait, where does this go ???? malloced....Hmmm.....
  char *suffix;            // for units etc (volts,time, ...) make this an index ???

  uint nsize      : 8;     // name size - can be zero for null name

  uint sys        : 1;     // system added

}  MATHN;


 // math algebra holder
 // func must be set

 // each struct item is "func(val, val2, val3) variable no of pars
 // and each val is number or 'ref' to data term or a subterm (brackets)
 // and subterms calc'ed first

typedef struct mathx
{
 void *fid;                       // attached to parent mathx or name

 MNUM fpar [3];                   // max of 3 fixed params/refs

 uint func      : 8;              // math function +,-, *, / etc.
 uint calctype  : 4;              // local calc type
 uint pix       : 2;              // param index (for subterms with fid)
 uint npars     : 2;              // num pars (max 3)
 uint lfirst    : 1;              // this item is first in local set, so don't print the implied '+'
 uint mfirst    : 1;              // this is first term of calculation

 uint fbkt      : 1;              // this math op is func with brackets (copy of preset)
 uint prt       : 1;              // 'printed' temp flag for debug

}  MATHX;


// preset function definition list for calcs

typedef struct mxf
{
uint fnpars     : 2;           // number of pars (3 max)  may up this
uint bkt        : 1;           // func must have brackets  ( + - / *  do not)
uint defctype   : 4;           // default calctype (0-16)
ushort nextzznum;

void (*calc) (MHLD *);         // actual calc function

} MXF;


 /*****************************************************
 * many-many linker by void* as foreign key type link
 * don't need reverse chain, just swop keys over as reqd
 * Technically don't need which chain scekey is in for a search,
 * but do need destkey chain to get correct cast of void.
 * so keep both sce and dest chains for ease of use.
 * NB. sce and dst chains NOT used as search keys....
 *
 *****************************************************/

typedef struct fkl
{
    void *keysce;           // source key
    void *keydst;           // dest   key
    uchar chsce;            // source chain
    uchar chdst;            // dest   chain
    uchar type ;            // default to 1, for multiple links between chain pair

}  FKL;


/********************************
 * RBASE list structure
 * map register to fixed address
 * multiple entries for same register allowed if range differs
 ********************************/

typedef struct
{
  uint    val;               // address value pointed to, including bank
  uint    setat;             // address set at
  uint    invat;             // where invalidated (address
  uint    rstart    : 20;    // range start
  uint    reg       : 10;    // register (as address)

  uint    rend      : 20;    // range end  - valid for rstart-r->end
  uint    ucnt      : 3;     // invalid count         NO!! need actual addresses to cover repeated scans !!

  uint    oinv       : 1 ;    // invalid flag by opcodes
  uint    tinv : 1;           // invalid flag by test
  uint    usrcmd    : 1 ;    // added by cmd (not changeable)
  uint    lock      : 1;     // set if rbase via a list (2020/2060)
  uint    test      : 1;     // set in test scan

  // add subroutine link ??

} RBT;


/********************************
 * PSW basic link struct
 * two straight addresses
 ********************************/

typedef struct
{
  uint    jstart;        // address of jump, including bank
  uint    pswop;         // address of psw setter
} PSW;


 /********************************
 *   symbol table structure
 ********************************/

// Note - fend has structure as defined so that largest write field first, then read, for
//        each field size, ordered by field start.
//        field end(0-0x1f), sign (0x20), write (0x40), no bit field specified (0x80)
// Range is 0-0xfffff if none specified (i.e. all 15 banks)

typedef struct
{

 char       *name;
 // char   *ncmnt;       // symbol comment for repeated explanation ?
 uint fmask ;             // field bitmask  (for ease of matching, calced from fstart,fend)

//28
 uint addr    : 20;
 uint fend    : 8;      // field end bit (0-0x1f), sign (0x20), write (0x40), nobits (0x80)
 uint  test   : 1;      // set in test scan
//29
 uint rstart  : 20;     // RANGE start - valid to rend (0 and 0xfffff for 'all')
 uint fstart  : 5;      // 0-31 (normally 0-7 unless overlap type)
 uint noprt   : 1 ;     // printed by one of other outputs (prt_xx), suppress sym print
 uint usrcmd  : 1 ;     // set by dir cmd, do not change
 uint sys     : 1 ;     // SAD autogenerated
 uint immok   : 1 ;     // sym is allowed with immediate ops (below PCORG)

// 29
 uint rend    : 20;     // RANGE - range end
 uint symsize : 8 ;     // size of symbol name + null (for mallocs and free)
 uint pbkt    : 1 ;     // print brackets in output (i.e. any adnl type extras, bits, write)
 uint inv     : 1 ;     // marked as invalid (vect tests etc)

 } SYM;


//*****  data addresses found in code analysis - links back to opcode
//do need possible more flags for func,tabs, (structs? loops ?)


typedef struct dx
{
   // 25

uint ofst      : 20;     // where read or written (opcode)
uint opcix     : 3;      // operand index (0-3)
uint test      : 1;      // done from test scan
uint inv       : 1;      // invalid (after test)
uint dscan     : 1;      //when scanned for data
// 30

uint dataptr   : 20;     // data pointer. Keyed with ofst
uint optype    : 3;      // reg = 0, imd = 1, ind = 2, inx = 3, bit = 4, adr = 5, off = 6; autoinc (ind) = 7;
uint fend      : 7;      // useful, saves opcode lookup ?


}  DTD;





// general command struct

//maybe can embed bank by using different bank in end ?
// but it's messy....and horrid


typedef struct lbk        // command header structure
{

 //32
 uint start    : 20;           // start address (inc bank)
 uint size     : 8;            // TOTAL size of data ADT additions/ dflt if fcom is zero
 uint xbank    : 4;            // where bank ptr != startbank but no extra ADT (vect, word ptr)

 //30
 uint end      : 20;           // end address (inc bank)
 uint fcom     : 5;            // command index
 uint term     : 2;            // data command has terminating byte(s) 0-3
 uint cptl     : 1;            // use compact layout if 1, extended if 0
 uint usrcmd   : 1;            // cmd defined by user
 uint nomerge  : 1;            // cannot merge with anything, for when ADT aded later...
 uint inv      : 1;

 ///does this need a test flag ?
} LBK;

// subroutine structure

typedef struct
{
  uint  start    : 20;        // start address (inc bank)
  uint  size     : 7;         // for any attached args (ADT)

  uint  usrcmd   : 1 ;        // set by command, no mods allowed
  uint  sys      : 1 ;        // system autogenerated
  uint  cptl     : 1 ;        // use compact layout if set,  extended otherwise
  uint  inv      : 1 ;
  uint  test     : 1 ;        // added in test scan
 } SUB;




// for subroutines with special functions

typedef struct xspf
 {
  void* fid;         // address or pointer ---uint pkey     :20;       // start addr of subr

//28
  uint fromadd  : 20;      // is part of search key, for duplicates
  uint spf      : 5;

  uint usercmd  : 1;
  uint inv      : 1;       //probably not necessary....
  uint test     : 1;

  uint sizereg  : 10;       // may not need these ?
  uint addrreg  : 10;
  uint fendin   : 7;
  uint fendout  : 7;




 } SPF;


/********************************
 *   Jump list structure
 ********************************/

typedef struct jlt
{
//27
  uint   fromaddr : 20;    // from address
  uint   size     : 3;     // jump size for adjacent
  uint   back     : 1;     // backwards jump
  uint   obkt     : 1;     // prt open bracket, and swop if logic from goto
  uint   cbkt     : 1;     // prt close bracket
  uint   bswp     : 1;     // bank change
  uint   inv      : 1;
  uint   test     : 1;
  uint   fscan    : 1;     //when scanned for funcs or tables

//28
  uint   toaddr   : 20;    // this will always be a new SBK, unless a retn
  uint   jtype    : 4;     // type (from scan)
  uint   cmpcnt   : 3;     // count back to previous compare (if set)
  uint   retn     : 1;     // a jump to a 'ret' opcode

  uint   jelse    : 1;     // static which has if jump toaddr immediately after it.

//  uint done  : 1;

//  uint jor   : 1;



// uint jwhile  : 1;
// uint jdo     : 1;

//  uint   subloop  : 1;     // this is a loop within a loop
//  uint   inloop   : 1;
//  uint   exloop   : 1;


 } JMP;




typedef struct sbk {          // scan block structure

struct sbk *caller;           // caller (for stack traces) calls only

//30
uint  start      : 20;        // start address
uint  psw        : 6;         // psw to carry over
uint  type       : 3;         // call type. 1 = cond, 2 = stat, 4 = sub
uint  chscan     : 1;         // chain scan only for debug really

//29
uint  curaddr    : 20;        // current addr - scan position (inc. start)
uint  scnt       : 8;         // temp for scan checks
uint  pushp      : 1;         // a pushp issued and live when set in this block (for multibanks) change to number of pushps ?
                              // pushp used for stack builds

//29
uint  nextaddr   : 20;        // next addr (opcode) or end of block.
uint  proctype   : 3;         // 0 sig/other,  1 (test flag)  2 scanning , 3 scan + test,  4 emulate,  5 emulate + test
uint  usercmd    : 1;         // added by user command, guess otherwise
uint  stop       : 1;         // scan complete
uint  inv        : 1;         // invalid opcode or flag in scan
uint  argsget    : 1;         // this (proc) is an arg getter
uint  emulreqd   : 1;         // mark block as emu required (every time called).

//26
uint  substart   : 20;        // start address of related subroutine (for names and sigs) for jumps

uint  regstat    : 1;         // do register sizes


uint incond      : 20;             // address where conditional jump ends
uint tstvect     : 1;              // issued from vector list, a 'test' mark

uint nodata : 1;
//need to reinstate nodata to stop accesses within tables and functions.....


}  SBK;



//V5 pushed   (SBK*)-1 directly, but didn't check result in a pop ??
 // if (newblk == (SBK*) -1  &&  newblk == s->stack[s->stkptr]) in fake_push.





/************************
operand holder for code anlysis
one code 'instance' has 4 OPS entries (3 ops [1][2][3] + indexed [0])
op [3] is ALWAYS the write op
only has fend (field end) as fstart is always zero for operands
***************************/


// register always present, except for immediates.
// addr is register address for straight reg-reg modes, or address for indirect modes
// address is 20....


typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of read or write, or an immediate value   (go int 16; or short ??)
 uint addr ;             // actual address read from or written to (= register most of the time)

// (24 bits)
 uint reg     : 10;     // original register address (as in opcode) only 8, but allow for rambank.
 uint fend    : 9;      // field size +0x20 (sign) +0x40 (write) +0x100 show sign and size (for print)
 uint optype  : 3;      // reg = 0, imd = 1, ind = 2, inx = 3, bit = 4, adr = 5, off = 6; autoinc (ind) = 7;
 uint neg     : 1;      // offset address is negative (byte only?)
 uint rbs     : 1;      // restore this for printing, maybe with val & address corrections.

} OPER;



// one 'opcode' with its associated operands

//set opcix at 0xff for args ??  wild idea, but....could then use operand for up to 4 items...

typedef struct inst
{
  OPER   opnd[4];          // 4 operands. [0] is for offset with indexed, and other stuff (jump offset)

//31
  uint  ofst     : 20;     // address of opcode, inc bank
  uint  opcix    : 8;      // opcode index into opcode table (max 112)
  uint  inv      : 1;
  uint  test     : 1;
  uint gotsigflag   : 1;  // skip or goto over 0xfe...special printing....CARD a01a/a01b

//30

  uint  opcode   : 8;      // actual opcode
  uint  sigix    : 7;      // opcode signature index
  uint  bank     : 4;      // bank override for this operand, if set by bank prefix
  uint  opsize   : 4;      // total opcode size in bytes
  uint  numops   : 3;      // number of operands.
  uint  opcsub   : 2;      //  0 = register (default) 1=imediate, 2 = indirect, 3 = indexed
  uint  save     : 1;      // save to chain (stops duplicate attempts)
  uint  feflg    : 1;      // sign/alt prefix set

} INST;

typedef const INST    CINST;       //constant version for chain retrieves
typedef const OPER    COPER;


// print holder for each operand, to match OPER.

typedef struct xpp
{
    SYM *opsym;            // if sym found. address based.

    uint sign    : 1;      // print size and sign
    uint bkt     : 1;      // print square brackets
    uint noprt   : 1;      // don't print operand.
    uint poptype : 3;      // print optype ... reg = 0, imd = 1, ind = 2, inx = 3, bit = 4, adr = 5, off = 6; autoinc (ind) = 7;

}  PP_OP;


// structure finder (tabs and funcs)
// argument holder for branch analysis subrs



typedef struct  sfx
 {
   SPF* spf;
   uint ans [2];
   uint rg  [2];
   uint lst [2];        //rbase ptr = address
 }  SFIND;


// Register status for fake stack and arguments handling


typedef struct regstx
 {

    MATHX *calc;             // calc associated (e.g. encode)
    uint argofst  : 20;      // actual address of this arg  (or pop address) ?  NB.   2nd key - reg may repeat (3654).. pops are zero?
    uint reg      : 10;      // this register - master key

    uint cofst    : 20;      // where created (opcode)   need host sbk ?
    uint fend     : 5;       // size
    uint popped   : 1;       // popped reg              //make these orig addresses ?? can check that way....

    uint arg      : 1;       // this is an argument register
    uint ref      : 1;       // index or indirect used, mark for name in ADT

  } RST;


typedef struct stk
{                                 // fake stack struct

SBK*  sblk;                       // current scan block (nextaddr for pop/push)
uint  popreg     : 10;            // register (last) popped to (or zero)
uint  ftype       : 3;             // 1 call, 2 imd, 4 pushp

} FSTK;


//preset strings, commands, names etc. index of CSTR strings

#define OPTSTR     0
#define SPFSTR     1
#define CMDSTR     2
#define MATHSTR    3
#define CALCSTR    4
#define INAME61    5
#define INAME65    6
#define AUTONAMES  7


// 'complex string' - strings with pars and  strlen without NULL terminator.

typedef struct cstr
 {

  uchar par1    ;       // 4 pars  depend upon actual list
  uchar par2    ;
  uchar par3    ;
  uchar len     ;       // string length, mostly
  cchar* string;        // name string

 }  CSTR;



typedef struct csinx
 {
   CSTR* strlist;

   ushort size;       //number of entries (cells)
   uint  delim    : 1;    //  9for calcs, whther to enforce delim at end of string match

 } CSINX;




// user command holder and comment holder for parse and processing

typedef struct               // command holder for cmd parsing
{
  MATHX  *mterm;
  ADT    *adnl;              // current adnl pointer
  CINST  *cinst;             // and instance (for comments)
  CSTR   names [2];           // name and size of 2 name strings

  char  *cmpos;             // current read char posn
  char  *cmend;             // end of command line

  int   p [8];              // 8 params max (allow for negative values)
  uint  pf[8];              // width, param flags, etc

  int  mcalctype  ;
  int  mfunc      ;        // math built in function
  int  sqcnt;              // square bracket count
  int  rbcnt;              // round bracket count
  int  adtcnt;             // adt terms
  int  mcnt;               // math terms
  int  error;              // for true error return

  uint adtsize;             // total size of additional options  (bytes)
  uint  fcom ;              // command (+ flags ?).

//29
  int   adreg    : 11;      // address register  also used for calctype, hence int
  uint  szreg    : 10;      // size register
  uint  spf      : 5;       // special function subroutine
  uint  firsterr : 1;       // first error, print cmd line if zero
  uint  split    : 1;       // '|' detected for split comment
  uint  newline  : 1;       // newline at front of comment.
  uint  bitfld   : 1;       // bit field specified in command

//30
  uint  ansreg   : 10;      // address register
  uint  npars    : 3;       // num of pars in first number set (for error checks)

  uint  term     : 2;       // terminating byte(s)
  uint  extcpt   : 2;       // use extended (1) or compact (2) layout


  uint  imm      : 1;       // immd ok for syms and reduced address marker, comments
  uint  flags    : 1;       // flagsword for syms
  uint  xnames   : 1;      // names only (syms)
 // uint  write    : 1;
  uint  debug    : 1;       // temp for a breakpoint

 } CPS;



/********************************
 *  Input Command definition structure

  //up to 8 types for each param for validation.
  // 0 - none, 1 start addr, 2 end addr, 3 register, 4 st range, 5 end range (4 and 5 equiv 1 and 2)?
  // maybe 6 for plain number (with opt strings above ??)
 ********************************/

 typedef struct drs               // command definition
 {
  uint  (*setcmd) (CPS*);         // command processor(cmd struct)
  uint  (*prtcmd) (uint, LBK *);  // command printer (ofst,cmd index)       //may need another INST ?
  uint  minpars : 3 ;             // min pars expected
  uint  maxpars : 3 ;             // max pars allowed/expected 0-7

  uchar ptype[4];                 // may need more ??

//20
  uint  cmpargs    : 1 ;             // if extended args cmd has effect (and printed)
  uint  cmpdata    : 1 ;             // if extended data cmd has effect (and printed)

  uint  namex      : 2 ;             // no name (0) name optional (1) expected (2)

  uint  minadt     : 5 ;             // min ADT levels (< 31)
  uint  maxadt     : 5 ;             // max ADT levels (< 31)

  uint  defsze     : 6 ;             // default size, same as ssize in ADT
  uint  defdptype  : 4 ;             // default type, same as dptype in ADT

  uint  merge      : 1 ;             // can be merged with same command (adj or olap)
  uint  stropt     : 2 ;             // 1 = command with option strings (e.g. setopt) 2 = calcstr             //change to first 'type' above ??

  cchar*  glopts;             // global options
  cchar*  mainopts;              // data levels
  cchar*  subopts;              //sub levels if allowed
} DIRS;



/********************************
 * signature holder structure in chain and core (sign are uchars)
 ********************************/

typedef struct xsig
{

 uint  start;             // start address of sig found
 uint  end;               // end of signature - only really used to stop multiple pattern matches and overlaps

//17
 uint  hix     : 8;       // sig handler index
 uint  patno   : 8;       // pattern number (for duplicate checks)
 uint  done    : 1;       // processed marker - for 'once only' sigs (e.g. timer)

 uint  vx[NSGV];          // 32 saved values

} SIG;





//global 'base' banks holder

typedef struct bpx
{
uint  datbnk;         // currently selected data bank - always 8 if single bank
uint  codbnk;         // currently selected code bank - always 8 if single bank
uint  rambnk;         // currently selected RAM bank  - always 0 for 8061
} BASP;



// 'symbol list' holder for searches. malloced struct.

typedef struct slx
{
  uint defwr;               // default write ix
  uint defrd;               // default read ix
  uint startix;             // start ix
  uint endix;               // end ix
  uint bcnt;                // count of bitfield syms found
  uint wcnt;                // count of 'whole' syms found

  uint fend : 8;               //for read/write/sign etc

}  SYMLIST;


// opcode main definition - called from INDEX structure, not opcode value
// changed to v5 like, with write op always at 3

typedef struct              // OPCODE definition structure
 {
  uint sigix   : 7;         // signature index (='fingerprint')
  uint sign    : 1;         // signed prefix allowed (goes to next opcode entry)

  uint p65     : 1;         // 8065 opcode only
  uint rbnk     : 1;         // rombank prefix allowed (8065 only)
  uint pswc    : 1;         // op changes PSW  (for conditional ifs)
  uint nops    : 2;         // number of ops   (max 3)

  uint xxxcondprt : 1;         //  not used

  uint fend[4];                  // operand sizes, field_end + 0x20 sign, 0x40 write, 0x100 print size + sign

  SBK* (*eml) (SBK*, INST *);    // emulation func
  uint (*pp) (INST *);           // print func
  cchar name [6];                // opcode name
  cchar *sce;                    // high level code description, with extra print markers
 } OPC;

typedef struct eml

{LBK* cmd;
 SBK *sblk;
} EMULOG;



typedef struct  sfrx            // OPCODE definition structure
 {
  uint reg   : 7;           // SFR
  uint read    : 1;         // can read

  uint write     : 1;         // can write
  uint byte      : 1;         // byte
  uint word      : 1;         // word

 } SFRSPEC;













// ----- end ------------

#endif

