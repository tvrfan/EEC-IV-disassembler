
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
 * is set up.  After this, code uses bank+address everywhere. (not tested beyond 4 banks)
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

#define C_DFLT   0     // command offsets up to 32 commands at present (32 is cmd flag, can move it)
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
#define C_TST    64      // test flag (for gaps)

// scan block types

#define S_IJMP   1
#define S_SJMP   2
#define S_CJMP   3
#define S_ISUB   4
#define S_VSUB   5
#define S_CSUB   6
#define S_JSUB   7


// opcode main definition - called from INDEX structure, not opcode value

typedef struct            // main OPCODE struct
 {
  uchar sigix;            // type index for signatures
  uchar alx;              // alternate index - signed prefix (divs etc) & src jump swaps

  uint pswc  : 1;         // changes PSW (for src printout of conditional ifs)
  uint nops  : 2;         // number of ops   (max 3)
  uint wop   : 2;         // write op index  (max 3)
  uint wsz   : 3;         // write op size
  uint rsz1  : 3;         // read op sizes   (as CADT)
  uint rsz2  : 3;
  uint rsz3  : 3;

  int opt ;                      // opcode type flags - may disappear
  int eml;                       // change for index
// int (*eml) (struct sbk *);     // emulation func
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

typedef struct cadt {   // size = 16 (4 int) this for cmnd chain 21 bits in flags
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
uint enc    : 3 ;    // encoded data type  1-7
uint foff   : 1 ;    // fixed offset (using addr)

uint nodis  : 1 ;    // don't print this entry in commands (but active)
uint disb   : 1;     // this entry disabled, skip over print (used for F:)
uint pget   : 1 ;    // this is a par getter (for encdx to find later !)
uint split  : 1 ;    // break printout with newline at this level

} CADT;


// main CHAIN struct to hold stuff for binary chop searches etc
// void * allows different struct types to be fed in for searches etc.

typedef struct chain                           // done like a class with subroutines by chain type
{
short num;                                     // no of (used) elements;
short pnum;                                    // no of pointers in array.
short asize;                                   // allocation size for pointer blocks (50 default)
short esize;                                   // entry (struct) size  - ZERO if remote chain
short max;                                     // max no of ptrs allocated   for debug
short allocs;                                  // no of reallocs
void  **ptrs;                                  // ptr to array of pointers
void  (*cfree) (void*);                        // struct chain *, int);                          // block free (by index)
int   (*comp)  (int, void*);                   // binary chop compare
int   (*equal) (int, void*);                   // equals, for find operations
} CHAIN;


/********************************
 * RBASE list structure
 * map register to fixed address
 ********************************/

typedef struct
{
  short  radd;          // register (as address)
  int    rval;          // address value pointed to, including bank
  int    start;
  int    end;           // valid for these addresses (= 0 for all) TEST
  uint   inv : 1;       // invalid flag  (value changes in code)
  uint   cmd : 1;       // added by cmd
  uint   cnt : 4;       // count > 1 or cmd to make sure.....

} RBT;

 /********************************
 *   symbol table structure
 ********************************/

typedef struct
{
 int    add;
 char   *name;
 short  size  ;        // size of symnam string (in chars)

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
 int  val;               // value of register contents or an immediate    uns short ?
 int  addr;              // actual operand address
 RBT  *rbs;              // rbase pointer if reg is an rbase
 SYT  *sym;              // symbol pointer if exists

 uint ssize   : 3;      // Read size  as CADT above
 uint wsize   : 3;      // Write size, zero if no write (can be diff to read)

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
  const OPC *opl;         // pointer to opcodes
  OPS  opr[4];            // up to 4 operands
  int  ofst;              // address of opcode, inc bank
  int nextaddr;           // add to make arg additions easier....

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
 // others ??
 // add tables, vect etc ?
} DDT;


typedef struct lbk        // command header structure
{
CADT *adnl;                 // additional cmd structs (chained)
int  start;                // start offset
int  end;                  // end offset

uint size : 8;              // CPARS and struct (subr or data) sizes (use for word/byte ?)

uint fcom : 5;            // command index  - only 13 commands so far...
uint term : 2;            // data command has terminating byte(s)
uint cmd  : 1;            // added by user command, guess otherwise
uint split : 1;           // split printout
 //uint chk  : 1;         // do ptn analysis from here (immediate or absolute address)
} LBK ;


typedef struct csig        // preprocess if copied flag ?
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
 int   addr;              // start address (and bank)
 uint  nargs1 : 5;        // total size (bytes) of any params (max 32)
 uint  nargs2 : 5;        // test - for alternate param nums
 uint  xnam   : 5;        // for subroutine names (funcs and tabs)
 uint  cmd    : 1;        // set by command, no mods allowed  - guess otherwise
 uint  cname  : 1;        // name added by command - don't change it
 uint  psp    : 1;        // subr starts with a push pswflags (adds one to address searches)
 uint  sgclc  : 1;        // (re)calc sigs from scratch (for encodes) ???   TEST 

 } SXT;



/* add fake stack for subr which have arguments.

typedef struct stack
{

//struct sbk *cblk;     // caller for this entry   (reqd ?)
uint psw : 1;         // this is a PSW entry - need for 8065
uint reg : 8 ;        // register (when popped)
uint addr : 20;       // a full address (inc bank)
} STK;
*/





typedef struct spars
{

ushort preg [NPARS];         // registers or ram saved
ushort pval [NPARS];         // and their values

} SPARS;


typedef struct sbk {         // scan structure
struct sbk *hblk;            // holder of this block [single] to be scanned, or zero if not on hold
struct sbk *caller;          // called from - for subroutine tracking
SPARS  *pars;                // params for scan
SXT    *sub;                 // Subroutine (new or existing - needed for sigs)

int start;                   // start addr
int end;                     // where set, defines a stop point (for if-then conds)
int curaddr;                 // current addr - scan position (inc. start)
int retaddr;                 // RETURN  addr - for param tracking 
int nextaddr;

uint  type     : 4;          // scan type = 1 init jump, 2 static jump, 3 cond jump,
                             // 4 int subr 5 vect subr, 6 called subr, 7 jmp to subr
uint  htype    : 4;          // hold type (same as type)
uint  cmd      : 1;          // added by user command, guess otherwise
uint  scanned  : 1;          // scan complete
uint  inv      : 1;          // invalid opcode or flag in scan - could be dodgy vect call
uint  holder   : 1;          // this block has holds waiting upon it (not itself)
uint  stop     : 1;          // stop scanning flag  (separated from count to make simpler
//uint  tst      : 1;          // test or investigate mode
//uint  ckd      : 1;           // checked
}  SBK ;


// a block can be held ONLY by one holder....


/********************************
 *   Jump list structure
 ********************************/

typedef struct
{
  int  from;              // from and to are addrs
  int  to;

  uint   type : 4;    // type (from scan)
  uint   cnt  : 2;    // this jump size (bytes) for ELSE
  uint   uco  : 1;    // not clean (jumps out)
  uint   uci  : 1;    // not clean  (jumps in)
  uint   mult : 1;    // Multiple 'to' points
  uint   back : 1;    // Backwards jump
  uint   cbkt : 1;    // add close bracket
  uint   retn : 1;    // a jump to a 'ret' opcode, a subr return point
  uint   bswp : 1;    // bank swop
  uint   lstp : 1;    // loopstop
  uint   elif : 1;    // ELSE type goto
 } JLT;


typedef struct         // command holder for cmd parsing 
{
  CADT  *adnl;
  struct xsig *z;         // SIG  *z;  doesn't work due to include order - this does ! for special funcs 
  int   p[4];             // 4 params
  uint  npars : 3;        // num of pars in p array (for error checks)
  uint  fcom : 5 ;
  uint  term : 1;         // terminating byte
  uint  levels : 8;       // additonal levels (after the colon)
  uint  split : 1;
  int   posn;             // char posn (for errors)
  int   size;            // command size (up to newline if a '|' char)
//  int   size2;            // command size after a split (=newline) level
  char  symname[33];      // max 32 chars ?

 } CPS;
 
/********************************
 *  Input Commands structure
 ********************************/


// if maxad = 0 then command is a FLAGS type (e.g. opt)

 typedef struct drs                  // command definition
{ int cxom;                       //const char *com;                   // command string  was com[6]
  int   (*setz) (CPS*);              // command processor
  int   (*prtz) (int, int);          // command printer (ofst,ix)
  uint  maxlv  : 5 ;                 // max CADT levels (31)
  uint  maxps  : 3 ;                 // max front pars/addresses expected 0-4
  uint  spos   : 3;                  // where 'start' is (par1 2 3 4 or none)
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
  uint  p5 : 1;       // 8061 or 8065 ?
  uint num : 1;       // with number ?
  const char *name;
} NAMES;


// add a STACK marker uint as well ??

/********************************
 * Preset sym names structure
 ********************************/

typedef const struct
{
 const char *symn;
 short  addr;
 short  bit1;
 uint pcx  : 2;       // 8061 = 1, 8065 = 2, both = 3;
 uint wrt : 1;       // write symbol

} DFSYM;

typedef struct
{                     // auto numbered symbol names
 int nval;            // last number allocated
 int mask;            // permission mask (against cmdopts)
 const char *pname;
 }  AUTON;

// end of header


#endif
