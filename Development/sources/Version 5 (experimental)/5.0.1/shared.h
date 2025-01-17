

 /**************************************************************************
 * Global declarations, structures and defines for all modules.  Global scope goes here.
 * may be separate <modulename>.h for more specific declarations in each module
 ***************************************************************************
 * BUILD NOTES
 * This code requires that INT and UNSIGNED INT are at least 32 bits (4 bytes).
 * and WILL NOT WORK with 16 bit compilers.
 * sizes stated here (in bits) for compilers used
 *
 * OS     compiler     int     float   long   void*
 * Win32  mingw        32      32      32     32
 * Linux  amd64-gcc    32      32      64     64
 *
 * This code is built with CodeBlocks and/or CodeLite IDE environments
 * on both Windows 32 and Linux 64 OSes.
 *
 *****************************************************************************
 * notes - bin addressing and banks
 *
 * 8065 CPU manual says that bank is an extra 4 bits in CPU, (bits 16-19) to give a 20 bit address (0-0xFFFFF)
 * which could therefore be 1 Mb (1048576).  Biggest bins seem to be 256k (= 0x40000), 4 banks, using 2 selection bits.
 *
 * Bin addresses used throughout this code are 20 bit, = bank in 0xf0000 + 16 bit address. Stored as unsigned int (uint).
 * This code uses a pointer in a BANK structure to map into each malloced memory block (= bank) for best speed.
 * This design is therefore bank order independent.
 *
 * This code can handle 15 banks with only changes to bank setup subroutines required (detect and check).
 * Apart from this, all code uses bank+address everywhere, but is not tested beyond 4 banks
 *
 * NB. Internally code uses (bank+1) internally, so that 'bank present' checks are much easier to do, which
 * means that internally bank numbers are 1-15  (= 0-14 in 'raw' terms).
 * Ford bins are only 0,1,8,9, which are mapped as 1,2,9,10
 *
 * Single bank bins are treated as if they are entirely in Bank 8 (9 internally) so exactly same analysis code is used.
 * Bank number is then suppressed in '_lst' printout (but not in debug information)
 *******************************************************************************************/

#ifndef _XSHARX_H

#define _XSHARX_H 1

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <fcntl.h>
#include  <sys/stat.h>
#include  <stdarg.h>
#include  <string.h>
#include  <math.h>

// #include  <time.h>             // if printing date and time


//shortcut for unsigned, const etc.

typedef unsigned char         uchar;
typedef unsigned short        ushort;
typedef unsigned int          uint;
typedef unsigned long         ulong;

typedef const char            cchar;
typedef const unsigned char   cuchar;


// Banks can be 1-15, only 0,1,8,9 used. All +1 internally = 1,2,9,A
// Registers fixed in Bank 8, 4 true 64k malloced banks.
// Single bank bins treated as bank 8 (= 9 internally)

#define BMAX      16             // maximum bank index

#define SADVERSION "5.0.1 Alpha"
#define SADDATE    "17 Dec 2023"


// debug defines - when switched on, this causes a LOT of output to xx_dbg file
// routines all named DBG_xx to make debug code more obvious

// #define XDBGX


#define NC(x)  sizeof(x)/sizeof(x[0])     // no of cells/entries in struct 'x'


#define PCORG  0x2000     // standard offset for all EEC bins

#define NSGV      16             // num sig values


// defines for which phase of analysis

#define ANLUDR    0                  // user commands read and check
#define ANLCMD    1                  // command processing
#define ANLFFS    4                  // find funcs, post scan (anlpass)
#define ANLPRT    5                  // print phase (anlpass)

#define SYMSZE    255                 // max size of symbol name (with null)
#define MATHSZE   32
#define STKSZ     10                 // size of fake stack(s)

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
//#define C_TIMR   10

#define C_CODE   10    // separate code command chain

#define C_ARGS   11   // struct data types

#define C_XCODE  12

#define C_SUBR   13
#define C_SCAN   14
// rbase          15

#define C_SYM    16


//bank 17
//setopts 18
//clropts 19
//set-psw 20
#define C_CALC   21    // calc (math formula) 21


#define C_USR    0x200     // by user command (can't change or merge)
#define C_SYS    0x400     // for system 'base generated' cmds
#define C_REN    0x800     // rename of symbol is allowed, even if duplicate

#define C_WRT    0x40      // 0x40 for write marker (fend) symbol or OPS field


#define P_NOSYM  0x80     // no sym print for pp_hdr (with byte size)

// jump types (and can have C_CMD for user commanded)

#define J_RET    0      // return opcodes
#define J_COND   2      // conditional jump ('if')
#define J_STAT   3      // 'static' jump (goto)
#define J_SUB    4      // subroutine call
#define J_VECT   5      // vect call, no push.

//#define J_ELSE   5    // not yet.

//#define J_PUSH   5      //  special
//#define J_OR     6
//#define J_NULL   7


//error numbers
#define E_DUPL     1           // duplicate
#define E_INVA     2           // invalid address
#define E_BKNM     3          // banks no match
#define E_ODDB     4          // odd address (WORD etc)
#define E_OVLP     5          // overlap (addresses or bit fields)
#define E_OVRG     6          // overlapping ranges
#define E_XCOD     7          // XCODE bans
#define E_NAMREP   8       // syname replaced
//#define E_NAMEBD   9       // dupl and orig not scanned
//#define MAX_CHERR  11     // max embedded chain error


//#define NOPTSTR   15             // num of option strings for setopts, clropts

// global command options, as bitmask


#define OPTSSRC     1         // source code
#define OPTTABL     2         // auto table names (+func on old system)
#define OPTSTRS     4         // auto subroutine names for function and table lookups
#define OPT8065     8         // 8065 codeset (can be set internally)

#define OPTLABEL    0x10      // auto label names (lab)
#define OPTMANL     0x20      // full manual mode
#define OPTINTF     0x40      // auto intrp func naming
#define OPTPROC     0x80      // auto proc names (xfunc, vfunc, intfunc)

#define OPTSIG      0x100     // auto Lookup Signature subroutine detect
#define OPTFUNC     0x200     // auto function names
#define OPTACOM     0x400     // do autocomments
#define OPTRSYM     0x800     // do preset symbols

#define OPTARGC     0x2000     // set compact argument layout
#define OPTSTCA     0x4000     // set extended data struct layout

#define OPTDEFLT    0xFC7      //  default (= multiple options)


// defines for signature index (patterns)


#define PATRBSE         0    // rbase lookup
//#define PATINIT         0    // initial jump (bank detect)
#define PATAVCL         1    // altstack vector list
#define PATTIMR         2    // timer list
#define PATFUNC         3    // func lookup - byte or word
#define PATTABL         4    // table lookup, later multibank
#define PATTBL2         5    // table lookup, early
#define PATEC13         6    // encoded address types 1 and 3
#define PATEC24         7    // encoded address types 2 and 4,

#define PATBINTP         8   // byte table interpolate (for signed/unsigned checks)
#define PATSTK          9   // stack fiddler

#define PATWINTP         11   // word table interpolate (for signed/unsigned checks)


//#define PATIGNI         9    // ignore interrupt




// defines for CHAINS , done by number so all chain handling is inside chain.cpp

#define CHSYM      1                // symbols
#define CHBASE     2                // register bases
#define CHSIG      3                // signatures
#define CHCMD      4                // commands main
#define CHAUX      5                // commands auxiliary

#define CHSCAN     6                // scans
#define CHSTST     7                // test scans , emu scans

#define CHSUBR     8                // subroutines

#define CHADNL     9                // additional print command data
#define CHSPF      10                // special func subroutines
#define CHPSW      11               // link for PSW set (unresolved conditional jumps)

#define CHDTK      12               // data tracking by opcode
#define CHDTD      13               // data tracking by address (reverse - ish)
#define CHMATHN    14               // math formula name
#define CHMATHX    15               // math formula definition (by term)
#define CHLINK     16               // linker forwards keyin->keyout (many->many)
#define CHJPF      17               // jumps, from
#define CHJPT      18               // jumps, to      (remote index)
#define CHBNK      19               // banks found
//#define CHRST      20               // register status
//#define CHPOP      21               // register pop links

//data/print/calc types            print/read   calc as

#define DP_HEX      1               // hex       integer
#define DP_ADD      2               // hex+bank  address (16 bit wrap)
#define DP_BIN      3               // binary    integer
#define DP_DEC      4               // decimal   integer
#define DP_FLT      5               // float     float
#define DP_REF      6               // 'x' ref   extract from ADT definition (= data field) maths only
#define DP_SUB      7               //  -        subterm to calc, maths only
#define DP_MSKN     8               // binary mask, with next byte - makes a symbol bx_A (timers etc)

// param flags for hex number reads, banks and so on.


#define HXRL      0xf              // read length mask 4 bits at bottom
#define HXLZ      0x40             // lead zero single bit
#define HXNG      0x80             // '-' before number (for negative offsets)



#define g_bank(x)    (x & 0xf0000)                            // get bank
#define nobank(x)    (x & 0xffff)                             // strip bank
#define bankeq(x,y)  ((x & 0xf0000) == (y & 0xf0000))         // address banks equal



//*******************************************************************************************
// file holder structure, names, paths, and file handle pointer
//  file order is (xx.bin, xx_lst.txt, xx_msg.txt, xx.dir, xx.cmt, xx_dbg.txt, SAD.ini)
//*******************************************************************************************

//255 filename 4096 path name

 // first 3 (rows) assigned in main.cpp
 // next are calculated


typedef struct
{

 cchar *suffix[7];      // file suffix (.bin etc)
 cchar *fpars [7];      // open file mode  NB. differs for linux and windows
 FILE  *fh[7];          // file handles [8] if adding a sigs file ???

 char  *fpath[7];       // file path as read from sad.ini or set to default (dpath)
 char  *fname[7];       // full file names with path

 uint  pathsize[7];     // strlen+1
 uint  namesize[7];     // strlen+1

 char  *defpath;        // default path (SAD exe location)
 char  *barename;       // bare file name (root binary name)
 uint  dpsize;          // defpath strlen+1
 uint  bnsize;          // barename strlen+1

 uint  flength;         // length of main binary file (entry [0]
 uint  error;           // file read error


} HDATA;




//*******************************************************************************************
// Bank definition. hold details & related pointers.

typedef struct                // indexed by bank num (0-16)
 {
 uchar  *fbuf;                // bin file (copy) for this bank (ptr to malloc).
 uchar  *ftype;               // type data for each bin byte in bank  1 = code, 2 = operand, 4 = argument

 uint  filstrt;               // start FILE OFFSET. ( = real file offset)
 uint  filend;                // end   FILE OFFSET.

 uint  minromadd : 20 ;        // (normally PCORG, 0x2000)
 uint  bkmask    : 4 ;         // possible banks as bitmask (for 'find')
 uint  dbank     : 4 ;         // destination bank as number

 uint  maxromadd : 20 ;        // where ROM bank really ends (after fill)
 uint  bprt      : 1 ;         // print this bank in msg file
 uint  bok       : 1 ;         // this bank is valid (and found flag)
 uint  usrcmd    : 1 ;         // set by command
 uint  cbnk      : 1 ;         // code start bank ( = 8 )
 uint  noints    : 1 ;         // bank with no interrupt vectors (FMO206)
 uint  P65       : 1 ;         // this is an 8065 bank


 }  BANK;

//bank find - only used at startup

typedef struct bnkfnd
{
 int  filestart;          // file start address
 uint pcstart;            // program start  (not always 0x2000)
 uint jdest;              // jump destination

 uint vc61   : 8;         // valid vector  count 8061 max 8
 uint ih61   : 8;         // valid handler count 8061 max 8
 uint vc65   : 8;         // valid vector  count 8065 max 40
 uint ih65   : 8;         // valid handler count 8065 max 40

 uint tbnk    : 5;         // temp bank number
 uint skip    : 5;         // max 15 (in code bank)
 uint lpstp   : 1;         // loopstop jump at start (not start bank)
 uint cbnk    : 1;         // real     jump at start (this is codebank)
 uint noints  : 1 ;         // bank with no interrupt vectors (FMO206)
}  BNKF;




//*******************************************************************************************
// Structure to hold data as linked ordered list for binary chop searches.
// void* pointer array supports different structs used in each list (chain)
// has defined handlers for free memory, compare, and print-block
// entry size can be zero to support two chains to same data to allow for different ordering
// pointer array is realloced when more pointers (blocks) are needed, and is always contiguous.
// data blocks (structs) are NOT contiguous
//*******************************************************************************************


typedef struct chain                           // done like a class with subroutines by chain type
{
struct chain *sce;                             // source chain of new blocks (for remote indexes)
uint num;                                      // no of (used) elements;
uint pnum;                                     // no of pointers in array.
uint asize;                                    // allocation size for pointer blocks
uint esize;                                    // entry (struct) size

uint lastix;                                   // last search index, for shortcuts and 'get_next_'
uint lasterr;                                  // for user commands etc
uint lowins;                                   // lowest inserted index for loops to restart
 uint newins;                                  // index of block used by chmem....

void  **ptrs;                                  // ptr to array of pointers
void  (*chfree) (void*);                       // block free (by struct pointer)
int   (*chcmp ) (struct chain *,  uint, void*);                  // struct chain *, uint, void*);  // compare,  for binary chop
int   (*cheq  ) (struct chain *,  uint, void*);     // equals for finds not always quite the same as compare
int   (*chprt ) (struct chain * , uint,  uint (*prt) (uint,cchar *, ...));     // print item (command errors, msg etc)
} CHAIN;

// totallocsize ?? DBG ??


/************************
 * operand holder for printouts and anlysis
 * inst has 5 OPS entries (3 ops [1][2][3] + indexed [0] + saved [4])
 * op 3 is write op, and can have different size from its read op equivalent for 1 and 2 operand opcodes

 * opcodes are defined as
 * 3 ops     sce1, sce2, dest    for   [3] = [2] <op> [1]
 * 2 ops     sce1, dest          for   [2] = [2] <op> [1]
 * 1 op      dest/sce1           for   [1] =     <op> [1]
 *
 * SAD organises these as
 * Num     op[3]    op[2]    op[1]       internal maths
 * 3 ops   dest     sce2     sce1       [3] = [2] <op> [1]
 * 2 ops   #sce2    sce2     sce1       [3] = [2] <op> [1]
 * 1 op    #sce1    -        sce        [1] = op [1] or [1]= op
 *
 * '#' means copy of, adjusted to write size as necesaary
 * ops are swopped or copied as required (STX swops, others copy).
 * for multimode address opcodes -
 * op[4] is copy of op[1] before indexed and indirect ops calculated
 * op[4] is incremented in ->val if autoinc set
 * op[0] is offset (indexed) or intermediate address (indirect) ops

***************************/

typedef struct          // operand storage, max 4 for each instruction
{
 int  val  ;            // value of register contents or an immediate value
 uint addr ;            // actual operand address (register mostly)

// (17 bits)
 uint fend   : 7;       // Read size as field (0-31), sign (+32), write (+64)
                        // all operands are treated as fstart = 0;

 uint imm     : 1;      // this is an immediate op
 uint ind     : 1;      // this is an indirect op
 uint inx     : 1;      // this is an indexed op (+ op[0] as offset)

 uint rgf     : 1;      // this is a register value
 uint bit     : 1;      // this is a bit value  (bit jumps)
 uint adr     : 1;      // this is an address (for jumps - includes bank)
 uint off     : 1;      // this is an indexed OFFSET address (with mask and sign)
 uint neg     : 1;      // offset address is negative
 uint inc     : 1;      // autoinc (easier for printout....)
 uint bkt     : 1;      // print bracket

 uint rbs     : 1;      // register is an rbase
 uint sym     : 1;      // 1 if sym found
} OPS;

//uint jback    : 1;       // backwards jump             could put these in OPS for exta info , ??
//uint jloop    : 1;      // infinite loop jump
//uint jdummy   : 1;      // dummy jump (offset is zero)

//uint jcond
//uint jstat             jump types ?? or a number
//uint jcall ??
//uint jvect ??






// 'instance' holder structure - for ONE opcode and all its operands

typedef struct inst
{

  struct sbk *scan;        // current scan block;
  OPS   opr[5];            // 5 operands.  [0] for indirect and indexed. [4] is saved original,

//29
  uint  ofst     : 20;     // address of opcode, including bank
  uint  opcix    : 8;      // opcode index into opcode table (max 112)
  uint  inv      : 1;      // invalid address, for ind and inx, stop upd_watch

//24
  uint  opcde    : 8;      // original opcode
  uint  sigix    : 7;      // opcode signature index
  uint  numops   : 3;      // number of operands - NOT always same as opl tab (e.g. BIT jumps)
  uint  opcsub   : 2;      // 0 = register (default) 1 = immediate, 2 = indirect, 3 = indexed
  uint  bank     : 4;      // bank override for this operand, if set. (by bank prefix)
  uint  sign     : 1;      // sign bit prefix (0xfe)


} INST;








// global chain/list  structures

// general command struct




/******************************************
* Additional data struct
* used by data structs and subrs for inline print params

* fid is void* to allow adt to be attached to anything else
* calcs are attached with link struct (many-many)
* ADT is now for print instructions only.
* SPF is for special func definitions.
* ADT attached to LBK and SBK (not SUBs any more).
**********************************************/


// tsize for total size of chain ?? cheat with a union ??

typedef struct xadt {

 void *fid;              // foreign key, first link to non-adt parent,by address,
                         // rest is to ADT by pointer for chain.
//31
 uint fstart  : 5 ;     // (B) 0-31 start of sub field (bit number). = B x y  (start, end)
 uint fend    : 6 ;     // (B) 0-31 end   of sub field (bit number), with sign (+32)
 uint cnt     : 5 ;     // (O) repeat count, 31 max
 uint pfd     : 5 ;     // (P) 0-31 print decimal places  [forces radix 10] = X n.p  main part goes in pfw
 uint pfw     : 6 ;     // (P) print min fieldwidth (0-31) inc decimal places ?  *** to confirm - add pfd ??
 uint bank    : 4 ;     // (K) holds a bank override, zero if none (i.e. default to databank)

// 8
 uint fnam    : 1 ;     // (N) look for symbol name, or 'name' flag with sym command (may be replaced with 'M')
 uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
 uint sub     : 1;      // has ADT subdefinitions - don't print if only size declared ?? front or back ?? redundant ??
 uint sbix    : 2;      // sublevel/subfield for use with fid+n  (top levels use plain fid) may only need one bit ?      <<<<<<<
 uint dptype  : 4;      // (X,R) and V <float V> radix and print mode.  as in defines at top 'DP___'

} ADT;

/*

 * cptl - compact layout ??
 * args - args layout ???     but these are HEADER type flags....

 * ADT is now print instructions only and no data - makes more sense overall

 * changes -
 * R  marked in dptype (as address).
 * D  offset address       - becomes calc  add(x+offset)
 * V  divide               - becomes calc  flo(x/n),
 * E  encoded data type    - becomes calc  add(enc(x,n))

 * allow short form as "= ( x/128)" in adt string.
 * add brackets will make cmd read the same...........
 *
 */


 /********************************
 *   symbol structure
 ********************************/

// This is BYTE based in the chain, with bits 0-7.
// bit number and address modified for fields greater than 7

// SFRs have extra bit to indicate no 'hangover' as word acces does NOT include upper byte
// where specified


typedef struct
{

 char       *name;
 // char   *ncmnt;       // symbol comment for repeated explanation ?

 uint addr    : 20;
 uint fstart  : 3;        // 0-7 (always mapped to byte)
 uint fend    : 7;        // Read size as field (0-31), sign (+32), write (+64)

 uint fmask ;          // field bitmask  (for ease of matching, calced from fstart,fend)

 uint rstart  : 20;     // RANGE start - valid to rend (0 and 0xfffff for 'all')

 uint prtadj  : 2 ;     // if set, use for bit recalcs if fstart greater than 8
 uint noprt   : 1 ;     // printed by one of other outputs (prt_xx), suppress sym print
 uint usrcmd  : 1 ;     // set by dir cmd, do not overwrite
 uint sys     : 1 ;     // system autogenerated
 uint immok   : 1 ;     // sym is allowed with immediate ops (below PCORG)
 uint sfrnop  : 1 ;     // sfr, word mode NOT overlap into next byte.

 uint whole   : 1;       // this is a whole byte(s) size. (i.e. no bit field)
 uint rend    : 20;      // RANGE - range end

 uint symsize : 8 ;     // size of symbol name + null (for mallocs and free)

 } SYM;




/********************************
 *   Jump list structure
 ********************************/

typedef struct jlt
{

  uint   fromaddr : 20;    // from address
  uint   back     : 1;     // backwards jump

  uint   obkt     : 1;     // print open bracket
  uint   swl      : 1;     // swop 'if' logic sense for or bracket
  uint   cbkt     : 1;     // print close bracket

  uint   bswp     : 1;     // bank change
  uint   size     : 3;
  uint   retn     : 1;     // a jump to a 'ret' opcode

  uint   toaddr   : 20;    // to address
  uint   jtype    : 4;     // type (same as scan)
  uint   cmpcnt   : 3;     // count back to attached compare (if set) for adjacent checks

 } JMP;

//  uint done  : 1;
//  uint jelse : 1;
//  uint jor   : 1;
//  uint   subloop  : 1;     // this is a loop within a loop
//  uint   inloop   : 1;
//  uint   exloop   : 1;
// uint jwhile  : 1;
// uint jdo     : 1;



// subroutine structure - only used for user defined subroutines, and holder for spf ???
// or move spf to scan blk

typedef struct
{
 uint  start  : 20;       // start address (inc bank)
 uint  usrcmd : 1;        // set by command, no mods allowed
 uint  sys    : 1;        // system autogenerated

 } SUB;


// ********************** SPF ************************
// for subroutines with special functions.
// chained via fid, first -> subr then parent

// 1    answer  (Rn = SUBR) in printout

// 2    func lookup
// 3    tab  lookup
// 4    timer ??  reserved

// 5    fixed args
// 6    variable args via djnz???

// 7    encode arg - may be at different level to arg getter.


// SPF and ADT now attached to the SCAN block of subr.
// links spf to adt ??
// CAN have chained spf (e.g. args+fnplu), but does that matter ??

typedef struct dp
{
  uint refaddr     : 20;      // test  - address of argument (as byte)
  uint reg         : 8;
  uint notarg      : 1;       // transfer, not argument.
  uint skip        : 1;       // skip arg if set (for word,long)

  uint dfend       : 5;       // 0-31 for size;
  uint calc        : 1;       // math calc is attached to this param


  // may need sreg (source regsiter...)

} DPAR;

typedef struct xspf
 {
  void *fid;              // scan block, or SPF for chaining - remove and use links ??  useful but a pain for chains

 // uint sigaddr : 20;      // for duplicate detection ??? and maths funcs ???

//29
  uint spf      : 8;       // special func index
  uint fendin   : 6;       // field size in (with sign) fstart is always zero ?? maybe not for tables and funcs
  uint fendout  : 6;       // field size out  for tables and funcs.
  uint argcnt   : 5;       // 15 max
  uint level    : 4;       // stack level ??  is not used at the moment ?
  uint jmparg   : 1;       // whether extract code has a jump (->emulate) or is fixed (-> args via spf) type 10 only ??

  DPAR  pars  [16];         // attached addresses/registers/encode
} SPF;


//  from SUB struct which may be dropped
 //uint  cmd    : 1;        // set by command, no mods allowed
// uint  sys    : 1;        // system autogenerated
// uint  cptl   : 1;        // use compact layout  (args is default) back to ADT ??
// //   //uint  size   : 8;        // for any ADT attached args not for remote arg getters, see spf






/********************************
 * PSW basic link struct
 * two straight addresses
 ********************************/

typedef struct
{
  uint    jstart;        // address of jump, including bank
  uint    pswop;         // address of psw setter
} PSW;

/*****************************************************
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
    void *keysce;          // source key
    void *keydst;          // dest   key
    uchar chsce;            // source chain
    uchar chdst;            // dest   chain
    uchar type ;            // default to 1, for multiple links between chain pair

}  FKL;



typedef struct dtk
{
 // OPCODE tracking (for data addresses)

// first items copy lbk for later transfer and validate or adjust
// mutliple starts in addnl struct below

// but opcsub+psh+cmp+stx is already 5 bits......numops is 7
//perhaps a simple opcix/full and 3 regs and


uint ofst    : 20;      // where from (opcode)

ushort rgvl[5];            // reg 0-4 for indexes etc.

uint ocnt    : 4;       // total size of 'from' opcode
uint opcix   : 8;       // opcode index

uint numops  : 2;       // actual numops (may be different to opctab)

uint base      : 20;    // where indexed, may be others in R[4]
int offset     ;

uint opcsub  : 2;       // direct,indirect,imd,inx *
//uint psh     : 1;       // push instruction or related *
//uint pop     : 1;       // push instruction or related *
//uint ainc    : 2;       // auto inc size *


} TRK;

//move inc etc into here to reduce access to main entry for struct analysis,
// opcsub and size, psh,


// DATA ADDRESS tracking

typedef struct dx
{

uint stdat   : 20;    // unique start address (PK).

uint gap1     :  5;    // where next address is close, keep gap
uint gap2     :  5;    // where next address is close, keep gap

uint bcnt     :  4;
uint icnt     : 4;
uint modes   :  4;    // multiple opcsub by bit flag for checks

//int offset     ;

//uint base      : 20;    // where indexed, may be others....may have to be full 20 bits for base+r as oposed to R+offset

uint inc    :   2;    // auto inc size, 1 or 2
uint olp     :  1;    // overlaps code   (not valid)
// base olap ??
uint psh     :  1;
uint pop     :  1;
uint rbs     :  1;

uint bsze    :  4;    // 1,2, or 4 bytes may need a 3 for triple.....

//more bits fit here anyway

}  DTD;

// this will not work for triple and long with extra address.
// put in ADT or here as extra 20 bit address ?? but if no extra, belongs HERE

typedef struct lbk        // command header structure
{
 uint start   : 20;           // start address (inc bank)
 uint size    : 8;            // TOTAL size of any data ADT additions
 uint usrcmd  : 1;            // added by user command, guess otherwise
 uint sys     : 1;            // system autogenerated
 uint dflt    : 1;            // this is a default command made up for print.

 uint end     : 20;           // end address (inc bank)
 uint fcom    : 5;            // command index
 uint term    : 2;            // data command has terminating byte(s) 0-3

 uint argl    : 1;            // use args layout
 uint cptl    : 1;            // use compact layout
 uint spfdone : 1;            // stop duplicate spf processing

                    // uint from    : 20;           // where called from
 uint adt      : 1;            // adt attached ?
} LBK;



/********************************
 * RBASE list structure
 * map register to fixed address
 * only one value per register allowed
 ********************************/

typedef struct
{
  uint  val;            // address value pointed to, including bank

  uint  rstart  : 20;    // range start
  uint  reg     : 10;    // register (max 0x3ff)

  uint  rend    : 20;    // range end  - valid for rstart-r->end
  uint  inv     : 1 ;    // invalid flag  (value changes somewhere in code)
  uint  usrcmd  : 1 ;    // added by cmd (not changeable, always valid)

} RBT;


// scan block structure - each scan has its own fake stack


typedef struct lcstk
{
  uint start     : 20;       // for SBK find.......may not be required... psp for pushp
  uint nextaddr  : 20;       // or sceaddr for psp ??

  uint  sceaddr  : 20;     // where this entry was added from.... for duplicate pshps.  read from ??)
  uint  type     : 4;      // 1 - call (sc), 2 - immd push (sc)   3 - pushp (=PSW), psp
  uint  modded   : 1;      // changed, to reflect back upwards (args)

} STACK;




typedef struct sbk {          // scan block structure

STACK stack[8];                  // stack of nextaddrs (etc) for args - malloc this for better space use ???

struct sbk   *caller;            // jumped to from here........

//static jump history for args and spfs too ??

//30
uint  start      : 20;        // start address

uint  stkptr     : 3;         // 0-7 for stack
uint  ignore     : 1;
uint  usrcmd     : 1;         // added by user command, guess otherwise
uint  stop       : 1;         // scan complete
uint  estop      : 1;         // stop for emulate first
uint  inv        : 1;         // invalid scan
uint  ssjp       : 1;         // call types - jump
uint  ssub       : 1;         // call or vect

//30

uint  curaddr    : 20;        // current addr - scan position (inc. start)
uint  chn         : 5;        // which scan chain this block is in, for emulate etc
uint  levf       : 2;         // level fiddler (pop,call,push types) up to 3 pops

uint  adtcptl     : 1;        // - compact layout ??    args otherwise - to ADT ??
uint  emureqd     : 1;        // bit mask of emu levels required (every time called) for args (here)
uint  getargs        : 1;        // arg getter


//31
uint  nextaddr   : 20;        // next addr (opcode) or end of block.
uint  adtsize    : 8;         // adt attached and its size, for args (user) .
uint  proctype   : 3;         // 0 none (e.g. sigs and searches), 1 - scanning, 2, gap and verify,  4 emulating

//29
//uint  popreg     : 10;        // when this block popped, zero otherwise.
uint  psw        : 6;         // psw state - should this become 16 include banks ??
uint  datbnk     : 4;         // 0-15 data bank - always 8(9) if single bank - > R11 !!
uint  codbnk     : 4;         // 0-15 code bank - always 8(9) if single bank    PSW
uint  rambnk     : 2;         // 0-3  RAM  bank - always 0 for 8061 PSW
//uint  logdata    : 1;         // log data for LBK (data) chain

//uint  spf        : 1;         // has special funcs or args. ?  actual defs in attached spf.  don't ned it, do get_spf..



}  SBK;


// Register status for stack and arguments handling.  Indexed by register 0-0x400
// This is a chain but directly accessed (no bfindix)
// rbase is separate chain because of support of address ranges

//do with links ?/



     /* link types
      *
      *
      * or swap argsblk from scan block to aux cmd ????
      * or add aux_cmd as extra link............
      *
      *
      *
      *
      *
      * FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)
   ptarget = (CHRST, reg, CHSTST, sbk, 1)      (reg<->scan)
   argsblk = (CHRST, reg, CHSTST, sbk, 2)      (reg <-> scan)      BUT TST gets CLEARED !!! which messes this up......
    *
    *
    *
   scanblk = (CHRST, reg, CHSCAN, sbk, 0)      (real scan)
    * auxcmd ?
    *
    * spf ??  CHSCAN    CHSPF  1   (scanblk)
    *         CHSCAN    CHSPF  2   (argsblk)
    *
    * spf attached to real scan (scanblk)    need spf to (real?) argsblk too.
    *
    * then need spf ... from core.cpp
  z = get_scan(CHSCAN,g->ptarget->start);        // this trick seems to fix multiples
         f = add_spf(z,10,1);            //s,10,1);             // a dummy spf for now, but with right level ??
*/

//curblk ??  where call is made from, or is that always emublk start point <- caller of start point ????
//  SBK *ptarget;         // subr scan block.   args/spf? are attached to this scan block when popped (i.e. the call before nextaddr)
 // SBK *argsblk;         // block where nextaddr is (where the args actually are) is this also caller block ??

// LBK* argscmd;       //the args command.........


// one spf, one ptarget, multiple argblks + cmd blocks  ??? but need to get both ways for some..all ???


// SINGLE pointer to a 'pop block' as only a few registers are used ??  Separate 'pop block' chain ??
// but sep chain cannot be preallocated, otherwise what's the point ???? or is links better as two way ???

// POPB *argsblk;    and have POPB as new chain, but this would be multiple ??
// many->one for each subr, although only one spf ??

//in core .cpp ??
//or should all auxcmds get linked to prtarget ??

// make this a chain ??




//if adding command types....may be...

// fill - data (+size), operand, opcode.


// Binary file byte type
// info about each binfile byte - for opcode, operand, data etc.
 /* ftype values (bottom 3 bits)
 * 1 = code
 * 2 = read data
 * 3 = write data (regs and RAM) not sure yet
 * 4 = args data
 * + fend in tope 5 bits
 * 5+3 bits.....make it a uint ???? then can get most of RST in it too....
 */



//allocs 4 anyway.........



typedef struct regstx
 {
  // 24        size 4 = 32 ?

  uint sreg    : 8;       // source register (i.e. the popped one)
  uint fend    : 5;       // size 0-31 for arg sizes
  uint argcnt    : 4;       // mark that popped reg is incremented -> args present.
  uint rbcnt   : 3;       // changed count, for rbases
  uint rbase   : 1;       // valid rbase register
  uint popped  : 1;       // register currently popped
  uint pinx    : 3;       // index to RPOP (= stack pointer)
  uint arg     : 1;       // this register holds an argument (or part of it)

 } RST;

  // uint stype : 2 ?        // for a non-pop copy to copy option ??? e.g. 7992 in A9L is ptr, 36a9 is direct...
  //direct (from popped), indirect (via Rx through to pop), (pointer to another register)

//  uint ereg    : 8;       // encode rbase register        -- put somewhere else ??      may have to put these back if can't gte to spf..
//  uint enc     : 4;       // encoded type        -- put somewhere else ??

//uint incr  : 1;  or more;  mark that popped reg is incremented -> args present.
// would need trick for [stk+x] code ??

  //uint par_no may need this for encode and so on ?? param number in arg list.

// maybe data type for tracking of say , time, volts, flags etc
  // uint dtype : 3;      // perhaps match type in ADT ??

  // should encode go in spf ??




typedef struct rpop
{
 SBK *tgtblk[2];         // real [0] and emu [1] subr scan block.  (i.e. the call before nextaddr)
 SBK *argblk[2];         // real [0] and emu [1] block where nextaddr is (where the args actually are, and caller of subr)
 LBK* auxcmd;            // args command
 SPF* fblk;              // link to spf block (attached to real scan block anyway ?) now via tgtblk->nextaddr
 uint popreg : 8;        // popped register (and used....) may need separate flags ??
} RPOP;




// opcode table, main definition - called from INDEX structure, not opcode value

typedef struct            // OPCODE definition structure
 {
  uint  sigix   : 6;             // signature index (='fingerprint')
  uint  sign    : 1;             // signed prefix allowed (goes to next entry)
  uint  p65     : 1;             // opcode 8065 only
  uint  pswc    : 1;             // op changes the PSW (for conditional ifs)
  uint  nops    : 2;             // number of ops   (max 3)
  uchar fend    [3];             // opsizes as 0-x (field end), +0x20 if signed, +0x40 if write
  SBK* (*eml) (SBK*, INST *);    // emulation func
  cchar name    [6];             // opcode name
  cchar *sce;                    // high level code equivalent + markers
 } OPC;


/********************************
 * signature structure
 ********************************/

typedef struct xsig
{                         // variable part of sig for holding mathces

 uint  start;             // start address of sig found (for chain
 uint  end;               // end of signature - only really used to stop multiple pattern matches and overlaps
 uint  hix;               // signature handler index (calls subr[hix])
 uint  done   : 1 ;       // processed marker - for debug really
 uint  v[NSGV];           // 15 saved values.  v[0] is a param for handler subr

 char name [16];          // sig name - for debug mainly
} SIG;


// sig processor index, linked inside each pattern.

typedef struct sigpxx
{
     void  (*pr_sig) (SIG *, SBK*);                // sig processor or zero if none
} SIGP;

// command and comment structure - string handling and parameter processing

typedef struct              // command holder for cmd AND COMMENT parsing
{
  ADT   *adnl;              // current adnl pointer
  INST  *minst;             // current instance ptr
  char  *cmpos;             // current read char posn
  char  *cmend;             // end of command line

  int   p   [8];            // 8 params max (allow for negative values)
  uint  pf  [8];            // param flags

  int   sqcnt;              // square bracket count
  int   rbcnt;              // round bracket count
  int   adtcnt;             // adt terms
  int   mcnt;               // math terms
  int   error;              // for true error return

  char  string   [256];     // any name, normally symbol

  int  tsize      ;       // total size of ADT  (bytes)
  int  mcalctype  ;
  int  mfunc      ;           // math built in function
  int  spf        ;       // special function subroutine

  int  szreg      ;      // size register

  int  adreg      ;      // address register
  int  ansreg     ;      // answer register

  int  seq        ;       // seq no for additionals
  int  strsze     ;       // actual size of string
  int  npars      ;       // num of pars in first number set (for error checks)
  int  fcom       ;       // command
  int  term       ;       // terminating byte(s)
  int  argl       ;       // use args layout
  int  cptl       ;       // use compact layout
  int  firsterr   ;       // first error, print cmd line if zero

 } CPS;

/********************************
 *  Input Command definition structure
 ********************************/

 typedef struct drs               // command definition
 {
  uint  (*setcmd) (CPS*);         // command processor(cmd struct)
  uint  (*prtcmd) (uint, LBK *);  // command printer (ofst,cmd index)

  uint  minpars : 3 ;             // min params expected 0-7 (for p (8) in CPS above)
  uint  maxpars : 3 ;             // max params allowed 0-7
  uchar ptype[4];                 // only 4 param types used so far in commands
                                  // 0 - none, 1 start addr, 2 end addr, 3 register,
                                  // 4 start range, 5 end range  (4 and 5 and 1 and 2 are pairs)
  uint  namex   : 1 ;             // name expected/allowed

  uint  minadt  : 5 ;             // min main ADT levels (0- 31)
  uint  maxadt  : 5 ;             // max main ADT levels (0- 31)
              // sub levels ?
  uint  dfend   : 5 ;             // default fend (size), from zero
  uint  merge   : 1 ;             // can be merged with same command (adj or olap)
  uint  stropt  : 2 ;             // command with strings, which option (std=0 setopt=1, maths = 2)

    // 3 spare bits

  uint   ovrmsk;                 // can override any cmd in this mask
  cchar* gopts;                  // global options allowed
  cchar* mainopts;               // main (top level) option letters allowed
  cchar* subopts;                // subopts options allowed

} DIRS;


/********************************
 * Autonumbered sym names structure
 ********************************/

typedef struct axn
{                      // auto numbered symbol names
 uint  mask;           // permission mask (a char, against cmdopts)
 uint  spf;            // spf function (or zero if no relevant)
 uint  fendin  : 6;    // as per ADT
 uint  fendout : 6;    // sign flags for func and tab lookup subrs
 cchar *pname;
 } AUTON;




/* for below structs, some similar fields shared to try to make handling regular and consistent -
 * pmode      identical to ADT.   0 = none (default) 1 = hex (user), 2 = address (hex+bank) 3 = bin  4 = decimal int  5 = decimal float
 *            NB. default is HEX for ADT, but DECIMAL in math calcs
 * datatype   0 empty (none),  1 integer,  2 reference ('x' in math) ,  3 float, 7 subterm attached via FK
 *            signed/unsigned ?
 * calctype   0 none, 1 address (16 bit wrap and bank), 2 integer,  3 - float   (signed/unsigned ??)
*/


// variable number type........... *


typedef struct  mlnum    // multiple number holder
{

 union
 {
     // may need double instead of float ?
   float fval;            // actual value (as floating)
    int  ival;            // actual value (as int)               // allow signed
 //  uint  refno;            // ref (Xn)   - may be redundant.
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

  uint nsize     : 8;      // name size - can be null
  uint mcalctype  : 4;     // master calc type

  uint sys        : 1;     // system added
// uint nameprt : 1;       // print or not ??  if dummy name (direct = in adt)  print formula in adt, not formula list.

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
 uint calctype    : 4;            // local calc type (is this required ?)
}  MATHX;


// preset function list, includes single char +,- etc may need datatypes ??? or use calctype ?

typedef struct mxf
{
uint fnpars   : 2;           // number of pars (3 max)
uint bkt      : 1;           // func must have brackets  ( + - / *  do not)
uint defctype : 4;           // default calctype
ushort nextnum;

void (*calc) (MHLD *);       // actual calc function (no mathx ?)

} MXF;


#endif

