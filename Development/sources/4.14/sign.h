/**************************************************
* Signature defines
* Signatures are like a 'regular expression' matcher
* a set of pattern matches to find specific items in the binary.
* for function lookups, rbase lists, and so on.
*************************/

#ifndef _XSIGNX_H

#define _XSIGNX_H 1

// #include "core.h"
#include "shared.h"




/*************************************
 * DESCRIPTION
 *
 * Each Signature entry begins with its own header, with name and handler subroutine.
 *
 * byte num
 * [0] size of header (minimum is 1 for no header)
   then optional entries determined by size in [0]
 * [1] flags for this sig
 *      1    no overlaps allowed.
 *      2    debug flag
 * [2] sig handler index
 * [3] extra parameter 0-255 for ID or other uses
 * [4] sig name (mainly for debug)
 *
 * patterns then start at sig + sig[0] to skip the header.
 *
 *
 * 'Match pattern' is 4 bytes by default, some are different size.
 *
 * bytes                      For all patterns
 * 0xff ff 00 00   [0], [1]   Type, Flags, first two bytes
 * 0x00 00 ff 00   [2]        2 Indexes 0-15, 16-31, into params array for saved and rematched items
 * 0x00 00 00 ff   [3]        Value,  to match or specify - e.g opcode, operand or data (register address or immediate value etc.)
 *
 *
 * Header types by number range
 *
 * x80 opcode + x10 per operand
 * x20 data pattern
 * x50 child signature -- not all of size 4

 * x0  standard match (e.g. one complete opcode)
 * x1  optional/repeat match
 * x2  skip up to n patterns
 * x3  match x of y, in any order (NB. provides for 'a or b or c' as 1 of 3)
 * x4  pattern defined by external parameter ? maybe
 *
 */


// FIRST byte [0] - define match type, as a header block.  opcode, data, operands, child sigs, etc.  each item in header.

#define  SIGOCOD  0x80     // 4 bytes opcode pattern,                      [1] flags, [2] index, [3] value to match. Data/operand pats may follow.
#define  SIGOPER  0x10     // 4 bytes operand/data pattern                 [1] flags, [2] index, [3] value to match

#define  SIGORPT  0x81     // 4 bytes opcode repeat/optional               [1] min matchcnt,  [2]  index,   [3] is max match = num patterns
#define  SIGOSKP  0x82     // 4 bytes opcode skip pattern 'skip to x'      [1] max skips
#define  SIGOSUB  0x83     // 4 bytes opcode sub pattern order independent [1] min match,     [2]  index    [3] is max = num patterns (x of y)

#define  SIGCHILD 0x50     //  2 bytes pattern is child sig                [1] sign index
#define  SCHORPT  0x51     //  6 bytes child sig repeat/optional           [1] min matchcnt,  [2]  save index,   [3] is no sigs/max, [4] subsig index [5] 2nd save index
// **** See note on child sigs below ***** //

// #define SCHSUB  0x53     // X of Y as child sigs.....                   [1] min match,     [2]  index    [3] is max = num sigs (x of y) followed by 0x50...
//#define  SCHPAR   0x54     // child sig defined by parameter  ?          [1]  index    may be useful...

#define  SIGDATA  0x20     // 4 bytes data pattern only, no operand        [1] flags, [2] index, [3] value to match


// SECOND byte [1] -  define flags

// opcode flags  (0x80 start)
#define  SIGSOPC  0x40        // save opcode address mode of opcode.
#define  SIGMOPC  0x20        // must match  multiaddress mode of opcode.
#define  SIGBIT   0x8         // save bit number for bit jump (and others).
#define  SIGADD   0x4         // save actual destination address for jumps (offset if not set)
#define  SIGFE    0x2         // save if sign bit (0xfe)

//operand flags  (0x10 start)

// #define SIGPOP  0x10     //special pop repeat ??   do in handler, list is there.........  index in list of start+max ??

#define  SIGINX   0x8        // alternate indexing for repeats.
#define  SIGLBT   0x4        // drop lowest bit of register (for autoinc and some byte ops)
#define  SIGIGN   0x2        // allow any value (ignore)
#define  SIGBNK   0x1        // add current bank (for addresses) - jumps done already



//**** note for child sigs ****
//  for all calls of child sig (0x50,0x51) it is the responsibility of the handler subr to copy parameters upwards.
// for 0x51 child sig can be called multiple times, so lists must be APPENDED and a count kept somewhere in the caller for each list, as params in the child sig
// restart for each call.


//********************************




typedef struct match
{       // signature state holder, master and child

struct match *caller;  // calling (parent) MMH or zero
INST    *c;       // instance for this pattern (has scan block ptr..
SBK     *scan;

uchar   *sx;      // pointer within current pattern

uint  *svals;     // temp saved values for repeat/optional match patterns
uint  *lvals;     // local values.  malloced if localv set (below), or points to sig values

char  *sname;     // temp for sig name debug

uint hix  ;       // child handler
uint msize;       // malloc size for block for above pointers


// may need ofst in here for subsigs....
// maybe 2 matchcounts ?
// 10 with debug

//#ifdef XDBGX
uint sigstart;           //debug
uint dbgflag  : 1;
//#endif

uint  opno      : 2;   // operand number
uint  patsub    : 2;   // opcsub from pattern
uint  novlp     : 1;   // no overlap of sig with another
uint  sigmatch  : 1;   // sig matches (so far)
uint  skipcod   : 1;   // redundant opcodes can be skipped (for front overlaps)
uint  highb     : 1;   // match (extra) high byte only - word and long
uint  oswp      : 1;   // swop operands (for stx <-> ldx)

uint  mc        : 4;   // current match count, repeats, optionals, etc
uint  cinx1     : 5;   // caller index (0-31)
uint  cinx2     : 5;   // caller index
uint  par1      : 4;   // extra param to pass

} MMH;


// sig processor index, linked inside each pattern.

typedef struct sigpxx
{
     void  (*pr_sig) (SIG *, SBK*);                // sig processor or zero if none
} SIGP;


//child sig processor

typedef struct chldsb
{
     void  (*pr_sig) (MMH *);                // sig processor or zero if none
} CHLDSUB;


















//*********************************************external declarations

int    g_byte       (uint);

extern uint   cmdopts;         // command options
extern uint   anlpass;
extern BANK bkmap[];
extern OPC opctbl[];
extern uchar opcind[];
extern CHAIN chsig;


//uint   get_cmdopt   (uint);
//uint   get_anlpass  (void);
int    get_numbanks (void);
void   show_prog    (int);
void   do_code      (SBK *, INST *);

void* cmem (void *ptr, size_t osize, size_t nsize);
void  mfree(void *addr, size_t size);
//void*  chmem        (uint);

void* chmem(CHAIN*);
SIG*   add_sig      (SIG *);

uchar  get_opc_inx  (uchar);

uint   val_rom_addr (uint addr);
uint   databank     (uint addr, INST *);

//BANK* get_bkdata  (uint ix);



const OPC*   get_opc_entry (uchar);

#ifdef XDBGX
uint DBGPRT        (uint, cchar*, ...);
#endif



#endif