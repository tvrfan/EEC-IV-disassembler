
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

#include  "shared.h"


#define PCORG  0x2000     // standard offset for all EEC bins

// option  defs (as bitmask)

#define PSSRC     (cmdopts & 1)         // C source code
#define PTABL     (cmdopts & 2)         // F auto table names (+func on old system)
#define PSTRS     (cmdopts & 4)         // G auto subroutine names for function and table lookups
#define P8065     (cmdopts & 8)         // H 8065 codeset (can be set internally)
#define PLABEL    (cmdopts & 0x10)      // L auto label names (lab)
#define PMANL     (cmdopts & 0x20)      // M full manual mode

#define PINTF     (cmdopts & 0x40)      // N auto intrp func naming
#define PPROC     (cmdopts & 0x80)      // P auto proc names (xfunc, vfunc, intfunc)
#define PSIG      (cmdopts & 0x100)     // S auto Lookup Signature subroutine detect
#define PFUNC     (cmdopts & 0x200)     //  auto function names
#define PACOM     (cmdopts & 0x400)     // do autocomments
#define PRSYM     (cmdopts & 0x800)     // do preset symbols
#define PSTPD     (cmdopts & 0x1000)     // set step D for 8065 (odd boundaries)

#define PDEFLT    (0x1C7)      // (P|N|S|C|F|G) default - procnames,intrnames,sigs, src

#define ANLFFS    4                  // find funcs, post scan (anlpass)
#define ANLPRT    5                  // print phase (anlpass)
#define STKSZ     10                 // size of fake stack(s)  
#define EMUGSZ    5                  // size of emulate arguments list  
   
#define NSGV      16                 // num sig values, but sig code allows for 32   

// command values.  Up to 31 commands (32 is cmd flag, can move it)

#define C_DFLT   0  // preset size data types.
#define C_BYTE   1
#define C_WORD   2
#define C_LONG   3
#define C_TEXT   4
#define C_VECT   5
 
#define C_ARGS   6   // struct data types
#define C_TABLE  7
#define C_FUNC   8
#define C_STCT   9
#define C_TIMR   10

#define C_CODE   11    // separate code command chain
#define C_XCODE  12

#define C_SUBR   13 
#define C_SCAN   14

#define C_SYM    17

#define C_CMD    32      // by command (can't change or merge)
#define C_SYS    128     // for system 'base generated' cmds  

#define P_NOSYM  256     //nosym for pp_hdr

// jump and scan request types (and can have C_CMD for user commanded)

#define J_RET    0      // return opcodes  
#define J_INIT   1
#define J_COND   2      // conditional jump ('if') 
#define J_STAT   3      // 'static' jump (goto)
#define J_SUB    4      // subroutine call
//#define J_ELSE   5      //not yet.
//#define J_OR     6

//error numbers
#define E_DUPL  1          // duplicate
#define E_INVA  2          // invalid address 
#define E_BKNM  3          // banks no match
#define E_ODDB  4          //odd address (WORD etc)
#define E_OVLP  5          //overlap
#define E_OVRG  6          //overlapping ranges
#define E_XCOD  7          // XCODE bans
#define E_SYMNAM   8       // syname replaced
//#define E_PAR   9


// chain index defines 

 #define CHJPF      0
 #define CHJPT      1
 #define CHSYM      2
 #define CHBASE     3
 #define CHSIG      4
 #define CHCODE     5
 #define CHDATA     6
 #define CHSCAN     7
 #define CHEMUL     8
 #define CHSBCN     9
 #define CHSGAP    10
 #define CHSUBR    11
 #define CHADNL    12
 #define CHANS     13
 #define CHRGST    14
 #define CHSPF     15
 #define CHPSW     16
// #define mblk     17


#define g_bank(x)    (x & 0xf0000)                            // get bank
#define nobank(x)    (x & 0xffff)                             // strip bank
#define bankeq(x,y)  ((x & 0xf0000) == (y & 0xf0000))         // address banks equal






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
  uint rsz1  : 3;         // read op sizes   (as ADT)
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
uint lasterr;                                  // for user commands etc (perhaps bit flags instead ???

//const char *ertxt;                            // for user error messages - could cheat and use this as errcode as well (< 32)
void  **ptrs;                                  // ptr to array of pointers

void  (*cfree) (void*);                        // block free (by struct pointer)
int   (*comp)  (struct chain *, uint, void*);  // compare,  for binary chop
int   (*equal) (struct chain *, uint, void*);  // equals,   for finer checks (e.g. inserts)
} CHAIN;


/******************************************
* Additional data struct - can chained together
* used by data structs and subrs for inline params
**********************************************/

// change this to foreign key type relationship


typedef struct adt {   // size = 24 (6 int) in amd64

void  *fid;           // foreign row id (to another CHAIN) 
float  fdat;          // full float for divisor (float = int for size !!)

uint dreg   : 8 ;     // data destination register for data - for arg processing
int  seq    : 8 ;     // sequence number, set negative for new query
int  data;            // addr, reg, offset, etc. int to allow for -ve

uint cnt    : 5 ;     // (O) repeat count, 31 max
uint ssize  : 3 ;     // 1,2,3 unsigned, 5,6,7 signed - for byte,word,long
uint pfw    : 5 ;     // (P) print min fieldwidth (0-31)
uint dcm    : 1 ;     // (D) decimal print format if set, hex otherwise

uint fnam   : 1 ;     // (N) look for symbol name
uint newl   : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr  : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint bank   : 1 ;     // data holds a valid bank (in 0xf0000)
uint foff   : 1 ;     // data is an offset address 
uint ans    : 1 ;     // this is ANSWER definition (separate item, for verification)
uint enc    : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)

} ADT;


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
  uint    inv : 1;       // invalid flag  (value changes somewhere in code)
  uint    cmd : 1;       // added by cmd (not changeable)
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

// move autonumber into here, where it should be.
// use separate chains, but keep flags for write and bit for ease of logic
// to make comparsions faster and simpler in chain, addb is combination
// of address, bitno, and size  4|16|4  bank, address, bitno/size.
// bitno is max of 7 as split by byte in insert and find.

typedef struct
{

 char   *name;
 uint    addb;           // actual sym address + bitno 4|16|4 bank|address|bitno + 1
 uint    rstart;          // valid only between start and end (0 for all) (bit flag??)
 uint    rend;

 uint   noprt  : 1 ;    // printed by one of other outputs (prt_xx), suppress sym print
 uint   cmd    : 1 ;    // set by dir cmd, do not overwrite
 uint   sys    : 1 ;    // system autogenerated
 uint   xnum   : 5 ;    // autonumber symbol type if > 0  (31 max, for subrs)
 uint   writ   : 1 ;    // write symbol if set (read if not)
 uint   flags  : 1 ;    // this sym has bitnames declared also
 uint   immok  : 1 ;    // sym is Ok with immediate ops
 uint   tsize  : 8 ;    // size of symbol name +null (for mallocs and free)

 } SYM;
 
typedef struct lbk        // command header structure
{

 uint cmd    : 1;            // added by user command, guess otherwise
 uint sys    : 1 ;           // system autogenerated
 uint size   : 8;            // TOTAL size of data ADT additions
 uint start  : 20;           // start address (inc bank)

 uint term   : 2;            // data command has terminating byte(s) 0-3
 uint argl   : 1;            // use args layout
 uint cptl   : 1;            // use compact layout
 uint fcom   : 5;            // command index
 uint end    : 20;           // end address (inc bank)

 uint opcsub  : 2;           // from inst for verifcation
 uint soff    : 4;           // small offset for indexed 0-16  ??
 uint from   : 20;           // where added from (not cmd items, which are zero)

} LBK ;
 
/************************
operand tracking for printouts and anlysis
OPS has 2 sets of 4 entries (up to 3 op with indexed)
***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of register contents or an immediate value 
 uint addr ;            // actual operand address (register mostly)

 uint ssize   : 3;      // Read size  as ADT above
 uint wsize   : 3;      // Write size, zero if no write (can be different to read size)
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a register  value
 uint bit     : 1;      // this is a bit value
 uint adr     : 1;      // this is an address (for jumps - includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with mask and sign)
 uint neg     : 1;      // offset address is negative
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print square brackets
 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found  (we know if it's read or write....) 
 uint symi    : 1;      // 1 if sym allows immediates
} OPS;




// could put current scan block in here ??

typedef struct inst
{
  OPS   opr[5];           // up to 5 operands. [4] is for SAVED operands, indirect and indexed.
  uint  opcix    : 8;      // opcode index into opcode table (max 112)
  uint  ofst     : 20;     // address of opcode, inc bank

  uint  numops   : 3;      // number of operands - NOT ALWAYS same as opl (e.g. for BIT jumps)
  uint  opcsub   : 2;      //  0 = register (default) 1=imediate, 2 = indirect, 3 = indexed
  uint  wop      : 2;      // write op index
  uint  inc      : 1;      // autoinc (op[1]) only with indirect (= word, +2)
 // uint  nextaddr : 20;     // address of NEXT opcode, inc bank

  uint  bnk     : 1;      // this opcode has a BANK CHANGE prefix - seems to override both code and data banks for opcode
  uint  bank    : 4;      // bank override for this operand.   (0-15) may be different to ofst bank (eg. a jump)
  uint  psw     : 6;      // psw AFTER this operation

} INST;


// structure finder (tabs and funcs)
// argument holder for branch analysis subrs

typedef struct  sfx
 {
   uint rg[2];
   uint ans[2];
 }  SFIND;  

typedef struct cmnt
 {
   char  *ctext;              // comment text
   INST  *minst;              // where ops are
   uint  ofst    : 20;        // next addr for comment
   uint  nl      : 1;         // does cmt begin with newline      
 } CMNT;


// subroutine structure 

typedef struct
{
 uint  cmd    : 1;        // set by command, no mods allowed
 uint  sys    : 1 ;       // system autogenerated
 uint  cptl   : 1;        // use compact layout
 uint  end    : 20;
 uint  size   : 8;        // for any attached args (ADT)
 uint  start  : 20;       // start address (inc bank)
 } SUB;


// for subroutines with special functions
// only one per subroutine so can use address 
// but standard is to use a void * fid;

typedef struct xspf
 {
  void *fid;               // points to SXT
  uint addrreg  : 8;
  uint sizereg  : 8;

  uint sizein   : 3;       // may not need this...
  uint sizeout  : 3;       // or this
  uint spf      : 5;        
 } SPF;


/********************************
 *   Jump list structure 
 ********************************/

typedef struct jlt
{
//  SBK *sfrom;
//  SBK *sto;
// struct jlt  *parent;    //for heirarchy ??

  uint   jtype    : 4;     // type (from scan)
  uint   bswp     : 1;     // bank change
  uint   size     : 3;
  uint   fromaddr : 20;      // from address

uint done  : 1;
uint jelse : 1;
uint jor  : 1;

  uint   back     : 1;     // backwards jump  
 // uint   uco      : 1;     // not clean (jumps out)
 // uint   uci      : 1;     // not clean  (jumps in)
  uint   obkt     : 1;       // add open bracket (i.e. swop logic for goto)
  uint   cbkt     : 1;     // add close bracket

 uint jwhile  : 1;
 uint jdo     : 1;
  uint   retn     : 1;     // a jump to or a direct 'ret' opcode (to be ditched...??)

  uint   cmpcnt   : 3;    // count back to previous compare (if set)
  uint   toaddr   : 20;   // this will always be a new SBK, unless a retn

 } JMP;






typedef struct sbk {          // scan block structure

struct sbk *caller;           // caller (for stack builds) //but this could be multiple.......
// JLT*  parent;

uint  type       : 3;          // 1 - cond, 2 = stat, 4 = sub
uint  gapchk     : 1;          // picked up by gap scan
uint  topgap     : 1;          // top level gap scan (for comment)
uint  start      : 20;         // start address

uint  scnt       : 8;         // temp for scan checks
uint  chscan      : 1;
uint  curaddr    : 20;        // current addr - scan position (inc. start)

uint  nodata     : 1;         // don't add data (for tables and funcs)
uint  args       : 1;         // this (proc) is an arg getter
uint  scanning   : 1;         // scan processing (do jumps, scans etc) 
uint  emulating  : 1;         // emulating (with dummy scan block)
uint  emulreqd   : 1;         // mark block as emu required (every time called) for args (here)
uint  nextaddr   : 20;        // next addr (opcode) or end of block.

uint  cmd        : 1;         // added by user command, guess otherwise
uint  sys    : 1 ;    // system autogenerated
uint  stop       : 1;       // scan complete
uint  inv        : 1;         // invalid opcode or flag in scan

uint  php        : 1;         // block uses pushp (for multibanks)

uint  substart   : 20;        // start address of related subroutine (for names and sigs)

}  SBK ; 


// need both these next structs to be able to do both pops and arg tracing.............


typedef struct regstx
 {
   uint ssize   : 3;       // size last used, as ADT
   uint popped  : 1;       // popped reg
   uint arg     : 1;       // argument reg
   uint enc     : 4;       // encoded type
   uint reg     : 20;      // register (probably won't need all 20 - 13 ?)
   uint sreg    : 20;      // source register (i.e. the popped one) for ADT
   uint data    : 20;      // dreg for encode or data

 } RST;



//   Fake Stack - for extra checks - probably do need this.....

// TYPES 1-call 2-imd push 3-pushp (4 ?std push)
// pop set if popped (and register)
// 2 stacks to keep track of unloaded addresses (7aa1 in A9L)


typedef struct stk
{                              // fake stack struct
uint  popped     : 1;             // register entry is popped, cleared when pushed back             
uint  reg        : 8;             // register (last) popped to
uint  origadd    : 20;            // original value in nxtadd - for args detect
uint  type       : 3;             // 1 call, 2 imd, 3 psw
uint  psw        : 6;             // PSW value
uint  newadd     : 20;            // current addr value (at push)
}  FSTK;


typedef struct             // command holder for cmd parsing 
{
  uint   posn;             // char posn/column (for errors)
  uint   maxlen;





  int   p[8];               // 8 params max (allow for negative values)
  int   error;              // for true error return 
  uint  spf      : 5;       // special function subroutine
  uint  adreg    : 8;       // address register
  uint  szreg    : 8;       // size register
  uint  npars    : 3;       // num of pars in first number set (for error checks)
  uint  fcom     : 5;
  uint  term     : 2;       // terminating byte(s)
  uint  argl     : 1;       // use args layout
  uint  cptl     : 1;       // use compact layout


  int   levels   : 6;       // additonal levels (after the colon) allow negative
  int   tsize    : 8;       // row size from adnl      
  uint  firsterr : 1;       // first error, print cmd line if zero 
  char  symname  [66];      // max 63 chars plus a couple for checking

 } CPS;
 
/********************************
 *  Input Commands structure
 ********************************/

 typedef struct drs               // command definition
 {
  uint  (*setcmd) (CPS*);         // command processor(cmd struct)
  uint  (*prtcmd) (uint, LBK *);  // command printer (ofst,cmd index)

  uint  maxpars : 3 ;             // max pars allowed/expected 0-7
  uint  sapos   : 3 ;             // where single addr to be verified
  uint  appos   : 3 ;             // where 'start/end' pair is
  uint  namex   : 1 ;             // name expected/allowed

  uint  maxadt  : 5 ;             // max ADT levels (<31)
  uint  minadt  : 5 ;             // min ADT levels (<31)
  uint  defsze  : 3 ;             // default size, same as ssize in ADT 

  uint  ovrsc   : 5 ;             // overrides controlled by 'score'
  uint  merge   : 1 ;             // can be merged with same command (adj or olap)
  uint  decdft  : 1 ;             // default to decimal print
  const char*  gopts;             // global options
  const char*  opts;              // data levels
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
  const char *name;
} NAMES;


/********************************
 * Preset sym names structure
 ********************************/

typedef const struct
{
  uint p81   : 1;        // 8061
  uint p85   : 1;        // 8065
  uint wrt   : 1;        // write symbol
  uint bit   : 1;        // has bit if set, size otherwise
  uint bitno : 4;        // bit number if bit set (0-15)
  uint addr  : 20;       // address
  const char *symname;   // name

} DFSYM;

typedef struct
{                      // auto numbered symbol names
 int   mask;           // permission mask (a char, against cmdopts)
 uint  sizein  : 3;    // as per ADT
 uint  sizeout : 3;    // sign flags for func and tab lookup subrs
 const char *pname;
 }  AUTON;


typedef struct xsig
{
 struct xptn *ptn;      // pointer to signature pattern
 uint  start;           // start address of sig found
 uint  end;             // end of signature - only really used to stop multiple pattern matches and overlaps
 uint  done   : 1 ;     // processed marker - for debug really
 uint  skp    : 4 ;     // skipped opcodes at front
 uint  v[NSGV];         // 16 saved values
} SIG;


typedef struct xptn
{
 uint  *sig;                                   // actual sig pattern
 const char *name;                             // name of pattern
 void  (*pr_sig) (SIG *, SBK*);                // sig processor or zero if none
 uint size  : 16;                              // no of instructions in pattern
 uint vflag  : 4;
} PAT;

// end of header



#endif
