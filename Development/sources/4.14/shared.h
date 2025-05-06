

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
* 806x bit numbering - bit 0 is least significant (rightmost),
* bit 7 (15 etc) is most significant.
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

/// #include <wchar.h>      for extended char.......

#ifndef _XSHARX_H

#define _XSHARX_H 1

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef const char     cchar;


// Banks can be 0-15, only 0,1,8,9 used. All +1 internally = 1,2,9,A
// All registers in Bank 8,  4 banks of 64k are malloced.


#define BMAX      16             // maximum bank index

#define SADVERSION "4.0.14.3"
#define SADDATE    "06 May 2025"

// debug defines - when switched on, this causes a LOT of output to xx_dbg file
// DBGPRT id is kept to make debug code more obvious to view

// #define XDBGX

#define NC(x)  sizeof(x)/sizeof(x[0])     // no of cells in struct 'x'


#define g_bank(x)    (x & 0xf0000)                            // get bank
#define nobank(x)    (x & 0xffff)                             // strip bank
#define bankeq(x,y)  ((x & 0xf0000) == (y & 0xf0000))         // address banks equal





#define PCORG  0x2000     // standard offset for all EEC bins

// option  defs (as bitmask)

#define PSSRC     (cmdopts & 1)         // source code
#define PTABL     (cmdopts & 2)         // auto table names (+func on old system)
#define PSTRS     (cmdopts & 4)         // auto subroutine names for function and table lookups
#define P8065     (cmdopts & 8)         // 8065 codeset (can be set internally)

#define PLABEL    (cmdopts & 0x10)      // auto label names (lab)
#define PMANL     (cmdopts & 0x20)      // full manual mode
#define PINTF     (cmdopts & 0x40)      // auto intrp func naming
#define PPROC     (cmdopts & 0x80)      // auto proc names (xfunc, vfunc, intfunc)

#define PSIG      (cmdopts & 0x100)     // auto Lookup Signature subroutine detect
#define PFUNC     (cmdopts & 0x200)     // auto function names
#define PACOM     (cmdopts & 0x400)     // do autocomments
#define PRSYM     (cmdopts & 0x800)     // do preset symbols

#define PARGC     (cmdopts & 0x2000)     // set argument compact layout default
#define PSTCA     (cmdopts & 0x4000)     // set data struct as argument layout (DATAEXT)

#define PDEFLT    (0xFC7)                //  default collection

//param flags for banks and so on.



#define HXRL      0xf              // read length mask 4 bits at bottom
#define HXLZ      0x40             // lead zero single bit
#define HXNG      0x80             // '-' before number for offsets


#define ANLFFS    4                  // find funcs, post scan (anlpass)
#define ANLPRT    5                  // print phase (anlpass)
#define STKSZ     10                 // size of fake stack(s)
#define EMUGSZ    5                  // size of emulate arguments list
#define SYMSZE    96                 // max size of symbol name

#define NSGV      32                 // num sig values, but sig code allows for 32
#define MAXSEQ    254                // max seq number

// command values.  Up to 31 commands (32 is cmd flag, can move it)
// symbols first

#define C_SIGN   0x20     // symbols and fields signed
#define C_WRT    0x40     // write flag
#define C_WHL    0x80     // 'whole' flag for symbols
#define C_OLP    0x100    // don't adjust addresses

#define C_USR    0x200     // by user command (can't change or merge)
#define C_SYS    0x400     // for system 'base generated' cmds
#define C_REN    0x800     // rename of symbol is allowed, even if duplicate




//#define C_CMD      0x100      // by command (can't change or merge)

#define C_NOMERGE  0x1000     // inhibit merge for command options
#define C_GAP      0x2000     // for gap scan



#define C_DFLT   0  // preset size data types.
#define C_BYTE   1
#define C_WORD   2
#define C_TRPL   3
#define C_LONG   4
#define C_TEXT   5
#define C_VECT   6


#define C_TABLE  7
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
//bank
//setopts
//clropts
//set-psw



#define P_NOSYM  256     // nosym for pp_hdr

// jump and scan request types (and can have C_CMD for user commanded)

#define J_RET    0      // return opcodes
#define J_INIT   1
#define J_COND   2      // conditional jump ('if')
#define J_STAT   3      // 'static' jump (goto)
#define J_SUB    4      // subroutine call
//#define J_ELSE   5      //not yet.
//#define J_OR     6

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


// chain index defines

 #define CHJPF     0
 #define CHJPT     CHJPF  + 1
 #define CHSYM     CHJPT  + 1
 #define CHBASE    CHSYM  + 1
 #define CHSIG     CHBASE + 1
 #define CHCMD     CHSIG  + 1
 #define CHAUX     CHCMD  + 1
 #define CHSCAN    CHAUX  + 1
 #define CHEMUL    CHSCAN + 1
 #define CHSBCN    CHEMUL + 1
 #define CHSGAP    CHSBCN + 1
 #define CHSUBR    CHSGAP + 1
 #define CHADNL    CHSUBR + 1
 #define CHRGST    CHADNL + 1
 #define CHSPF     CHRGST + 1
 #define CHPSW     CHSPF  + 1


 #define CHDTK     16
 #define CHDTKR    17
 #define CHDTKO    18
 #define CHDTKD    19
 #define CHBNK     20
 #define CHOPC     21
// #define mblk     17



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
// uint   *opcbt;               // opcode start flags for this bank (bit array ptr).

 uint  filstrt;                // start FILE OFFSET. ( = real offset, can be negative)
 uint  filend;                // end   FILE OFFSET.


 uint  minromadd : 20;                 // min PC (normally PCORG)
 uint  bkmask   : 4 ;         // possible banks as bitmask (for 'find')
 uint  dbank    : 4 ;         // destination bank as number

 uint  maxromadd;                 // end PC (= max PC allowed (where fill starts)
 //uint  maxbk;                 // where bank really ends (after fill)
 uint  bprt     : 1 ;         // print this bank in msg file
 uint  bok      : 1 ;         // this bank is valid (and found flag)
 uint  usrcmd   : 1 ;         // set by command
 uint  cbnk     : 1 ;         // code start bank ( = 8 )
 uint  P65      : 1 ;         // this is an 8065 bank
 uint  noints   : 2 ;         // missing bytes affect filstart

 }  BANK;

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






// opcode main definition - called from INDEX structure, not opcode value
// changed to v5 like, with write op always at 3

typedef struct              // OPCODE definition structure
 {
  uint sigix   : 7;         // signature index (='fingerprint')
  uint sign    : 1;         // signed prefix allowed (always goes to next entry)

  uint p65     : 1;         // opcode 8065 only
  uint rbk     : 1;         // rombank prefix allowed
  uint pswc    : 1;         // op changes the PSW (for conditional ifs)
  uint nops    : 2;         // number of ops   (max 3)

  uint fend1  ;             // read op field end
  uint fend2  ;             // These sizes are field_end + 0x20 sign, 0x40 write
  uint fend3  ;             // and get converted for sce code printouts

  void (*eml) (struct sbk *, struct inst *);    // emulation func
  const char name [6];           // opcode name
  const char *sce;               // high level code description, with extra print markers
 } OPC;


typedef struct    // malloced blocks tracker
{
void *blk;        // pointer to block
size_t size;      // size of block
} MBL;


// main CHAIN struct to hold stuff for binary chop searches etc
// void* provides for different structs to be used in each chain
// with defined handlers for free mem, compare, and equals
// chain ptr allows same subrs to work for different chains

typedef struct chain                           // done like a class with subroutines by chain type
{
uint num;                                      // no of (used) elements;
uint pnum;                                     // no of pointers in array.
uint asize;                                    // allocation size for pointer blocks
uint esize;                                    // entry (struct) size  - ZERO if remote chain
uint DBGmax;                                   // max no of ptrs allocated   for debug
uint DBGallocs;                                // no of reallocs   for debug
uint lastix;                                   // last search index, for shortcuts and 'get_next_...'
uint lasterr;                                  // for user commands etc

void  **ptrs;                                  // ptr to array of pointers
void  (*cfree) (void*);                        // block free (by struct pointer)
int   (*comp)  (struct chain *, uint, void*);  // compare,  for binary chop
} CHAIN;


/******************************************
* Additional data struct
* used by data structs and subrs for inline params
**********************************************/

/* V5 follows (extra calcs)
// tsize for total size of chain ?? cheat with a union ??

typedef struct xadt {

 void *fid;              // foreign key, first link by address, rest void* pointer.
//28
 uint fstart  : 5 ;     // (B) 0-31 start of sub field (bit number). = B x y  (start, end)
 uint fend    : 7 ;     // (B) 0-31 end   of sub field (bit number), with sign (0x20) and write (0x40)
 uint cnt     : 5 ;     // (O) repeat count, 31 max
 uint pfd     : 5 ;     // (P) 0-31 print decimal places  [forces radix 10] = X n.p  main part goes in pfw
 uint pfw     : 6 ;     // (P) print min fieldwidth (0-31) inc decimal places ?  *** to confirm - add pfd ??

// 13
 uint bank    : 4 ;     // (K) holds a bank override, zero if none (i.e. default to databank)
 uint dptype  : 4 ;     // (X,R) and V <float V> radix and print mode.  as in defines at top 'DP___'
 uint sbix    : 2 ;     // sublevel/subfield for use with fid+n  (top levels use plain fid) may only need one bit ?      <<<<<<<
 uint fnam    : 1 ;     // (N) look for symbol name, or 'name' flag with sym command (may be replaced with 'M')
 uint newl    : 1 ;     // (|) break printout with newline (at end of this level)
 uint sub     : 1 ;     // has ADT subdefinitions - don't print if only size declared ?? front or back ?? redundant ??



} ADT;

//calc structures.........

// variable number type........... *


typedef struct  mlnum    // multiple number holder
{

 union
 {
     // may need double instead of float ?
   float fval;            // actual value (as floating)
   int  ival;             // actual value (as int)               // allow signed
 };

// may be more stuff to go here

 uint dptype : 4 ;         // data/print/calc type - see above

}  MNUM;



// math hold structure for calcs of each cell, and nested cells

// sync calctype with pmode from ADT ???

typedef struct mathld
{
  void *fid;               // current math block (starts with ADT ptr)

  MNUM total;              // running total
  MNUM par[3];             // params as variable, in case they are subcalcs

  uint ofst     : 20;      // current ofst from base ADT (need this for range with rbase, etc)
  uint calctype : 4;       // matches dptype, and is MASTER calc type.
  uint npars    : 2;       // num pars (max 3) copied from mathx
  uint bank     : 4;       // if set, overrides data bank

// error ?

} MHLD;

// math formula name separate as it will be reused
// or have first term in here, ready for following f(x) terms ?
// but then what about new starts (brackets?)

// many-many (via FKL) to NAME <-term <- term

typedef struct mthn
{
  void *fid;               // points to ADT or SIG or CMND and linked multiple times

  char *mathname;          // name.  wait, where does this go ???? malloced....Hmmm.....
  char *suffix;            // for units etc (volts,time, ...) make this an index ???

  uint nsize      : 8;     // name size - can be zero for null name
  uint mcalctype  : 4;     // master calc type

  uint sys        : 1;     // system added

}  MATHN;


 // math expression
 // func must be set

 // each struct item is "func(val, val2,val3)
 // and each val is number or 'ref' to data term or a subterm (brackets)
 // and subterms calc'ed first


typedef struct mathx
{
 void *fid;                      // attached to parent mathx or name

 MNUM fpar [3];                  // max of 3 fixed params/refs

 uint pix       : 2;              // param index (for subterms with fid)
 uint npars     : 2;              // num pars (max 3)

 uint first     : 1;              // this item is first in local set, so don't print the implied '+'

 uint func      : 8;              // math function +,-, *, / etc.
 uint fbkt      : 1;              // this math op is func with brackets (copy of preset)
 uint prt       : 1;              // temp for debug
 uint calctype  : 4;              // local calc type
}  MATHX;


// preset function list, includes single char +,- etc may need datatypes ??? or use calctype ?

typedef struct mxf
{
uint fnpars     : 2;           // number of pars (3 max)
uint bkt        : 1;           // func must have brackets  ( + - / *  do not)
uint defctype   : 4;           // default calctype
ushort nextnum;

void (*calc) (MHLD *);         // actual calc function (no mathx ?)

} MXF;

 *****************************************************
 * many-many linker by void* as foreign key type link
 * don't need reverse chain, just swop keys over
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





typedef struct xadt {

    //data is removed entirely with calc..............
    // as are some other fields too

union
{
float fldat;           // full float for divisor (float = int32 for size)
int   data;            // addr, reg, offset, etc. int to allow for -ve
};


void *fid;              //uint fkey;             //  foreign key,  (ofst < 8) | seq 1-255

//31
uint dreg    : 10 ;    // data destination register for data - for arg processing (0-0x3ff)
uint fstart  : 5 ;     // 0-31 start of sub field (bit number).
uint fend    : 7 ;     // 0-31 end   of sub field (bit number), sign 0x20, write 0x40
uint cnt     : 5 ;     // (O) repeat count, 31 max
uint pfw     : 5 ;     // (P) print min fieldwidth (0-31)

//19
uint bank    : 4 ;     // (K) holds a bank override, zero if none
uint places  : 4 ;     // 0-15 print decimal places (is 7 enough ?) [forces radix 10]
uint enc     : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)
uint prdx    : 2 ;     // (X) print radix 0 = not set, 1 = hex, 2 = dec, 3 bin
uint fnam    : 1 ;     // (N) look for symbol name
uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr   : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint foff    : 1 ;     // (D) data is an offset address
// uint ans     : 1 ;     // (=) this is ANSWER definition (separate item, for verification) move t spf
uint div     : 1 ;     // (V or /) data is FLOAT, use fldat (union with data)
uint mult    : 1 ;     // (*) data is FLOAT, use fldat (union with data)
uint sbix : 1;         //subchain

// uint write  : 1 ??
} ADT;




/********************************
 * RBASE list structure
 * map register to fixed address
 * only one value per register allowed
 ********************************/

typedef struct
{
  uint    val;               // address value pointed to, including bank
  uint    rstart    : 20;    // range start
  uint    reg       : 10;    // register (as address)

  uint    rend      : 20;    // range end  - valid for rstart-r->end
  uint    ucnt      : 3;     // invalid count

  uint    inv       : 1 ;    // invalid flag  (value changes somewhere in code)
  uint    usrcmd    : 1 ;    // added by cmd (not changeable)
  uint    lock      : 1;     // set if rbsae via a list (2020/2060)

  // uint sig   :1 ;   // added by signature ??? stop invalid.


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

// includes autonumber symbols (with type).
// addb is combination of address, bitno  4|16|4  bank, address, bitno.
// bitno is max of 7 and split by byte in insert and find.
typedef struct
{

 char       *name;
 // char   *ncmnt;       // symbol comment for repeated explanation ?
 uint fmask ;             // field bitmask  (for ease of matching, calced from fstart,fend)

 uint addr    : 20;
 uint fend    : 8;        // Read size as field (0-0x1f), sign (0x20), write (0x40) WHOLE (0x80)
 //uint olap    : 1;        // SFR - this overlaps next bytes (ie word at R10, byte at R11)


 uint rstart  : 20;     // RANGE start - valid to rend (0 and 0xfffff for 'all')
 uint fstart  : 5;      // 0-31 (normally 0-7 unless overlap type)
 uint noprt   : 1 ;     // printed by one of other outputs (prt_xx), suppress sym print
 uint usrcmd  : 1 ;     // set by dir cmd, do not change
 uint sys     : 1 ;     // system autogenerated
 uint immok   : 1 ;     // sym is allowed with immediate ops (below PCORG)

 uint rend    : 20;     // RANGE - range end
 uint pbkt    : 1 ;     // print brackets in output (i.e. any extras??)
 uint symsize : 8 ;     // size of symbol name + null (for mallocs and free)

 } SYM;








//*****  for data addresses tracking found in code analysis


// tricky - definitely need search by a) register b) data start.
// c) ofst ??

typedef struct dtk
{

// first items copy lbk for later transfer and validate or adjust
// mutliple starts in addnl struct below

// but opcsub+psh+cmp+stx ia already 5 bits......numops is 7
//perhaps a simple opcix/full and 3 regs and


  ushort rgvl[5];            // reg 0-4 for indexes etc.

  uint ofst    : 20;      // where from - PRIMARY KEY

  uint opcsub  : 2;       // direct,indirect,imd,inx
  uint psh     : 1;       // push instruction or related

  uint ocnt    : 4;       // total size of 'from' opcode
  uint opcix   : 8;       // opcode index

  uint ainc    : 2;       // auto inc size
  uint numops  : 2;       // actual numops

 // uint done ??
} TRK;

//move inc etc into here to reduce access to main entry for struct analysis,
// opcsub and size, psh,


typedef struct dx
{
uint ofst      : 20;    // foreign key is ofst (opcode)

int off      : 8;      // where indexed, may be others....may have to be full 20 bits for base+r as oposed to R+offset

uint stdat   : 20;    // no seq, uses unique start address instead.

//uint modes  :  4;    // multiple opcsub by bit flag for checks   DOESN'T WORK

uint opcsub : 2;

//not so much as an imd, but more as fixed or variable, so R0+inx are FIXED

uint inc    :   2;    // auto inc size, 1 or 2
uint olp     :  1;    // overlaps code   (not valid)
uint psh     :  1;
uint gap     :  5;    // where next address is close, keep gap
uint bsze    :  3;    // 1,2, or 4 bytes
uint hq      :  1;    //high quality reliable
//more bits fit here anyway

}  DTD;


// register indexed ???
typedef struct dr
{



} DTR;








// general command struct

typedef struct lbk        // command header structure
{

 uint start  : 20;           // start address (inc bank)
 uint size   : 8;            // TOTAL size of data ADT additions
 uint dflt   : 1;            // this is default command made up for print.

 uint end    : 20;           // end address (inc bank)
 uint fcom   : 5;            // command index
 uint term   : 2;            // data command has terminating byte(s) 0-3
 uint argl   : 1;            // use args layout
 uint cptl   : 1;            // use compact layout
 uint usrcmd : 1;            // defined by user

// gribble  nomerge
 uint nomerge    : 1;          //cannot merge with anything


} LBK;





//overlay struct (on ops[4]) for bitwise searches.

typedef struct opsx
{
uint pmask ;         // print mask
uint nmfnd  ;        // names found mask
uint rw    : 1;      // look for read or write syms ??
uint fxor  : 1;      // this is an XOR opcode
uint flags : 1;      // flags word 'F'
uint names : 1;      // names      'N'
uint sceix : 3;      // source index
}  OPBIT;


// subroutine structure

typedef struct
{
 uint  cmd    : 1;        // set by command, no mods allowed
 uint  sys    : 1 ;       // system autogenerated
 uint  cptl   : 1;        // use compact layout  (args is default)
 uint  end    : 20;
 uint  size   : 8;        // for any attached args (ADT)
 uint  start  : 20;       // start address (inc bank)
 } SUB;


// for subroutines with special functions only ever 1 attached
// add ANSWER to this !

typedef struct xspf
 {
  void* fid;         // address or pointer ---uint pkey     :20;       // start addr of subr
 //   SIG  *sig;               // sig if relevant.

  uint fromadd  : 20;       // is part of search key, for duplicates
  uint fendin   : 7;       // may not need this...
  uint fendout  : 7;       // or this

  uint addrreg  : 10;
  uint spf      : 5;
  uint sizereg  : 10;
 } SPF;


/********************************
 *   Jump list structure
 ********************************/

typedef struct jlt
{

  uint   back     : 1;     // backwards jump
  uint   obkt     : 1;     // prt open bracket (and swop logic from goto)
  uint   cbkt     : 1;     // prt close bracket


  uint   bswp     : 1;     // bank change
  uint   size     : 3;
  uint   fromaddr : 20;      // from address

//  uint done  : 1;
//  uint jelse : 1;
//  uint jor   : 1;



// uint jwhile  : 1;
// uint jdo     : 1;
  uint   retn     : 1;     // a jump to a 'ret' opcode
//  uint   subloop  : 1;     // this is a loop within a loop
//  uint   inloop   : 1;
//  uint   exloop   : 1;
  uint   jtype    : 4;     // type (from scan)
  uint   cmpcnt   : 3;    // count back to previous compare (if set)
  uint   toaddr   : 20;   // this will always be a new SBK, unless a retn

 } JMP;


/* loop struct ??

typedef struct loop
{
uint  start      : 20;         // start address
uint  end        : 20;         // end address
//uint  lastimd    : 20;         // last immediate assigned. NO !!
uint  incr       : 4;          // at least one incr (count)
uint incr2       : 4;          // for inc at same address (diouble check)
uint  inrx       : 1;          // at least one inx or inr
uint  increg     : 10;         // register incremented
int   sval ;
uint datastart   : 20;

} LOOP;
*/

// scan block struct - can be chained to simulate a stack for nested subr calls


typedef struct sbk {          // scan block structure

struct sbk *caller;           // caller (for stack builds)

uint  type       : 3;          // 1 - cond, 2 = stat, 4 = sub
uint  start      : 20;         // start address

uint  scnt       : 8;         // temp for scan checks
uint  chscan     : 1;         // chain scan
uint  gapscan    : 1;         // gap scan
uint  curaddr    : 20;        // current addr - scan position (inc. start)

uint  logdata    : 1;         // log data to LBK chain
uint  args       : 1;         // this (proc) is an arg getter

uint proctype : 3;            // 0 sig, 1 scanning , 4 emulating
//uint  scanning   : 1;         // scan processing (do jumps, scans etc)
//uint  emulating  : 1;         // emulating (with dummy scan block)
//uint  lscan      : 1;         // loop scan (emulate), change some things
uint  emulreqd   : 1;         // mark block as emu required (every time called) for args (here)
uint  nextaddr   : 20;        // next addr (opcode) or end of block.

uint  cmd        : 1;         // added by user command, guess otherwise
uint  stop       : 1;         // scan complete
uint  inv        : 1;         // invalid opcode or flag in scan

uint  php        : 1;         // block uses pushp (for multibanks)

uint  substart   : 20;        // start address of related subroutine (for names and sigs)

}  SBK;



/************************
operand handling for printouts and anlysis
inst has 5 OPS entries (3 ops [1][2][3] + index [0] + saved [4])
op [3] is ALWAYS the write op
only has fend (field end) as fstart is always zero for operands
***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of register contents or an immediate value
 uint addr ;            // actual operand address (register mostly)

// (20 bits)
 uint fend    : 8;         // field size +0x20 (sign) +0x40 (write)

 uint ind     : 1;      // indirect
 uint inx     : 1;      // indexed, use with operand [0]
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a  register  value
 uint bit     : 1;      // this is a  bit       value
 uint adr     : 1;      // this is an address (includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with sign)
 uint neg     : 1;      // offset address is negative
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print square brackets
 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found (printout only)
} OPS;



typedef struct inst
{

  OPS   opr[5];            // 5 operands. [4] is for SAVED operands, indirect and indexed.

  uint  ofst     : 20;     // address of opcode, inc bank
  uint  opcix    : 8;      // opcode index into opcode table (max 112)
  uint  feflg    : 1;      // sign/alt  bit prefix (0xfe) mainly for sigs

  uint  opcode   : 8;
  uint  sigix    : 7;      // opcode signature index
  uint  psw      : 6;      // psw AFTER this operation
  uint  bank     : 4;      // bank override for this operand, if set. (by bank prefix)
  uint  numops   : 3;      // number of operands - NOT ALWAYS same as opl (e.g. for BIT jumps)
  uint  opcsub   : 2;      //  0 = register (default) 1=imediate, 2 = indirect, 3 = indexed

} INST;

typedef struct topc
{
    uint ofst;

}  TOPC;


// structure finder (tabs and funcs)
// argument holder for branch analysis subrs

typedef struct  sfx
 {
   SPF* spf;                     //int
   uint ans [2];
   uint rg  [2];
   uint lst [2];
 }  SFIND;


// Register status for fake stack and arguments handling


typedef struct regstx
 {
 //  uint ssize   : 4;       // size last used, as ADT

//uint substart : 20;       // for subroutine ??

   uint fend    : 5;       //
   uint popped  : 1;       // popped reg
   uint arg     : 1;       // argument register (any)
   uint sarg    : 1;       // source arg (1st level from pop)
   uint inc     : 1;       // has been incremented
   uint inr     : 1;       // indirect (address)
 //  uint used    : 1;       // when used as arg (for size)
   uint enc     : 4;       // encoded type
   uint reg     : 10;      // register
   uint sreg    : 10;      // source register (i.e. the popped one) for ADT
   uint data    : 20;      // dreg for encode or data
   uint ofst    : 20;

 } RST;



//   Fake Stack - for arg handling and emulation

// TYPES 1-call 2-imd push 3-pushp (4 ?std push)
// pop set if popped (and register)


typedef struct stk
{                              // fake stack struct
uint  popped     : 1;             // register entry is popped, cleared when pushed back
uint  reg        : 10;             // register (last) popped to
uint  origadd    : 20;            // original value in nxtadd - for args detect
uint  type       : 3;             // 1 call, 2 imd, 3 psw
uint  psw        : 6;             // PSW value
uint  newadd     : 20;            // current addr value (at push)
}  FSTK;


// user command holder and comment holder for parse and processing


typedef struct               // command holder for cmd parsing
{
  ADT   *adnl;              // current adnl pointer
  INST  *minst;
  char  *cmpos;             // current read char posn
  char  *cmend;             // end of command line

  int   p[8];               // 8 params max (allow for negative values)
  uint  pf[8];              // param flags

  int   error;              // for true error return

  uint adtsize;             // total size of additional options  (bytes)
  uint  fcom ;              // command + flags.
  uint  adreg    : 10;       // address register

  uint  spf      : 5;       // special function subroutine
  uint  firsterr : 1;       // first error, print cmd line if zero
  uint  szreg    : 10;       // size register

  uint  ansreg    : 10;       // address register
  uint  seq      : 8;       // seq no for additionals
  uint  npars    : 3;       // num of pars in first number set (for error checks)

  uint  term     : 2;       // terminating byte(s)
  uint  argl     : 1;       // use args layout
  uint  cptl     : 1;       // use compact layout
  uint  imm      : 1;       // immdok for syms
  uint  flags    : 1;       // flagsword for syms
  uint  names    : 1;       // names only (syms)
  uint write     : 1;
  uint bits      : 1;
  uint debug     :1;             //temp for a breakpoint
  char  symname[SYMSZE+1];   // symbol 96 + null

 } CPS;

typedef struct cstr
 {
  uint minsize;
  cchar* str;

 }  CSTR;

/*
typedef struct cmnt
 {
   char  *ctext;               // comment text       cmpos
   INST  *minst;               // where ops are                 minst
   uint  ofst     : 20;        // next addr for comment         p[0]
   uint  newl     : 1;         // set if cmt begins with newline  argl
 //  uint  nflag    : 2;         // newline reqd for cmnd blocks split
 } CMNT;
*/

/********************************
 *  Input Command definition structure
 ********************************/

 typedef struct drs               // command definition
 {
  uint  (*setcmd) (CPS*);         // command processor(cmd struct)
  uint  (*prtcmd) (uint, LBK *);  // command printer (ofst,cmd index)
  uint  minpars : 3 ;             // min pars expected
  uint  maxpars : 3 ;             // max pars allowed/expected 0-7

  //up to 8 types for each param for validation.
  // 0 - none, 1 start addr, 2 end addr, 3 register, 4 st range, 5 end range (4 and 5 are 1 and 2?

  uchar ptype[4];

  uint  namex   : 1 ;             // name expected/allowed

  uint  minadt  : 5 ;             // min ADT levels (< 31)
  uint  maxadt  : 5 ;             // max ADT levels (< 31)

  uint  defsze  : 6 ;             // default size, same as ssize in ADT
  uint  merge   : 1 ;             // can be merged with same command (adj or olap)
  uint  stropt  : 2 ;             // 1 = command with strings (e.g. setopt) 2 = calcstr             //change to first 'type' above ??

  cchar*  gopts;             // global options
  cchar*  opts;              // data levels
} DIRS;


/********************************
 * Interrupt subr sym names structure
 ********************************/

typedef struct
{
  uint  start : 8;     // start offset
  uint  end   : 8;     // end (0 - 0x5f)
  uint  p5    : 1;     // 8061 or 8065
  uint  num   : 1;     // with auto number
  cchar *name;
} NAMES;


/********************************
 * Preset sym names structure
 one for 8061, one for 8065
 ********************************/


typedef const struct
{

  uint fstart   : 3;       // start bit number (0-7)
  uint fend     : 8;       // end bit number
  uint addr     : 20;      // address as byte
  uint olap     : 1;       // SFR overlaps with next byte(s)
  cchar *symname;          // name

} DFSYM;






/********************************
 * Autonumbered sym names structure
 ********************************/

typedef struct
{                      // auto numbered symbol names
 int   mask;           // permission mask (a char, against cmdopts)
 uint  fendin  : 6;    // as per ADT
 uint  fendout : 6;    // sign flags for func and tab lookup subrs
 cchar *pname;
 }  AUTON;


/********************************
 * signature structure
 ********************************/

typedef struct xsig
{
// struct xptn *ptn;      // pointer to signature pattern
 uint  start;             // start address of sig found
 uint  end;               // end of signature - only really used to stop multiple pattern matches and overlaps
 uint  hix;               // sig handler index
// uint  done   : 1 ;     // processed marker - for debug really
// uint  skp    : 4 ;     // skipped opcodes at front
 uint novlp : 1;
 uint par1  : 4;
 uint  vx[NSGV];         // 16 saved values

 char name [16];
} SIG;


typedef struct xptn
{
 uint  *sig;                                   // actual sig pattern
 cchar *name;                                  // name of pattern
 void  (*pr_sig) (SIG *, SBK*);                // sig processor or zero if none
 uint size  : 16;                              // no of instructions in pattern
 uint vflag  : 4;                              // for encoded addresses
} PAT;


typedef struct bpx
{
uint  datbnk;         // currently selected data bank - always 8 if single bank
uint  codbnk;         // currently selected code bank - always 8 if single bank
uint  rambnk;         // currently selected RAM bank  - always 0 for 8061
} BASP;



// need all variables in here to pass around, as malloced struct.

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






























#endif

