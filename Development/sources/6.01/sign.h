/**************************************************
* Signature defines
* Signatures are like a 'regular expression' matcher
* a set of pattern matches to find specific items in the binary.
* for function lookups, rbase lists, and so on.
*************************/

#ifndef _XSIGNX_H

#define _XSIGNX_H 1

#include "shared.h"



/*************************************
 * DESCRIPTION
 *
 * Each Signature entry begins with its own header, with name and handler subroutine.
 *
 *each full sig starts with header (sx) which is -

 sx[0]             header size = 7 minimum. (= 1 char name+terminator)
 sx[1]             pattern number (from pattern, for duplicate checks)
 sx[2]             flags
 sx[3]             handler subroutine index
 sx[4]             fixed parameter - goes into lvals[0]
 sx[5]             name from here to end of header (with zero terminator)
  *
 * patterns then start at sig + sig[0] to skip the header.

 flags (sx[2])

 1 no overlap allowed - for patterns that start with a repeat, mainly
 2 debug flag for reporting and tracing pattern mismatch
 4 copy parent array to child pattern
 8 copy child array back to parent if match

 * size of header
 -----    size of parameter array ?

 *
 * 'Match pattern' varibale headr, 4 by default
 *
 * bytes      For all patterns
 * [0]        Pattern/match type
 * [1]        Option Flags
 * [2]        Index (0-31) some have extra fields > 31. index into params array for saved and rematched items
 * [3]        Value,  to match or save - e.g opcode, operand or data (register, address, etc)
 * ([4]        sub signature number or low byte value)
 * ([5]        second index for save index)

 * Header types by number range (byte [0]
 *
 * 0x80 opcode pattern - followed by 0x10 - 0x13  operand patterns (0-3 is operand number)
 * 0x20 data pattern
 * 0x50 child signature

   plus modifiers
 * x0  standard match (e.g. one complete pattern)
 * x1  optional/repeat pattern match
 * x2  skip up to n patterns
 * x3  match x of y, in any order   (e.g. provides for 'a or b or c' as 1 of 3)
    (* x4  pattern defined by external parameter ? maybe)
 *
 */


// FIRST byte [0] - define match -  type+header.   opcode, data, child sig.
// 'index' is which array index to save the value , if set.  in [2] as standard

#define  SIGOCOD  0x80     // hdr 4 bytes   - opcode match,                      [1] flags,         [2] index,       [3] opcode value to match.   Data/operand pats may follow.
#define  SIGORPT  0x81     // hdr 4 bytes   - repeat/optional opcode             [1] min matchcnt   [2] num match    [3] max match,  = num patterns
#define  SIGOSKP  0x82     // hdr 4 bytes   - skip opcode 'skip to x'            [1] max skips      [2] num skips    [3] step back or consume matching op - make this a flag ??
#define  SIGOSUB  0x83     // hdr 4 bytes   - match x of y opcodes               [1] min match      [2] index        [3] max = num patterns (x of y)


#define  SIGCHLD  0x50     // hdr 2 bytes   - child sig pattern                  [1] child sig (index)
#define  SCHORPT  0x51     // hdr 6 bytes   - repeat/optional child sig          [1] min matchcnt  [2]  num match    [3] num sigs/max,  [4] child sig index  [5] 2nd save index
#define  SCHOSUB  0x53     // hdr 4 bytes   - match x of y child sigs            [1] min match     [2]  save index   [3] num child/max sigs



// SIGOCOD followed by 0 - 3  OPERAND matches  [0] = 0x10,11,12,13    // 4 or 5 bytes       [1] flags,        [2] save index,   [3] byte to match  opt - [3] (high byte), ([4] low byte)


//**** See note on child sigs below ***** //

// #define SCHSUB  0x53     // X of Y as child sigs.....                   [1] min match,     [2]  index    [3] is max = num sigs (x of y) followed by 0x50...
//#define  SCHPAR   0x54     // child sig defined by parameter  ?          [1]  index    may be useful...

#define  SIGDATA  0x20     // 4 bytes data pattern only, no operands        [1] flags, [2] index, [3] value to match


// SECOND byte [1] -  option flags

// Opcode option flags  (0x80 start)
//maybe have flag to extend a byte for more flags ?  make nibbles for easier read ?

// #define SIGOPSZ 0x40        // save operand size (in bytes) must be [3] if in opcode...move to operand ? change to nibbles

#define  SIGFE    0x8         // save/match sign bit (0xfe)  (index split, 1 + 5 bits) must match if ix=0
#define  SIGSOPC  0x4         // save/match opcode address mode. (index split 2+5 bits, (sub (3), index (31) what about increment ? in operand ?
#define  SIGBIT   0x2         // save/match bit number (as mask) for bit jump  (index split into 4+4 bits = bit 0-7,  must match if ix & 80)


//OPERAND option flags  (0x10, 11, 12, 13)

// opsize ?? could use for fnplu etc.

#define  SIGAINC  0x20       // autoinc must be set         - could do in opcode ? or better as optype ? not in opcsub
#define  SIGLBT   0x10       // drop lowest bit of register for match (some autoinc and top/bottom byte ops)
#define  SIGWORD  0x8        // word value (5 byte field).  (ignores bank).
#define  SIGLST   0x4        // increment index to build list (for loops)
#define  SIGIGN   0x2        // ignore value (but can keep value via ix)
#define  SIGADDR  0x1        // force address instead of default for optype

// must be increment ?? register and offset via 2 terms ??  add split index as in opcode ?  (ix + optype same as ix + bitno)

//NB.  can use same operand more than once , and also [0] for jump offsets ([1] for address)

// NB. save by index dodgy if saving a zero!!  probably OK for all registers etc. but still logically wrong....

//**** note for child sigs ****
//  for all calls of child sig (0x5x) can auto-copy pars array up and down.
// CARE - for 0x51 child sig can be called multiple times, so lists must be APPENDED and a count kept somewhere in the caller for each list, as params in the child sig
// restart for each call.


//********************************




typedef struct match
{       // signature state holder, master and child

struct match *caller;  // calling (parent) MMH or zero
INST    *cinst;       // instance for this pattern (has scan block ptr..malloced
SBK     *scan;        // malloced

uchar   *sx;      // pointer within current pattern

uint  *svals;     // temp saved values for repeat/optional match patterns amlloced
uint  *lvals;     // local values.  malloced

uint hix  ;       // child handler
uint msize;       // malloc size for block for above pointers

//uint arraysize;  // to have separate array sizes and for sig ?

uint patno;       //pattern number from signature
// may need ofst in here for subsigs....
// maybe 2 matchcounts ?
// 10 with debug

//#ifdef XDBGX
//uint subno;
uint sigstart;           //debug
uint dbgflag  : 1;
//#endif

//26
uint  cinx1     : 5;   // caller index (0-31)
uint  cinx2     : 5;   // caller index
uint  mc        : 4;   // current match count, repeats, optionals, etc
uint  opno      : 2;   // operand number
uint  novlp     : 1;   // no overlap of sig with another
uint  sigmatch  : 1;   // sig matches (so far)
uint  skipcod   : 1;   // redundant opcodes can be skipped (for front overlaps)
uint  failok    : 1;   // so sigfail csn be skipped
uint  cpyarrin  : 1;   // copy parent lvals into child (for matching and tfr)
uint  cpyarrout : 1;   // copy child array back to parent if match
} MMH;


// sig handler processor index, linked inside each pattern.

typedef struct sigpxx
{
     void  (*pr_sig) (SIG *, SBK*);                // sig processor or zero if none
} SIGP;

//child sig processor, linked by mmh

typedef struct chldsb
{
     void  (*pr_sig) (MMH *);                // sig processor or zero if none
} CHLDSUB;






















//*********************************************external declarations

int    g_byte       (uint);

void   show_prog    (void);
SBK*   do_code      (SBK *, INST *);

void* cmem     (void *, size_t, size_t);
void  mfree    (void *, size_t);
void* chimem    (uint);
CHAIN* get_chain(uint);

SIG*   add_sig      (SIG *);
SIG*   get_sig      (uint) ;

uchar  get_opc_inx    (uchar);
uint   get_cmdopt     (uint);
uint   get_anlpass    (void);

const OPC* get_opc_entry  (uchar);

uchar get_opc_inx     (uchar);
BANK *get_bankmap(uint bank);


uint   val_rom_addr   (uint addr);
uint   databank       (uint addr, INST *);


const OPC*   get_opc_entry (uchar);

#ifdef XDBGX
uint DBGPRT        (uint, cchar*, ...);
#endif



#endif