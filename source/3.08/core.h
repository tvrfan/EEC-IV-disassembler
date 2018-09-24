
/*******************************************************************
common declarations for 'core' code modules (disassembly)
**********************************************************************

 * NOTE - This code assumes 'int' is at least 32 bit (4 bytes),
 * 
 * On some older compilers this would require the use of LONG instead of INT.
 * code here also uses a lot of casts.

 * On Win32 builds (Codeblocks mingw) int=long=void*  all at 32 bits.
 * On Linux builds (CodeBlocks gcc amd64)   int at 32 bits, long=void* at 64 bits.
 * 
 * Bin addresses throughout are composite, 20 bit (in an int) - bank in 0xf0000 + 16 bit address.
 * this code uses a pointer in BANK structure to point into binary file for best speed.
 *
 * 8065 manual says that bank is an extra 4 bits,  (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could be 1 Mb (1048576), but biggest bins seem to be 256k (= 0x40000), or 4 banks.
 * 
 * This code can handle the full 16 banks with only changes to read_bin subroutine, where bank structure
 * is set up.  After this, code uses bank+address everywhere. (but not tested beyond 4 banks)
 *********************************************************/

#ifndef _XCOREX_H
#define _XCOREX_H 1

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <sys/stat.h>
#include  <stdarg.h>
#include  <string.h>
#include  <math.h>

#include  "shared.h"


#define NC(x)  sizeof(x)/sizeof(x[0])     // no of cells in struct 'x'

#define PCORG  0x2000     // standard offset for all EEC bins

// options (as bitmask letters in an int)

#define PSSRC     (cmdopts & C)      // source code
#define PDCM      (cmdopts & X)      // print variables in decimal  (hex default was X)
#define PFUNC     (cmdopts & F)      // auto function and table names
#define PSTRS     (cmdopts & G)      // auto subroutine names for function and table lookups
#define P8065     (cmdopts & H)      // 8065 codeset (can be set internally)
#define PLABEL    (cmdopts & L)      // auto label names (lab)
#define PMANL     (cmdopts & M)      // full manual mode

#define PINTF     (cmdopts & N)      // auto intrp func naming
#define PPROC     (cmdopts & P)      // auto proc names (xfunc, vfunc, intfunc)
#define PSIG      (cmdopts & S)      // auto Lookup Signature subroutine detect

#define PDEFLT    (P|N|S|C|F|G)        // default - procnames,intrnames,sigs, src

#define MAXLEVS   16                 // max command (CADT) levels
#define ANLPRT     5                 // define the print phase of anlpass
#define NPARS     16                 // num bytes in PARS block


// command values

#define C_DFLT   0     // command values. Up to 32 commands (32 is cmd flag, can move it)
#define C_BYTE   1
#define C_WORD   2
#define C_TEXT   3
#define C_TABLE  4
#define C_FUNC   5
#define C_STCT   6
#define C_TIMR   7
#define C_VECT   8     // non merge but override (6-8)
#define C_ARGS   9

#define C_SUBR   10
#define C_XCODE  11
#define C_CODE   12
#define C_SCAN   13

#define C_CMD    32      // by command (can't change)

// scan block types

#define S_RETJ   0       // return opcodes  
#define S_SJMP   1       // 'fixed' or static jump
#define S_CJMP   2       // conditional jump  ('if') 
#define S_CSUB   3       // subroutine call
#define S_JSUB   4       //  special - a jump to a subr


// opcode main definition - called from INDEX structure, not opcode value

typedef struct            // main OPCODE struct
 {
  uint sigix : 7;         // type index for signatures
  uint sign  : 1;         // signed prefix allowed (to next entry)
  uint p65   : 1;         // 8065 only
  uint pswc  : 1;         // changes PSW (for src printout of conditional ifs)
  uint nops  : 2;         // number of ops   (max 3)
  uint wop   : 2;         // write op index  (max 3)
  uint wsz   : 3;         // write op size
  uint rsz1  : 3;         // read op sizes   (as CADT)
  uint rsz2  : 3;
  uint rsz3  : 3;

  void (*eml) (struct sbk *);    // emulation func
  const char name [6];           // opcode name
  const char  *sce;              // high level code description (with op markers)
 } OPC;


typedef struct    // malloced blocks tracker
{
void *blk;        // pointer to block
size_t size;      // size of block
} MBL;


/******************************************
* Additional data struct - can chained together
* used by data structs and subrs for inline params
**********************************************/

typedef struct cadt {   // size = 24 (6 int) in amd64
struct cadt *next;

float  divd;          // full float for divisor
int    addr;          // for addresses used in encode, offset etc.(also bitno)

uint dreg   : 8 ;    // data destination register for data

uint ssize  : 3 ;    // 1,2,3 unsigned, 4,5,6 signed - for size 1,2,4 bytes
uint pfw    : 3 ;    // print min fieldwidth 1-8 (0-7)
uint dcm    : 1 ;    // decimal print format, hex is default
uint vect   : 1 ;    // value is a vect to a subr
uint name   : 1 ;    // look for symbol name

uint cnt    : 5 ;    // repeat count, 32 max
uint enc    : 3 ;    // encoded data type  1-7         (only subrs)
uint foff   : 1 ;    // fixed offset (using addr)

uint nodis  : 1 ;    // don't print this entry in commands (but active)
uint disb   : 1;     // this entry disabled, skip over print (used for F:)
uint pget   : 1 ;    // this is a par getter (for encdx to find later !)
uint newl   : 1 ;    // break printout with newline here (=at this level)

} CADT;


// main CHAIN struct to hold stuff for binary chop searches etc
// void* provides for different structs to be used in each chain
// with defined handlers for free mem, compare, and equals

typedef struct chain                           // done like a class with subroutines by chain type
{
short num;                                     // no of (used) elements;
short pnum;                                    // no of pointers in array.
short asize;                                   // allocation size for pointer blocks (50 is default)
short esize;                                   // entry (struct) size  - ZERO if remote chain
short DBGmax;                                  // max no of ptrs allocated   for debug
short DBGallocs;                               // no of reallocs   for debug
void  **ptrs;                                  // ptr to array of pointers
void  (*cfree) (void*);                        // block free (by pointer)
int   (*comp)  (int, void*);                   // compare,  for binary chop
int   (*equal) (int, void*);                   // equals,  for finer checks and inserts
} CHAIN;


/********************************
 * RBASE list structure
 * map register to fixed address
 ********************************/

typedef struct
{

  int    val;          // address value pointed to, including bank
  int    start;
  int    end;           // valid for these addresses (= 0 for all)
  short  reg;           // register (as address)
  uint   inv : 1;       // invalid flag  (value changes in code)
  uint   cmd : 1;       // added by cmd (not changeable)
  uint   blk : 1;       // add as part of rbase block (via signature)  
  uint   cnt : 4;       // count of changes
} RBT;

 /********************************
 *   symbol table structure
 ********************************/

typedef struct
{

 char  *name;
 int   add;
 int   start;
 int   end;

 char   bitno  ;       // bit 0 to 31, signed.  -1 for whole word/byte
 uint   used  : 1 ;    // 'used' marker set if name used
 uint   prtd  : 1 ;    // printed by one of other output (prt_x), suppress sym print
 uint   cmd   : 1 ;    // set by dir cmd, do not overwrite
 uint   anum  : 1 ;    // autonumber type (prevents updates)
 uint   writ  : 1 ;    // write symbol if set (read if not)
 } SYT;
 

 
/************************
operand tracking for printouts and anlysis
OPS has 2 sets of 4 entries (up to 3 op with indexed)
***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val;               // value of register contents or an immediate value
 int  addr;              // actual operand address
 RBT  *rbs;              // rbase pointer if reg is an rbase
 SYT  *sym;              // symbol pointer if exists

 uint ssize   : 3;      // Read size  as CADT above
 uint wsize   : 3;      // Write size, zero if no write (can be different to read size)
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a register  value
 uint bit     : 1;      // this is a bit value
 uint adr     : 1;      // this is an address (jumps - includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with mask and sign)
 uint stk     : 1;      // this is a STACK/ASTACK register
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print square brackets
} OPS;


typedef struct
{
  OPS  opr[4];            // up to 4 operands
  int opcix;              // opcode index
  int  ofst;              // address of opcode, inc bank
  int nextaddr;           // add to make arg additions easier....

  uint inum    : 8;      // instance number (for checking distance)
  uint numops  : 3;      // number of operands - NOT ALWAYS same as opl (e.g. BIT jumps)
  uint inx     : 1;      // indexed  opcode  (op[1] , op[0] is imd)
  uint inr     : 1;      // indirect opcode  (op[1])
  uint inc     : 1;      // autoinc  opcode  (op[1])
  uint imd     : 1;      // immediate opcode (op[1])   N.B. imds can be elsewhere too
  uint bnk     : 1;      // this has a BANK CHANGE prefix - seems to override both code and data banks for opcode
  uint bank    : 4;      // bank - new bank if flg set, otherwise, databank (0-16)  (add codebank ?)
} INST;

//*****  for data addresses found in code analysis 

typedef struct dd
{
  int start;
  int from;               // where called from ?
  int p1;                 // cols or
  int p2;                 // rows or size out for funcs
  int p3;                 // TEST for exta index value

  uint ssize    : 5;        // entry size (as CADT)
  uint imd      : 1;        // via imd opcode
  uint inx      : 1;        // via index opcode (rbase conv. to imd)
  uint inr      : 1;        // indirect (not allowed ?)
  uint inc      : 5;        // allow for up to 32 incs...

  uint vect     : 1;        // called from a PUSH (may be code or data)
  uint tab      : 1;        // table routine
  uint func     : 1;        // func routine
  uint rbt      : 1;        // rbase
  uint psh      : 1;        // PUSH
  uint dis      : 1;        // disable (for overlaps)
  uint cmd      : 1;        // by user cmd, no changes
} DDT;


typedef struct lbk        // command header structure
{
CADT *adnl;                // additional cmd structs (chained)
int  start;                // start offset
int  end;                  // end offset

uint size : 8;            // for args and struct sizes
uint fcom : 5;            // command index
uint term : 2;            // data command has terminating byte(s)
uint cmd  : 1;            // added by user command, guess otherwise
uint newl : 1;            // newline in printout requested in CADT chain
} LBK ;


typedef struct csig        // signature holder struct
{
 struct csig  *next;
 struct xsig  *sig;        // SIG*  same sig may be referenced more than once.
 int  caddr;               // where called (for multiple calls)
 int  dat1;                // general data holder = address
 int  dat2;                // may need another - reg data ? 2 is same size as pointer !
 uint par1   : 8;          // various - no of pars for multi level types
 uint lev1   : 3;          // up to seven levels
 uint lev2   : 3;          // for vpar types, which need 2 levels
 uint gpar   : 1;          // this is a getpars signature (for quick search)
 uint enc    : 1;          // this is an encode signature (for quick search) 
 uint l1d    : 1;          // level 1 done (cadt added)
 uint l2d    : 1;          // level 2 done (cadt added)
 uint disb   : 1;          // disable processing across all levels
}  CSIG;

// CADT and Signature holder for subroutines.
// reduces reduce data useage as most subrs will have a zero SXDT pointer

typedef struct pex
{
 CADT   *cadt;                    // MAIN CADT chain
 CSIG   *sigc;                    // SIGC chain
} SXDT;

// subroutine structure

typedef struct
{
 SXDT *sdat;              // intermediate holder (most will be NULL)
 int   start;             // start address (and bank)
 uint  nargs1 : 5;        // total size (bytes) of any params (max 32)
 uint  nargs2 : 5;        // test - for alternate param nums
 uint  xnam   : 5;        // for subroutine names (funcs and tabs)
 uint  cmd    : 1;        // set by command, no mods allowed
 uint  cname  : 1;        // name added by command - don't change it
 uint  psp    : 1;        // subr starts with a push pswflags (for address searches)
 uint  sgclc  : 1;        // recalc sigs from scratch (for encodes) 
 } SXT;


typedef struct hdls
{
struct hdls *next;
struct sbk *hblk;
} HOLDS;


// may be allow chain of scanpars to reduce number of scans ?? is it worth it ??


typedef struct sbk {         // scan structure
HOLDS *hchn;                 // chain of other blocks held by this block
struct sbk *caller;          // called from - for subroutine tracking
SXT    *sub;                 // Subroutine (new or existing - needed for sigs)

struct sbk *held;            // for debug, hold latest caller, can be flag otherwise this block on hold to another sbk block

ushort preg [NPARS];         // registers or ram saved             put scanpars in here directly
ushort pval [NPARS];         // and their values                   nearly all sbks have them anyway

int start;                   // start addr
int end;                     // where set, defines a stop point (for if-then conds)
int curaddr;                 // current addr - scan position (inc. start)
int retaddr;                 // RETURN  addr - for param tracking 
int nextaddr;

uint  ctype    : 4;          // scan type 
uint  cmd      : 1;          // added by user command, guess otherwise
uint  scanned  : 1;          // scan complete
uint  inv      : 1;          // invalid opcode or flag in scan - could be dodgy vect call

}  SBK ;

/********************************
 *   Jump list structure       replace with binary tree ?
 ********************************/

typedef struct
{
  int  from;              // from and to are addrs
  int  to;

  uint   jtype : 4;     // type (from scan)
  uint   jcnt  : 2;     // this jump size (bytes)
  uint   bswp  : 1;     // bank change
  uint   back  : 1;     // backwards jump  
  uint   uco   : 1;     // not clean (jumps out)
  uint   uci   : 1;     // not clean  (jumps in)
  uint   cbkt  : 1;     // add close bracket
  uint   retn  : 1;     // a jump to a 'ret' opcode

 } JLT;

 
typedef struct             // command holder for cmd parsing 
{
  CADT  *adnl;
  struct xsig *z;         // SIG  *z;  doesn't compile.... 
  int   p[4];             // 4 params max
  uint  npars  : 3;       // num of pars in p array (for error checks)
  uint  fcom   : 5 ;
  uint  term   : 1;       // terminating byte
  uint  levels : 8;       // additonal levels (after the colon)
  uint  newl   : 1;       // split print here (=newline)
  int   posn;             // char posn (for errors)
  int   size;             // command size (up to newline if a '|' char)
  char  symname[33];      // max 32 chars ?

 } CPS;
 
/********************************
 *  Input Commands structure
 ********************************/

 typedef struct drs                  // command definition
{ const char com[8];                 // command string
  int   (*setcmd) (CPS*);            // command processor(cmd struct)
  int   (*prtcmd) (int, int);        // command printer (ofst,ix)
  uint  maxlv  : 5 ;                 // max CADT levels (31)
  uint  maxps  : 3 ;                 // max front pars/addresses expected 0-4
  uint  vpos   : 3 ;                 // single addr to be verified
  uint  spos   : 3 ;                 // where 'start/end' pair is (1 2 3 4 or none)
  uint  namee  : 1 ;                 // name expected/allowed
  uint  ovrde  : 5 ;                 // overrides for 32 commands (bit mask)
  uint  merge  : 1 ;                 // can be merged with same command (adj or olap)
  const char*  opts;
} DIRS;

/********************************
 * Interrupt subr sym names structure
 ********************************/

typedef struct
{ short start;
  short end;
  uint  p5 : 1;       // 8061 or 8065
  uint num : 1;       // with auto number
  const char *name;
} NAMES;


/********************************
 * Preset sym names structure
 ********************************/

typedef const struct
{
 const char *symn;
 short  addr;
 short  bit1;
 uint pcx  : 2;       // 8061 = 1, 8065 = 2, both = 3;
 uint wrt : 1;        // write symbol

} DFSYM;

typedef struct
{                     // auto numbered symbol names
 int nval;            // last number allocated
 int mask;            // permission mask (against cmdopts)
 const char *pname;
 }  AUTON;

// end of header


#endif
