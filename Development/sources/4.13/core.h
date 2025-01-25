
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
 * 8065 manual says that bank is an extra 4 bits in CPU, (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could be 1 Mb (1048576), but biggest bins seem to be 256k (= 0x40000), or 4 banks, using 2 bits.
 *
 * Bin addresses used throughout are 20 bit, = bank in 0xf0000 + 16 bit address. Sometimes as unsigned int.
 * This code uses a pointer in a BANK structure to map into each malloced memory block (= bank) for best speed.
 * This design is therefore bank order independent.
 *
 * This code can handle all 16 banks with only changes to bank setup subroutines required (detect and check).
 * Apart from this, all code uses bank+address everywhere, but not tested beyond 4 banks
 *
 * NB. Changed above to store (bank+1) internally, so that 'bank present' checks are much easier, which
 * means that code now would support 15 banks, as 1-15 internally or 0-14 in bin terms, but only 0,1,8,9
 * are used, and are therefore internally mapped to 1,2,9,10
 *********************************************************/

#ifndef _XCOREX_H
#define _XCOREX_H 1

#include  "shared.h"


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

#define NSGV      16                 // num sig values, but sig code allows for 32
#define MAXSEQ    254                // max seq number

// command values.  Up to 31 commands (32 is cmd flag, can move it)

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

#define C_CMD    32      // by command (can't change or merge)
#define C_SYS    128     // for system 'base generated' cmds
#define C_NOMERGE  512    // inhibit merge for data options
#define C_GAP    256     // for gap scan


#define P_NOSYM  256     // nosym for pp_hdr

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
  uint sigix   : 7;              // signature index (='fingerprint')
  uint sign    : 1;              // signed prefix allowed (goes to next entry)
  uint p65     : 1;              // opcode 8065 only
  uint pswc    : 1;              // op changes the PSW (for conditional ifs)
  uint nops    : 2;              // number of ops   (max 3)
  uint wop     : 2;              // write op index  (max 3) this gets written to

  uint wfend   : 6;              // write op field end (zero if not wop)
  uint rfend1  : 6;              // read op field end
  uint rfend2  : 6;              // These sizes are field_end + 32 for sign
  uint rfend3  : 6;              // and get converted for sce code printouts

  void (*eml) (struct sbk *, struct inst *);    // emulation func
  const char name [6];           // opcode name
  const char *sce;               // high level code description, with extra markers
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

// Note that float is same size as uint (4), so could perhaps reuse data as a cast (float)



/*

* to split - structs in unions

range - name /

typedef struct rngadt
{
 void  *fid;            // Foreign key to SYM or RBT

 union uxz{
  struct  zxz {
 char   *name;
 uint tsize;
 } SXA;
 struct xzx {
 uint   val;
 uint testit : 5;
} SXB;
  } ;

 uint   rstart : 20;    // valid between start and end (0 and 0xfffff for 'all')
 uint   rend   : 20;
 uint   tsize  : 8 ;    // size of symbol name + null (for mallocs and free)

 //uint val;             //if used for RBT as well..........
// char   *ncmnt;       // symbol comment for repeated explanation

} RNGADT;












typedef struct xall {
void  *fid;           // foreign row id (to another CHAIN or this CHAIN)
uint seq    : 8 ;     // sequence number, 1-255   (0 reserved for new enquiry)
uint type   : 8 ;     //data type (= which union option)

union {

struct //xadt
{
float  fdat;          // full float for divisor (float = int for size)

int  data;            // addr, reg, offset, etc. int to allow for -ve


uint dreg   : 10 ;    // data destination register for data - for arg processing (0-0x3ff)

uint fstart  : 5;     // 0-31 start of sub field (bit number).
uint fend    : 6;     // 0-31 end   of sub field (bit number), with sign (+32)

uint cnt    : 5 ;     // (O) repeat count, 31 max
uint pfw    : 5 ;     // (P) print min fieldwidth (0-31)
uint prdx   : 2 ;     // (X) print radix 0 = hex, 1 = dec, 2 bin 3 ?

uint fnam   : 1 ;     // (N) look for symbol name
uint newl   : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr  : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint bank   : 1 ;     // (K) data holds a valid bank (in 0xf0000)
uint foff   : 1 ;     // (D) data is an offset address
uint ans    : 1 ;     // (=) this is ANSWER definition (separate item, for verification)

uint enc    : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)
} add;

struct        //xspf
 {
  void *fid;               // Foreign key to SUB
  uint fendin   : 6;       // may not need this...
  uint fendout  : 6;       // or this
  uint addrreg  : 10;

  uint spf      : 5;
  uint sizereg  : 10;
 } spf;

};                   //end of union
} ADT;

*/



typedef struct xadt {

union
{
float fldat;           // full float for divisor (float = int32 for size)
int   data;            // addr, reg, offset, etc. int to allow for -ve
};

uint fkey;             //  foreign key,  (ofst < 8) | seq 1-255

//31
uint dreg    : 10 ;    // data destination register for data - for arg processing (0-0x3ff)
uint fstart  : 5 ;     // 0-31 start of sub field (bit number).
uint fend    : 6 ;     // 0-31 end   of sub field (bit number), with sign (+32)
uint cnt     : 5 ;     // (O) repeat count, 31 max
uint pfw     : 5 ;     // (P) print min fieldwidth (0-31)

//19
uint bank    : 4 ;     // (K) holds a bank override, zero if none
uint prdx    : 2 ;     // (X) print radix 0 = not set, 1 = hex, 2 = dec, 3 bin
uint fnam    : 1 ;     // (N) look for symbol name
uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr   : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint foff    : 1 ;     // (D) data is an offset address
uint ans     : 1 ;     // (=) this is ANSWER definition (separate item, for verification)
uint div     : 1 ;     // (V or /) data is FLOAT, use fldat (union with data)
uint mult    : 1 ;     // (*) data is FLOAT, use fldat (union with data)
uint places  : 4 ;     // 0-15 print decimal places (is 7 enough ?) [forces radix 10]
uint enc     : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)

// uint write  : 1 ??
} ADT;




/********************************
 * RBASE list structure
 * map register to fixed address
 * only one value per register allowed
 ********************************/

typedef struct
{
  uint    val;            // address value pointed to, including bank
  uint    rstart : 20;    // range start
  uint    inv    : 1 ;    // invalid flag  (value changes somewhere in code)
  uint    cmd    : 1 ;    // added by cmd (not changeable)
  uint    rend   : 20;    // range end  - valid for rstart-r->end
  uint    reg    : 10;    // register (as address)

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

 char   *name;
 // char   *ncmnt;       // symbol comment for repeated explanation
 uint   addb;           // actual sym address + bitno 4|16|4|1 = bank|address|bitno|r/w

 uint   rstart : 20;    // valid between start and end (0 and 0xfffff for 'all')
 uint   fend   : 6 ;    // 0-31 + signed bit 32, shuffled down with start (in addb)
 uint   noprt  : 1 ;    // printed by one of other outputs (prt_xx), suppress sym print
 uint   cmd    : 1 ;    // set by dir cmd, do not overwrite
 uint   sys    : 1 ;    // system autogenerated
 uint   flags  : 1 ;    // split into bitnames for LDX, CLR, CMP
 uint   names  : 1 ;    // stop Bx_Rn prints, names only

 uint   rend   : 20;
 uint   xnum   : 5 ;    // autonumber symbol type if > 0  (31 max, for subrs)
 uint   immok  : 1 ;    // sym is allowed with immediate ops (below PCORG)
 uint   prtbit : 1 ;    // set if no bitmatch (search only)
 uint   tsize  : 8 ;    // size of symbol name + null (for mallocs and free)

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
uint fk      : 20;    // foreign key is ofst (opcode)

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
 uint cmd    : 1;            // added by user command, guess otherwise
 uint sys    : 1;            // system autogenerated
 uint dflt   : 1;            // this is default command made up for print.
 uint size   : 8;            // TOTAL size of data ADT additions
 uint start  : 20;           // start address (inc bank)

 uint term   : 2;            // data command has terminating byte(s) 0-3
 uint argl   : 1;            // use args layout
 uint cptl   : 1;            // use compact layout
 uint user   : 1;
// gribble  nomerge
// uint adt    : 1;          //use size instead
 uint fcom   : 5;            // command index
 uint end    : 20;           // end address (inc bank)
} LBK ;





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
  uint pkey     :20;       // start addr of subr
  uint fendin   : 6;       // may not need this...
  uint fendout  : 6;       // or this

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


// loop struct ??

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
uint  scanning   : 1;         // scan processing (do jumps, scans etc)
uint  emulating  : 1;         // emulating (with dummy scan block)
//uint  lscan      : 1;         // loop scan (emulate), change some things
uint  emulreqd   : 1;         // mark block as emu required (every time called) for args (here)
uint  nextaddr   : 20;        // next addr (opcode) or end of block.

uint  cmd        : 1;         // added by user command, guess otherwise
uint  stop       : 1;         // scan complete
uint  inv        : 1;         // invalid opcode or flag in scan

uint  php        : 1;         // block uses pushp (for multibanks)

uint  substart   : 20;        // start address of related subroutine (for names and sigs)

}  SBK ;


/************************
operand handling for printouts and anlysis
inst has 5 OPS entries (3 ops [1][2][3] + index [0] + save [4])
***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of register contents or an immediate value
 uint addr ;            // actual operand address (register mostly)

// (22 bits)
 uint rfend   : 6;      // Read size  as field end plus sign
 uint wfend   : 6;      // Write size, zero if no write (can be different to read size)
 uint imd     : 1;      // this is an immediate value
 uint rgf     : 1;      // this is a register  value
 uint bit     : 1;      // this is a bit value
 uint adr     : 1;      // this is an address (for jumps - includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with mask and sign)
 uint neg     : 1;      // offset address is negative
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print square brackets
 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found
} OPS;










typedef struct inst
{
//make OPS a separate struct with fk ??? pk is ofst...............

  OPS   opr[5];            // 5 operands. [4] is for SAVED operands, indirect and indexed.

  uint  opcix    : 8;      // opcode index into opcode table (max 112)
  uint  ofst     : 20;     // address of opcode, inc bank

  uint  sigix    : 7;      // opcode signature index
  uint  numops   : 3;      // number of operands - NOT ALWAYS same as opl (e.g. for BIT jumps)
  uint  opcsub   : 2;      //  0 = register (default) 1=imediate, 2 = indirect, 3 = indexed
  uint  wop      : 2;      //  write op index
  uint  bank     : 4;      // bank override for this operand, if set. (by bank prefix)
  uint  psw      : 6;      // psw AFTER this operation

} INST ;



// structure finder (tabs and funcs)
// argument holder for branch analysis subrs

typedef struct  sfx
 {
   uint spf;
   uint ans[2];
   uint rg [2];
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

  uint  tsize    : 8;       // total size of ADT  (bytes)
  uint  adreg    : 10;       // address register

  uint  spf      : 5;       // special function subroutine
  uint  firsterr : 1;       // first error, print cmd line if zero
  uint  szreg    : 10;       // size register

  uint  seq      : 8;       // seq no for additionals
  uint  npars    : 3;       // num of pars in first number set (for error checks)
  uint  fcom     : 5;       // command
  uint  term     : 2;       // terminating byte(s)
  uint  argl     : 1;       // use args layout
  uint  cptl     : 1;       // use compact layout
  uint  imm      : 1;       // immdok for syms
  uint  flags    : 1;       // flagsword for syms
  uint  names    : 1;       // names only

  char  symname[SYMSZE+1];   // symbol 96 + null

 } CPS;


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
  uint  stropt  : 1 ;             // command with strings (e.g. setopt)             //change to first 'type' above ??

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
  uint p81   : 1;        // 8061       //prob change to bitmask if more options
  uint p85   : 1;        // 8065
  uint wrt    : 1;        // write symbol
  uint fstart : 6;        // start bit number (0-31) or 32 if no bit
  uint fend   : 6;        // end bit number , 32 for sign
  uint addr  : 20;       // address
  const char *symname;   // name

} DFSYM;

/********************************
 * Autonumbered sym names structure
 ********************************/

typedef struct
{                      // auto numbered symbol names
 int   mask;           // permission mask (a char, against cmdopts)
 uint  fendin  : 6;    // as per ADT
 uint  fendout : 6;    // sign flags for func and tab lookup subrs
 const char *pname;
 }  AUTON;


/********************************
 * signature structure
 ********************************/

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
 uint vflag  : 4;                              // for encoded addresses
} PAT;


typedef struct bpx
{
uint  datbnk;         // currently selected data bank - always 8 if single bank
uint  codbnk;         // currently selected code bank - always 8 if single bank
uint  rambnk;         // currently selected RAM bank  - always 0 for 8061
} BASP;

#endif

// end of header