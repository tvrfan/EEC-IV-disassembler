
/*******************************************************************
common declarations for 'core' code modules (disassembly)
**********************************************************************

 * NOTE - This code assumes 'int' is at least 32 bit (4 bytes),
 * 
 * On some older compilers this would require the use of LONG instead of INT.
 * code here also uses a lot of pointer casts. Should be fine on modern compiler,
 * but stated here just in case
 *
 * On Win32 builds (Codeblocks via mingw) int=long=void* all at 32 bits.
 * On Linux builds (CodeBlocks via gcc amd64)   int at 32 bits, long=void* at 64 bits.
 * 
 * Bin addresses throughout are composite, 20 bit (typically in an int) - bank in 0xf0000 + 16 bit address.
 * This code uses a pointer in a BANK structure to map into the binary file for best speed.
 * This is therefore bank order independent.
 *
 * 8065 manual says that bank is an extra 4 bits,  (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could be 1 Mb (1048576), but biggest bins seem to be 256k (= 0x40000), or 4 banks.
 * This code can handle the full 16 banks with only changes to the initial bank setup subroutines
 * required (detect and check).  Aparat from this, all code uses bank+address everywhere, but not tested beyond 4 banks
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
#define PFUNC     (cmdopts & F)      // auto function and table names
#define PSTRS     (cmdopts & G)      // auto subroutine names for function and table lookups
#define P8065     (cmdopts & H)      // 8065 codeset (can be set internally)
#define PLABEL    (cmdopts & L)      // auto label names (lab)
#define PMANL     (cmdopts & M)      // full manual mode

#define PINTF     (cmdopts & N)      // auto intrp func naming
#define PPROC     (cmdopts & P)      // auto proc names (xfunc, vfunc, intfunc)
#define PSIG      (cmdopts & S)      // auto Lookup Signature subroutine detect

#define PDEFLT    (P|N|S|C|F|G)      // default - procnames,intrnames,sigs, src

#define MAXLEVS   16                 // max command (CADT) levels
#define ANLPRT    5                  // define the print phase of anlpass
#define STKSZ     10                 // size of fake stack(s)  
#define EMUGSZ    5                  // size of emulate arguments list  
   
   
//#define MAXSCANS  2000             // max no of scan requests - to stop loops
//#define RETLEVS   10               // levels to check back on scans
//#define INSTSZ  512

// command values.  Up to 31 commands (32 is cmd flag, can move it)

#define C_DFLT   0  // print, mergable.
#define C_BYTE   1
#define C_WORD   2
#define C_LONG   3
#define C_TEXT   4
#define C_VECT   5 
#define C_ARGS   6

#define C_TABLE  7   // structs, not mergeable, overide group 1
#define C_FUNC   8
#define C_STCT   9
#define C_TIMR   10

#define C_CODE   11    // separate command chain
#define C_XCODE  12

#define C_SUBR   13     // non print
#define C_SCAN   14

#define C_CMD    32      // by command (can't change or merge)

// jump and scan request types (and can have C_CMD for user commanded)

#define J_RET    0      // return opcodes  
#define J_INIT   1
#define J_COND   2      // conditional jump ('if') 
#define J_STAT   3      // 'static' jump (goto)
#define J_SUB    4      // subroutine call
#define J_ELSE   5


// opcode main definition - called from INDEX structure, not opcode value

typedef struct            // OPCODE definition structure
 {
  uint sigix : 7;         // signature type (='fingerprint')
  uint sign  : 1;         // signed prefix allowed (goes to next entry)
  uint p65   : 1;         // 8065 only
  uint pswc  : 1;         // op changes PSW (for src printout of conditional ifs)
  uint nops  : 2;         // number of ops   (max 3)
  uint wop   : 2;         // write op index  (max 3) useful for printouts
  uint wsz   : 3;         // write op size
  uint rsz1  : 3;         // read op sizes   (as CADT)
  uint rsz2  : 3;
  uint rsz3  : 3;

  void (*eml) (struct sbk *, struct inst *);    // emulation func
  const char name [6];           // opcode name
  const char  *sce;              // high level code description, with operand markers
 } OPC;


typedef struct    // malloced blocks tracker
{
void *blk;        // pointer to block
size_t size;      // size of block
} MBL;


/******************************************
* Additional data struct - can chained together
* used by data structs and subrs for inline params

* when spf set, use addr as multiple register holder,
and ssize and pfw as size holders. funcs need 1 register
as address, 1 as answer, 2 sizes
tables need extra colsize register (3 regs)


**********************************************/

typedef struct cadt {   // size = 24 (6 int) in amd64
struct cadt *next;

float  fdat;          // full float for divisor (float = int for size !!)
int    data;          // addr, reg, offset, etc. int to allow for -ve

uint dreg   : 8 ;     // data destination register for data - for arg processing

uint cnt    : 5 ;     // (O) repeat count, 31 max
uint ssize  : 3 ;     // (Z) 1,2,3 unsigned, 5,6,7 signed - for byte,word,long
                      // 4 is no print
uint pfw    : 3 ;     // (P) print min fieldwidth 1-8 (0-7)
uint dcm    : 1 ;     // (D) decimal print format if set, hex otherwise

uint name   : 1 ;     // (N) look for symbol name
uint newl   : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr  : 1 ;     // (R) value is an addr/pointer to symbol (R) (bank is valid)
uint foff   : 1 ;     // data is an offset address (with bank) 

uint enc    : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)

//uint spf    : 1 ;     // (F) subr special function type. cmd only (use cnt for up to 31 types, 10 used)     

} CADT;




// main CHAIN struct to hold stuff for binary chop searches etc
// void* provides for different structs to be used in each chain
// with defined handlers for free mem, compare, and equals
// chain ptr allows same subrs to work for different chains

typedef struct chain                           // done like a class with subroutines by chain type
{
short num;                                     // no of (used) elements;
short pnum;                                    // no of pointers in array.
short asize;                                   // allocation size for pointer blocks
short esize;                                   // entry (struct) size  - ZERO if remote chain
short DBGmax;                                  // max no of ptrs allocated   for debug
short DBGallocs;                               // no of reallocs   for debug
void  **ptrs;                                  // ptr to array of pointers
void  (*cfree) (void*);                        // block free (by struct pointer)
int   (*comp)  (struct chain *, int, void*);   // compare,  for binary chop
int   (*equal) (struct chain *, int, void*);   // equals,  for finer checks and inserts
} CHAIN;




/********************************
 * RBASE list structure
 * map register to fixed address
 * only one value per register allowed
 ********************************/

typedef struct
{

  uint    val;          // address value pointed to, including bank
  uint    start;
  uint    end;           // valid for these addresses (= 0 for all)
  ushort  reg;           // register (as address)
  uint   inv : 1;       // invalid flag  (value changes in code)
  uint   cmd : 1;       // added by cmd (not changeable)
 // uint   blk : 1;       // add as part of rbase block (via signature)  
} RBT;



 /********************************
 *   symbol table structure
 ********************************/

// move autonumber into here, where it should be.
// use separate chains, but keep flags for write and bit for ease of logic
// to make comparsions faster and simpler in chain, addb is combination
// of address, bitno, and size  4|16|4  bank, address, bitno/size.

typedef struct
{

 char   *name;
 uint    addb;           // actual sym address + bitno 4|16|4 bank|address|bitno + 1
 uint    start;          // valid only between start and end (0 for all)
 uint    end;

 uint   noprt  : 1 ;    // printed by one of other outputs (prt_xx), suppress sym print
 uint   cmd    : 1 ;    // set by dir cmd, do not overwrite
 uint   xnum   : 5 ;    // autonumber symbol type if > 0  (31)
 uint   writ   : 1 ;    // write symbol if set (read if not)
 uint   tsize  : 6 ;    // size of symbol name (for mallocs and free)

 } SYT;
 
typedef struct lbk        // command header structure
{
CADT *adnl;                 // additional cmd structs (chained)
uint term   : 2;            // data command has terminating byte(s)
uint cmd    : 1;            // added by user command, guess otherwise
uint size   : 8;            // for args and struct sizes
uint start  : 20;           // start address (inc bank)

uint fcom   : 5;            // command index
uint acmt   : 4;            // autocomment type (e.g. "not called/ never gets here ?")
uint newl   : 1;            // newline in printout (requested in CADT chain)
uint end    : 20;           // end address (inc bank)

} LBK ;
 
/************************
operand tracking for printouts and anlysis
OPS has 2 sets of 4 entries (up to 3 op with indexed)
***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of register contents or an immediate value 
 uint  addr ;            // actual operand address (register mostly)

 uint ssize   : 3;      // Read size  as CADT above
 uint wsize   : 3;      // Write size, zero if no write (can be different to read size)
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a register  value
 uint bit     : 1;      // this is a bit value
 uint adr     : 1;      // this is an address (jumps - includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with mask and sign)
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print square brackets
 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found  (we know if it's read or write....) 
 uint symval  : 1;      // specal flag for rbs and offset (sym address in VAL, not addr)
} OPS;




// could put current scan block in here ??

typedef struct inst
{
  OPS   opr[5];           // up to 5 operands. [4] is for SAVED operands, indirect and indexed. (also specials ?)
  uint  opcix   : 8;      // opcode index into opcode table (max 112)
  uint  ofst    : 20;     // address of opcode, inc bank

  uint  numops  : 3;      // number of operands - NOT ALWAYS same as opl (e.g. for BIT jumps)
  uint  opcsub  : 2;      //  0 = register (default) 1=imediate, 2 = indirect, 3 = indexed
  uint  wop     : 2;      // write op index
  uint  inc     : 1;      // autoinc (op[1]) only with indirect (= word, +2)

  uint  bnk     : 1;      // this opcode has a BANK CHANGE prefix - seems to override both code and data banks for opcode
  uint  bank    : 4;      // bank override for this operand.   (0-15) may be different to ofst bank (eg. a jump)
  uint  psw     : 6;      // psw AFTER this operation

} INST;



typedef struct cmnt
 {
   char  *ctext;              // comment text
   INST  *minst;              // where ops are
   uint  ofst ;   //: 20;        // next addr for comment
 } CMNT;



// subroutine structure 
// go back to plain cadt for user cmd.....can't do variable args this way.
// move autonumber to symbol (where it should be)

typedef struct
{
 CADT *adnl;              // special functions, arguments, etc.
 struct xsig  *sig;       // sig attached
 uint  cmd    : 1;        // set by command, no mods allowed
 uint  spf    : 5;        // subr has special func/signature 
 uint  start  : 20;       // start address (inc bank)
 uint  end    : 20;       // TEST !!
 } SXT;

/********************************
 *   Jump list structure 
 ********************************/

typedef struct jt
{

  uint   jtype : 4;     // type (from scan)
  uint   bswp  : 1;     // bank change
  uint   size  : 3;
  uint   from  : 20;      // from address

  uint   back  : 1;     // backwards jump  
  uint   uco   : 1;     // not clean (jumps out)
  uint   uci   : 1;     // not clean  (jumps in)
  uint   cbkt  : 1;     // add close bracket
  uint   retn  : 1;     // a jump to or a direct 'ret' opcode

  uint   cmpcnt : 3;    // count back to previous compare (if set)
  uint   to     : 20;

 } JLT;

typedef struct sbk {          // scan block structure

struct sbk *caller;           // caller (go back to pointer for stack builds) 


uint  type       : 3;          // 1 - cond, 2 = stat, 4 = sub
uint  gap        : 1;          // picked up by gap scan
uint  start      : 20;         // start address

uint  scnt       : 8;         // temp for scan checks
uint  curaddr    : 20;        // current addr - scan position (inc. start)

uint  nodata     : 1;         // don't add data (for tables and funcs)
uint  args       : 1;         // this (proc) is an arg getter
//int   popcnt     : 4;         // popcount (for varargs checks) + or - 7
uint  nextaddr   : 20;        // next addr (opcode) or end of block.

uint  cmd        : 1;         // added by user command, guess otherwise
uint  stop       : 1;         // scan or emul complete
uint  inv        : 1;         // invalid opcode or flag in scan
uint  scanning   : 1;         // scan processing (do jumps, scans etc) 
uint  emulating  : 1;         // emulating (with dummy scan block)
uint  emulreqd   : 1;         // mark block as emu required (every time) for args (here)
uint  php        : 1;         // block uses pushp (for multibanks)

uint  hold       : 1;         // don't scan until end.
uint  substart   : 20;        // start address of related subroutine (for names and sigs)

// add a hold (for pushed addresses ?)

}  SBK ; 


// need both these next structs to be able to do both pops and arg tracing.............


typedef struct regstx
 {
   uint ssize   : 3;       // size last used, as CADT
   uint popped  : 1;       // popped reg
   uint arg     : 1;       // argument reg
   uint enc     : 4;       // encoded type
   uint reg     : 20;      // register (probably won't need all 20 - 13 ?)
   uint sreg    : 20;      // source register (i.e. the popped one) for CADT
   uint data    : 20;      // dreg for encode or data

  // uint addr    : 20;      // which arg this is attached to ? 
   // bank in here ??
   // add encoded flag/type ?
   
 } RST;


// override register value at x.  For func answers, encoded addrs etc.

typedef struct ovrxx
{
 //uint ssize : 3;
 uint reg : 12;       // may allow ram values in future ??
 uint ofst : 20;
 uint val;
}  OVR;

// and a psw source for printouts ????






//   Fake Stack - for extra checks - probably do need this.....

// TYPES 1-call 2-imd push 3-pushp (4 ?std push)
// pop set if popped (and register)
// 2 stacks to keep track of unloaded addresses (7aa1 in A9L)


typedef struct stk
{                              // fake stack struct

//SBK  *blk;                        // scan block for this block 
uint  popped     : 1;             // register entry is popped, cleared when pushed back             
uint  reg        : 8;             // register (last) popped to
uint  origadd    : 20;            // original value in nxtadd - for args detect
uint  type       : 3;             // 1 call, 2 imd, 3 psw
uint  psw        : 6;             // PSW value
uint  newadd     : 20;            // current addr value (at push)
}  FSTK;




 
typedef struct             // command holder for cmd parsing 
{
  CADT  *adnl;
  struct xsig *sig;        // SIG not declared yet ? 
  uint   posn;               // char posn/column (for errors)
  uint   maxlen;
  uint   lineno;

  int   p[8];             // 8 params max (allow for negative values)
  uint  npars  : 3;       // num of pars in first number set (for error checks)
  uint  fcom   : 5 ;
  uint  term   : 1;       // terminating byte
  uint  levels : 5;       // additonal levels (after the colon)
  uint  newl   : 1;       // split print somewhere (=newline)
  uint  decdft : 1;
  uint  size   : 8;       // command size (up to newline if a '|' char)
  char  symname[33];      // max 32 chars ?

 } CPS;
 
/********************************
 *  Input Commands structure
 ********************************/

 typedef struct drs                  // command definition
{ //const char com[8];                 // command string
  int   (*setcmd) (CPS*);            // command processor(cmd struct)
  uint  (*prtcmd) (uint, LBK *);     // command printer (ofst,cmd index)
  uint  maxlev   : 5 ;               // max CADT levels (31)
  uint  maxpars  : 3 ;               // max pars allowed/expected 0-7
  uint  startpos : 3 ;               // where 'start address' is (1 2 3 4 or 0 = none)
  uint  end_ex   : 1 ;               // end address expected
  uint  name_ex  : 1 ;               // name expected/allowed

  uint  ovrsc  : 5 ;                 // overrides by 'score'
  uint  merge  : 1 ;                 // can be merged with same command (adj or olap)
  uint  decdft : 1 ;                 // default to deimal print
  const char*  opts;
} DIRS;

/********************************
 * Interrupt subr sym names structure
 ********************************/

typedef struct
{ ushort start;
  ushort end;
  uint  p5 : 1;       // 8061 or 8065
  uint num : 1;       // with auto number
  const char *name;
} NAMES;


/********************************
 * Preset sym names structure
 ********************************/

typedef const struct
{
  uint p81 : 1;          // 8061
  uint p85 : 1;          // 8065
  uint wrt : 1;          // write symbol
  uint bit : 1;          // has bit if set, size otherwise
  uint bitno : 4;        // bit number if bit set (0-15)
  short  addr;           // address
  const char *symname;   // name

} DFSYM;

typedef struct
{                     // auto numbered symbol names
 int mask;            // permission mask (a char, against cmdopts)
 uint sizein  : 3;    // as per CADT
 uint sizeout : 3;     // sign flags for func and tab lookup subrs
 const char *pname;
 }  AUTON;

// end of header


#endif
