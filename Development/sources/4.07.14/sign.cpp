/*****************************************************
 *  Signature Declarations and includes
 ******************************************************/
#include "sign.h"

uint   maxpcadd    (uint);
uint   minadd    (uint);
uchar* map_addr  (int);
int    sjmp_ofst (uint);
int    wnprt     (const char *, ...);
int    wnprtn    (const char *, ...);
void*  chmem     (CHAIN *,uint);
SIG*   inssig    (SIG *);
void   clearchain(CHAIN *);
int    get_posn  (int);
void   show_prog (int);
RBT*   add_rbase (int, int);
LBK*   set_cmd   (int, int, int, int);

int    g_byte    (uint);
int    g_word    (uint);
int    g_val     (uint, uint, uint);


#ifdef XDBGX
uint DBGPRT( uint, const char* fmt, ...);
#endif

extern OPC   opctbl[];
extern uchar opcind[];
extern BANK  bkmap[];
extern int   cmdopts;
extern int   anlpass;
extern BASP basepars;
extern CHAIN chsig;


int bgv[NSGV];             // backup signature values
uint tempbank;

// see sign.h for description of SIGN arrays.

// startup jump for each bank (save jmp to see if loopstop)

uint initial[] =
{0xa0012010, 0x2000006};                // single jump (with destination in v[1])

// int_ignore (ret or retei)

uint ignint[] = {0x80001014};                        // single ret or reti


// this may now be covered by a vect3 ?

uint avct[] = {
0x8000300c,  0x4008a,  0x00022,               // ldw   R22,af8a       ALTSTACK = af8a;
0x00070000,  0x30201,                         // any order 2 patts, 1 must match, save in [3]
0x8000300c,  0x00011,  0x70011,               // (1) ldb   R11,11         BANK_SEL = 11;
0x8801200e,  0x6007b,                         // (2) push  247b           push(247b);
0x400f3000, 
0x86001014                                    //  skip up to 3 opcodes to a reta
};


//  timer signature  - this works for aa through to A9L to C3P2

// may need opcodes sooner to confirm bits for word/byte/sec/msec etc etc   ??
//  save contents of R30 for timer list start

/*
  Update_timers:  DOESNT work for EARS
68fe: a3,72,c0,3c       ldw   R3c,[R72+c0]   R3c = Eighths_ext;             # 1/8s remainder (in mSec)
6902: a1,b1,69,30       ldw   R30,69b1       R30 = Timer_list;
6906: a0,06,36          ldw   R36,R6         R36 = IO_TIMER;
6909: 4b,72,c2,36,38    sb3w  R38,R36,[R72+c2] R38 = R36 - Loop_time;       # IOTIMER delta time since last mSec
690e: 01,3a             clrw  R3a            R3a = 0;
6910: 0d,07,38          shldw R38,7          R38L *= 80;
6913: 8d,73,cb,38       divw  R38,cb73       R38 = R38L / cb73;             # *128/52083 = 1 mSec
6917: 88,00,38          cmpw  R38,0
691a: d7,02             jne   691e           if (R38 == 0)  {
691c: 20,92             sjmp  69b0           return; }                      # < 1 mS passed - return

691e: c7,74,5e,38       stb   R38,[R74+5e]   Bg_timer = R38;                # time since last timer update (mSecs)
6922: 65,40,00,3a       ad2w  R3a,40         R3a += 40;                     # (remainder IO times*128) + 64 (rounding)
6926: 08,07,3a          shrw  R3a,7          R3a = R3a / 80;                # return to IOTIMES remainder
6929: 48,3a,36,32       sb3w  R32,R36,R3a    R32 = R36 - R3a;               # effective time of this update (IOTIME-remainder)
692d: b1,20,3e          ldb   R3e,20         R3e = 20;                      # set Lmillisecs, = "new mS"
6930: 74,38,3c          ad2b  R3c,R38        R3c += R38;                    # add new mSs to current mS count
6933: 59,80,3c,3f       sb3b  R3f,R3c,80     R3f = R3c - 80;                # R3f +ve if new 1/8 sec
6937: d3,0f             jnc   6948           if ((uns) R3c >= 80)  {
6939: b0,3f,3c          ldb   R3c,R3f        R3c = R3f;                     # remainder =- 1/8 sec (in mS)
693c: 91,40,3e          orb   R3e,40         Leighths = 1;                  # = "New 1/8 Sec"
693f: e0,3d,06          djnz  R3d,6948       R3d--;
                                             if (R3d !=  0) goto 6948;      # 1 Sec remainder (in 1/8 secs)
6942: 91,80,3e          orb   R3e,80         Lseconds = 1;                  # set "New second" (when R3d = 0)
6945: b1,08,3d          ldb   R3d,8          R3d = 8; } }                   # reset 1 sec remainder (= 8/8, counts down)
6948: c3,72,c0,3c       stw   R3c,[R72+c0]   Eighths_ext = R3c;             # store 1/8 sec remainder (in mSec)
694c: c3,72,c2,32       stw   R32,[R72+c2]   Loop_time = R32;               # save IOTIME of last timer update (a true mS)
   */



uint times [] = {

0x8000300c,  0x10030,  0x2003c,          //ldb   R3c,[R30++]
0x8000300a,  0xf0000,  0x2003c,          //cmpb  R3c,0  (and save exit value)
0x80002013, 0x2000055,                    // je    +55      # end of list, return
0x900e300c,  0x10030, 0x4070032,        // ldzbw R32,[R30++]  ** one or two ldx bytes (byte or word)
0x80003017,  0x2003c, 0x00012,        // jnb   B0,R3c,3e93    # 2/3 word entry, skip mask check
0x8000300c,  0x10030, 0x3003d,        // ldb   R3d,[R30++]    # 4/5 word entry, get flags mask

0x8000300c,  0x10030, 0x40034,        // ldzbw R34,[R30++]    # get actual flags/register
0x80003005,  0x40034, 0x3003d,        // an2b  R3d,[R34]      # and mask them
0x80003017,  0x2003c, 0x0004,          // jnb   B3,R3c,3e91    # Result <> 0 to count
0x80002013,    00004,                      // je    +4             # Result == 0 to count
0x80002010,  0x20000e2,                      // sjmp  (back to start)
0x80002013,  0x20000e0,                      // je    (back to start)
0x80004005,  0x2003c, 0x5003e, 0,  // an3b  0,R3e,R3c      # mask for time bits.
0x80002013, 0x20000da,                      // je    (back to start) # No time bits overlap - skip count
0x80003017,  0x2003c, 0x5,          // jnb   B1,R3c
0x8000300c,  0x70032, 0x60036,        // ldw   R36,[R32]   # Count size is word
0x80002010,  0x00003,                      // sjmp  +3
0x8000300c,  0x70032, 0x60036,        // ldsbw R36,[R32]   # Count size is byte
0x80003017,  0x2003c, 0x4,          // jnb  B2,R3c,+4  # up or down
0x80002002,  0x60036                    // negw  R36       # Negate if counting DOWN
};


// making opcodes optional fixes BWAK, which has separate funcs for each type...

// ALSO note that AA has jnb, A9L has jb, reversing the sense !!
//can do opposites by checking 0-7 vs 8-15 of opcode, use sgv[17] as marker flags ?


uint fnlu[] = {      //  byte AND WORD function lookup signature  make * optional ?

 // why was the signed check made optional - for A9L hot wire ? BWAK ??

0x8000400a,  0x40032, 0x20002, 0x80034 ,   // cmpb/w  R34,[R32+2] or 4

0x880b3017,  0x300f6,  0x04,              // *jnb   B1,Rf6,x keep bit - Signed IN ?  **JB**
0x880b2013,  0x000a,                      // *jc    x keep carry state - goto unsigned
0x80002010,  0x0002,                      // *sjmp  x

0x80002013,  0x0006,                     // jge   x

0x80003006,  0x20002,  0x40032,            // ad2w  R32,2 (or 4)
0x80002010, 0x20000ed,                     // sjmp  x
0xc0003005, 0x20000df,  0x300f6,           // optional an2b RF6
0x8000300c,  0x40032,  0x60036,            // ldb   R36,[R32++]
0x8000300c,  0x40033,  0x70038,            // ldb   R38,[R32++]
0x80003007,  0x40032,  0x60036,            // sb2b  R36,[R32]
0x80003007,  0x40033,  0x80034,            // sb2b  R34,[R32++]
0x80003007,  0x40032,  0x70038,            // sb2b  R38,[R32]

0x880a3017,  0x300f6,  0x0004,            // *jnb   B1,Rf6,x   THIS is signed OUT  **JB***
0x880a2013,  0x0009,                     // *jc, x           these 4 were opt
0x80002010,  0x0002,                     // *sjmp, x
0x80002013,  0x0005,                     // *jge. x

0x80003009, 0x2000001,  0x300f6,            // orb, f6, 1
0x80002002,  0x70038,                      // negb R38
0x80003008,  0x80034,  0x70038,            // ml2b  R38,R34
0x8000300b,  0x60036,  0x70038,            // divb  R38,R36
0x80003017,  0x300f6, 0x200002,          // jnb B0,Rf6, x
0x80002002,  0x70038                          // negb R38

};


/*
2deb: d3,0b               jnc   2df8             if (R50 >= [R52])  {
2ded: 69,04,00,52         sb2w  R52,4            R52 -= 4;                         # back up in MAF transfer function Table to the right value
2df1: 8a,52,50            cmpw  R50,[R52]        
2df4: d9,f7               jgtu  2ded             if (R50 > [R52]) goto 2ded;       # loop
2df6: 20,0d               sjmp  2e05             goto 2e05;  }

2df8: 65,04,00,52         ad2w  R52,4            R52 += 4;                         # jmp ahead in the MAF transfer function to the right value
2dfc: 8a,52,50            cmpw  R50,[R52]        
2dff: d3,f7               jnc   2df8             if (R50 < [R52]) goto 2df8;       # loop
2e01: 69,04,00,52         sb2w  R52,4            R52 -= 4;
2e05: a2,53,58            ldw   R58,[R52++]      R58 = [R52++];                    # interpolate larger MAF
2e08: a2,53,54            ldw   R54,[R52++]      R54 = [R52++];                    # larger air flow
2e0b: 6a,52,58            sb2w  R58,[R52]        R58 -= [R52];                     # MAF difference
2e0e: 6a,53,50            sb2w  R50,[R52++]      R50 -= [R52++];                   # MAF delta to interpolate
2e11: 6a,52,54            sb2w  R54,[R52]        R54 -= [R52];                     # air flow difference
2e14: 6c,50,54            ml2w  R54,R50          lR54 *= R50;
2e17: 8c,58,54            divw  R54,R58          wR54 /= R58;
2e1a: 66,53,54            ad2w  R54,[R52++]      R54 += [R52++];                   # interpolated air flow
2e1d: 69,04,00,52         sb2w  R52,4            R52 -= 4;
2e21: c3,76,b4,52         stw   R52,[R76+b4]     Maf_ptr = R52;                    # Save last MAF tfr func ptr - for speed?
*/



/* cut down version for MAF_tfr lookup (A9L and others)

uint fstlu[] = {      //  fast lookup version of fnlu

 // why was the signed check made optional - for A9L hot wire ? BWAK ??

//0x8000400a,  0x40032, 0x20002, 0x80034 ,   // cmpb/w  R34,[R32+2] or 4

//0x880b3017,  0x300f6,  0x04,              // *jnb   B1,Rf6,x keep bit - Signed IN ?  **JB**
//0x880b2013,  0x000a,                      // *jc    x keep carry state - goto unsigned
//0x80002010,  0x0002,                      // *sjmp  x

//0x80002013,  0x0006,                     // jge   x

//0x80003006,  0x20002,  0x40032,            // ad2w  R32,2 (or 4)
//0x80002010, 0x20000ed,                     // sjmp  x
//0xc0003005, 0x20000df,  0x300f6,           // optional an2b RF6

0x8000300c,  0x40032,  0x60036,            // ldw   R36,[R32++]
0x8000300c,  0x40033,  0x70038,            // ldw   R38,[R32++]
0x80003007,  0x40032,  0x60036,            // sb2w  R36,[R32]
0x80003007,  0x40033,  0x80034,            // sb2w  R34,[R32++]
0x80003007,  0x40032,  0x70038,            // sb2w  R38,[R32]

0x80003008,  0x80034,  0x70038,            // ml2w  R38,R34
0x8000300b,  0x60036,  0x70038             // divw  R38,R36
//ad2w
//sb2w

};

*/




/* table signatures


 
 *   Sub10:
371f: 37,2d,1a            jnb   B7,R2d,373c      if (Tblsflg = 0) goto 373c;
3722: bc,33,3a            ldsbw R3a,R33          R3a = (int)R33;                   # SIGNED interpolate calc
3725: bc,31,3c            ldsbw R3c,R31          R3c = (int)R31;
3728: 68,3c,3a            sb2w  R3a,R3c          R3a -= R3c;
372b: ac,30,3c            ldzbw R3c,R30          R3c = (uns)R30;
372e: fe,6c,3a,3c         sml2w R3c,R3a          (long)R3c = R3c * R3a;
3732: c0,3a,3c            stw   R3c,R3a          R3a = R3c;
3735: 74,31,3b            ad2b  R3b,R31          R3b += R31;
3738: 37,3b,0f            jnb   B7,R3b,374a      if (B7_R3b = 1)  {
373b: f0                  ret                    return;

 * 
c 373c: 5c,33,30,3a         ml3b  R3a,R30,R33      (int) R3a = R30 * R33;            # UNSIGNED interpolate calc (AA)
c 3740: 5c,31,30,36         ml3b  R36,R30,R31      (int) R36 = R30 * R31;
7 3744: 68,36,3a            sb2w  R3a,R36          R3a -= R36;
6 3747: 74,31,3b            ad2b  R3b,R31          R3b += R31; }
6 374a: 65,80,00,3a         ad2w  R3a,80           R3a += 80;

 * 374e: f0                  ret                    return;
 *
 
 */
 
uint  ttrps[] = {     // table interpolate - signed. flag is SET for signed

0x980e2001,  0x2000033,                  // multiple clrb
0x880a3017,  0x3002d,   0x7001a,         // jnb   B7,R2d, x    table signed flag [10];
0x40098000,                            // skip up to 8 opcodes, (cnt in [9])
0x86003008,  0x2003a,   0x5003c         // sml2w R3c,R3a     MUST HAVE fe
};

uint  tblu [] = { // table lookup (byte) signed & unsigned

    // tab addr in [3], cols in [4]

0x80004008,  0x80033,  0x30034,  0x90036,     // ml3b  R36,R34,R33
0x80003006,  0x60031,  0x90036,               // ad2b  R36,R31

0xC0003006,  0x00,     0x50037,              // adcb  R37,0  OPT   Late LU
0xC0002013,  0x02,                           // jnc carry   OPT   Early LU
0xC0002019,  0x50037,                        // incb R37    OPT   Early LU

0x80003006,  0x90036,  0x40038,              // ad2w  R38,R36
0x8000300c,  0x40039,  0x60031,              // ldb   R31,[R38++]
0x8000300c,  0x40038,  0x80033,              // ldb   R33,[R38]
0xa0002011, 0x200001c,                       // interpolate (save call)
0x80003006,  0x30034,  0x40038,              // ad2w  R38,R34
0x8000300c,  0x40038,  0x80033,              // ldb   R33,[R38]
0x80002018,  0x40038,                        // decw  R38
0x8000300c,  0x40038,  0x60031,              // ldb   R31,[R38]
0x8000300c,  0x7003a,  0x30034,              // ldw   R34,R3a
0x80002011, 0x200000c                        // interpolate
};



uint tbl2 [] = {  // later table lookup

      // tab addr in [3], cols in [4] 

0x80004008,  0x80037,  0x30038, 0x1003a,      // ml3b R3a, R38, R37
0x80003006,  0x90035,  0x1003a,                 // ad2b R3a, R35
0x80003006,   0x00,    0x2003b,                 // adcb R3b, 0
0x80003006,  0x1003a,  0x4003c,                 // ad2w R3c, R3a
0x8000300c,  0x4003c,  0x90035,                 // ldb  R35, [R3c++]
0x8000300c,  0x4003c,  0x80037,                 // ldb  R37, [R3c]
0x80003006,  0x30038,  0x4003c,                 // ad2w R3c, R38  r3c tab addr[3] 38 is cols [4]
0x8000300c,  0x70034,  0x60039,                 // ldb  R39, R34
0x8000300c,  0xa0036,  0x2003b,                 // ldb  R3b, R36
0xa0002011, 0x200001c,                          // interpolate();
0x8000300c,  0x4003c,  0x80037,                 // ldb R37, [R3c]
0x8000400c,  0x4003c,  0x000ff,  0x90035,       // ldb R35, [R3c+ff]
0x8000300c,  0xb003e,  0x4003c,                 // ldb R3c, R3e
0x80002011, 0x200000c,                          // interpolate
0x8000300c,  0xb003e,  0xa0036,                // ldw R36, R3e
0x8000300c,  0x4003c,  0x70034,                 // R34 = R3c
0x8000300c,  0x2003b,  0x60039                   // R39 = R3b
};


uint enc24 [] = { // encoded addresses type 4 and 2
0x8000300c, 0x4080037,  0xa003a,                 // ldzbw R3a,R37;
0x80003005,  0x3001f, 0x4020037,                 // an2b  R37,0x1f (t4) or 0xf (t2)
0x80003004,     0x04,   0xa003a,                 // shrw  R3a,4
0xC0003005,     0x0e,   0xa003a,                 // an2b  R3a,e     not in type 2
0x80004006,  0xa003a,   0x500f0,  0x80036        // R36 += [R3a+f0];
};


uint enc13 [] = { // encoded addresses type 3 and 1 (A9L etc)

0x80003017,  0x4080042, 0x2000014,         // jnb   B7,R43,3cda  (drop bit)
0x8000300c,  0x4080042,  0x4003a,            // ldzbw R3a,R43      (drop bit)
0x000b0000,  0x303,                       // any order opcodes, mult, 11 entries, all 3 patts must match

0x80003005,   0xf,     0x4080042,         // an2b  R43,f
0x80003005,   0x30070,  0x4003a,            // an2b  R3a,70  (0xfe is a t1, 0x70 is a t3)
0x80003004,   0x3,      0x4003a,            // shrw  R3a,3

0x80004006,   0x4003b,  0x500f0, 0x80042,    // ad2w  R42,[R3a+f0]
0xC101300d,   0x9003e,  0x80042               // stw   R42,[R3e]    optional

};

uint rbse [] = {       // rbase array code

0x8000300c,  0x100f0, 0x20018,                     // ldw   R18,f0
0x8000400c,  0x0,     0x30020, 0x5001a,            // ldb   R1a,[0+2020];
0x8000300c,  0x60014, 0x7001c,                     // ldw   R1c,[R14++]
0x8000300d,  0x20018, 0x7001c,                     // stw   R1c,[R18++]
0x80003012,  0x5001a, 0x800f7,                     // djnz  R1a,d477
};



// process subroutine for sigs

void fnplu  (SIG*, SBK*);
void fsplu  (SIG*, SBK*);
void tbplu  (SIG*, SBK*);
void encdx  (SIG*, SBK*);
void ptimep (SIG*, SBK*);
void avcf   (SIG*, SBK*);
void rbasp  (SIG*, SBK*);

// *****  PATS are sign,name, sigproc, size.

// initial search sigs

// specifically called
PAT hdr    = {initial, "INIT"   , 0 , NC(initial) ,0 };         // initial jump
PAT intign = {ignint,  "IGNINT" , 0 , NC(ignint)  ,0 };         // ignore interrupt

// prescan sigs

PAT rbase =  {rbse,    "RBASE",  rbasp,  NC(rbse),   0};       // rbase lookup

PAT *prepats[] = {&rbase};          



//specifically called in tbplu, for signed check


PAT tint   = {ttrps,   "istrp"  , 0 , NC(ttrps)   ,0 };    // table interpolate (for signed/unsigned checks)

PAT *apats[] = {&tint};     




// scanned with code scans if  PSIGS  set

PAT avcl = {avct,   "AVCT" , avcf   , NC(avct)  ,0 };    // altstack vector list
PAT timr = {times,  "TIMER", ptimep , NC(times) ,0 };    // timer list
PAT fnlp = {fnlu,   "FnLU" , fnplu  , NC(fnlu)  ,0 };    // func lookup - byte or word
PAT tblp = {tbl2,   "TBL2" , tbplu  , NC(tbl2)  ,0 };    // table lookup, later multibank
PAT tblx = {tblu,   "TBLU" , tbplu  , NC(tblu)  ,0 };    // table lookup, early
PAT ec13 = {enc13,  "1ENC3", encdx  , NC(enc13) ,1 };    // encoded address types 3 and 1
PAT ec24 = {enc24,  "2ENC4", encdx  , NC(enc24) ,2 };    // encoded address type 4 and 2,

PAT *cpats[] = {&avcl, &timr, &fnlp, &tblp, &tblx, &ec13, &ec24};      //,&fnfs}; 


//************************************************************************



int do_datsig(SIG *z, uint foff, uint *sx, int psize, int dsize)
{
 /* data part of opcode signature
 * (ai) autoinc or int index is ALWAYS for first register....
 *  match the DATA after an opcode, return true or false
 * foff    file offset
 * sx      start point (data or operands) for pattern
 * psize   size of pattern
 * dsize   datasize and flags
 *******************************************/

int ans, ix;
uint sval,tval,pval, xval, max;          // for register vals
uint sg;

// OPCODE size and true ops in dsize, as well as patt size
// psize should equal real no of ops.  dsize is zero for data only match

ans = 1;

if (dsize)
 {                                // if opcode at front, check operands.
  ix = (dsize >> 4) & 0xf;        // true no of ops from opsize.
  if (ix != psize)  return 0;
 }

ix = 0;
max = maxpcadd(foff);


 if (dsize & 0x8000) 
  { //12 and 13 swop...do ops in reverse order
    ix = psize -1;
  }  


while (ans && ix < psize && ix >= 0)          // extra check for both ways...
   {
    if (foff >= max) return 0;        // hit end of file

    sg = sx[ix];

    if (sg & OPCOD) return 0;              // ->ix this is an opcode flag, stop

    sval = g_byte(foff);                       // next binfile value

    pval = sg & 0xff;                            // pattern - mod for autoinc fiddles
    tval = (sg >>16) & 0xf;                      // array index value 1f

    // drop lowest bit

    if ((dsize & 0x1000)|| (sg & SDLBT))
        {
         sval  &= 0xfe;                         // autoinc or int indexed, drop flag (value)
         pval  &= 0xfe;                         // drop flag (pattern)
         dsize &= 0xefff;                       // drop lowest bit request (0x1000)
        }

   //  do word check

    if ((ix == 0 && (dsize & 0x2000)) || (ix == 1 && (dsize & 0x4000)))
       {                      // convert to word sized operand (imd or inx data)
        xval = g_byte(foff+1);
        foff++;
        sval |= (xval << 8);
        if (sval > PCORG)          // only for non-regs
         { if (tempbank < 0x100000) sval |= tempbank; else sval |= basepars.datbnk;}    //correct address for inx
       }

     if (sg & SDINC)
       {                                             // increment value by repeat count for saved reg
         xval = tval;
         tval += z->v[(sg >> 8) & 0xf];              // 1f where repeat count is stored
         if (tval > NSGV) tval = xval;               // safety check to increment tval
       }

     if (tval && ans && (!z->v[tval] || (sg & SDANY))) z->v[tval] = sval;  // save register to index

    if ((sg & SDANY) == 0)   // not ignore value
      {
       if (tval)
          {
           if (sval != z->v[tval]) ans = 0;
          }
        else
          {
           if (sval != pval) ans = 0;
          }
       }

       
    if (dsize & 0x8000) ix--;  else ix++;         // next pattern (f or b)
    foff++;
    }

 return ans;   //  hit end of pattern
 }



//this is really BAD, to use tempbank................nasty....

uint do_jumpsig (int inx, uint foff, uint bank)
  {        // calc jump address for the different jump types
           // need SIGNED values in here

// need to change to calc-jump ops type which allows for wrap.....

   uint joff;

   joff = 0;               // jump offset

   if (bank >= 0x100000) bank = (foff & 0xf0000);    //get bank from foff
   
   if (inx > 49 && inx < 52) 
     {                                            // pushw
      joff = g_word(foff + 1);                    // direct address
      joff |= bank;                               // add bank
     }

   if (inx > 53 && inx < 84) 
     {     // jumps
      if      (inx > 69 && inx < 73)   joff = g_val(foff+2, 0, 39) + 3;  // JB, JNB, DJNZ  signed byte
      else if (inx > 80 && inx < 84)   joff = sjmp_ofst (foff)   + 2;  // short, special calc
      else if (inx > 72 && inx < 76)   joff = g_val(foff+1, 0, 47) + 3;  // long, signed word
      else if (inx == 80)  joff = 2;                                   // skip = addr + 2


      joff+=foff;                                                      // gets bank from foff
      joff &= 0xffff;                                                  // strip bank (and any wrap)
      joff |= bank;

      if (joff >= maxpcadd(foff)) joff = 0;
      if (inx > 75 && inx < 80) joff = 0;                              // Don't keep if a ret
     }
   return joff;
  }


int do_optopc(uint *foff, uint maxadd)
  {
    // skip any 'optional' opcodes.  max of 16 allowed.
    // ignore if f2-f7, fa-ff,

    int ans, opc;

    ans = 0;

    while(1)
      {
       opc = g_byte(*foff);                // code file value

       if (opc < 0xf2)  break;             // not skippable - break;
       if (opc == 0xf8) break;
       if (opc == 0xf9) break;

       (*foff)++;
       ans++;
       if (ans >= 15 ) break;              // skip up to 15 opcodes
       if (*foff > maxadd) break;
      }
    return ans;                            // number of skips
   }

int opsize(int inx, char opc, uint foff)
   {
    int x, gdsize;
    const OPC *ope;
    // change answer to include autoinc, op subtype, word
    // size  (&f), nops (&f0), inc (0x1000),  imd word (0x2000), inx word (0x4000)
    // swop operands (stx match to ldx) (0x8000)
    // nops is true number, so +1 for indexes

    ope = opctbl + inx;
    
    x = ope->nops;

    gdsize = x+1;          // basic size - num operands (2 bits) plus 1 opcode

    if (inx >= 10 && inx <=53)                 // multiple address mode types only
      {

       switch (opc & 3)
       {
        case 1:
          if ((ope->rfend1 & 31) > 7)     // bigger than byte
            {
             gdsize++;
             gdsize |= 0x2000;                                 // imd word
            }
          break;

        case 2:
          if (g_byte(foff+1) & 1) gdsize |= 0x1000;            // autoinc (set drop lowest bit)
          break;

        case 3:
          if (g_byte(foff+1) & 1)
            {
             gdsize += 2;
             gdsize |= 0x5000;                                 // inx word (+ drop lowest bit)
             }
          else gdsize ++;
          x++;                                              // extra op for index
          break;

        default:                           // this for non 'M' types too....
          break;
        }
      }

  if (inx > 76 && inx < 80)  gdsize ++;               // long jumps have extra byte

  gdsize |= (x << 4);            // keep true numops for matching

  return gdsize;
}

int check_sign(int foff,int sgstate)
{
	// check whether opcode has a sign prefix in front

  int val, ans;
  
  ans = 0;
  val = g_byte(foff-1);          // get previous value

  if (sgstate & OPSVL)
     {              // sign required
      if (val == 0xfe)  ans = 1;
     }
  else
     {
      if (val != 0xfe)  ans = 1;	 
	 }

return ans;		
	
}




int do_sgentry(SIG *z, uint **sz, uint foff)
 {                 // do ONE sign entry (code+data)

  int ans, skp, rpt;
  uint max;
  int opc, inx, tval, xval, sval;
  int psize, gdsize;
  OPC *opl;
  uint *sx, sg;

  sg = **sz;
  sx = *sz;

  psize = (sg >> 12) & 0xf;             // size of this pattern (inc data)  ADDED OUTSIDE SUBR
  ans  = 1;                             // 1 is match
  if ((sg & OPRPT) && (sg & OPSPV)) rpt = 1; else rpt = 0;  // allow optional repeat
  max = maxpcadd(foff);

  if (foff >= max)  return 0;           // foff+psize ?

  if (!(sg & OPCOD))
       {
        // CAN have just data (subr pars for example)
        ans = do_datsig(z,foff, sx, psize, 0);  // data match, no opcode size
        foff += psize;                        // psize, gdsize is not reliable here !
        *sz += psize;                         // and move pointer
        if (ans) return foff;             //continue;
        return -foff;                     // no match.
       }


   // opcode and operands

 while (ans)       //  need loop for repeat and optional patterns...
   {
    skp = do_optopc(&foff, max);

    if (foff >= max)  return 0;
    if (skp > 15)     return 0;          // too many skips (0xff)

    opc = g_byte(foff);          // code file value - skip prefixes in sigs

     // codbank and datbank OK as default  
     // but NOT for any subsequent opcodes, so check here

    tempbank = 0x100000;        // no bank change 
    if (opc == 0x10)
      {  // bank change
        sval = g_byte(foff+1);
        sval++;                  //for new offest banks
        if (sval >= 1 && sval <= 16)
          {        // valid, skip this and the bank value
           if (bkmap[sval].bok) tempbank = sval << 16;
           foff+=2;
           opc = g_byte(foff); 
          }
      }
    
    inx = opcind[opc];
    opl = opctbl+inx;               // opcode table entry

    if (opl->p65 && !P8065) inx = 0;   // check 8065
    if (!inx) return 0;                // invalid opcode, match fails

    xval = opl->sigix;                 // sval now sig group value but this only a char....128 max
    sval = sg & 0xff;

    gdsize = opsize(inx, opc, foff);

    if ((sg & OPSPX) && opl->sign)  ans = check_sign(foff,sg);

    if (sg & OPSPX && (xval + sval) == 25) gdsize |= 0x8000;               // mark STW or LDW
    else
    if (xval != sval) ans = 0;   // no opcode match (by sigix)
    

    tval = (sg >> 16) & 0xf;                                  // array index 1f

    if ((sg & OPJMP)) z->v[tval] = do_jumpsig (inx,foff, tempbank);                  // jump or call - save dest, and can be sgv[0] 

    if (inx > 9 && inx < 54 && (sg & OPSPV) && tval != (opc & 3)) ans = 0;           // check opcode subtype for multi modes
   
    if (tval && !z->v[tval] && (sg & OPSPV))
      {
       if (inx >= 70 && inx <= 71)                   //jnb (70) or jb (71)
        {
          z->v[tval] = (1 << (opc & 0x7));           // keep bit mask
          if (inx == 70)  z->v[tval] |= 0x1000;      // and state  JNB=1, JB=0 (JNB is swop bit)
        }
        
       if (inx == 56 || inx == 57 || inx == 101 || inx == 102)
          { if (inx & 1)  z->v[tval] |= 0x10000;    // save carry state for stc,clc,jc,jnc
		  }
      }

    if (inx > 9 && inx < 54 && (sg & OPSST))  z->v[tval] = (opc & 3);           // SAVE opcode subtype for multi modes

    if (sg & OPOPT) memcpy(bgv, z->v, NSGV*sizeof(int));          // save params

    if (ans) ans = do_datsig(z, foff+1, sx+1, psize-1, gdsize-1);   // try data match if opcode matches

    if (!ans)
      {
       // opcode or data params do not match
       // this is OK  IF optional pattern
       // OR end of a repeat patt with at least one match

       if ((sg & OPOPT) || ((sg & OPRPT) && rpt))
          {
          if (sg & OPOPT) memcpy(z->v, bgv, NSGV*sizeof(int));    // restore params
          ans = 1;
          rpt = 0;
		  *sz += psize;                         // and move pointer
          return foff;
          }
        }

    foff += (gdsize & 0xf);                      // next opcode in bin file

    if (!ans)  break;
    if (!(sg & OPRPT)) break;          // only continue (and loop) if ans and OPRPT set

    rpt++;                            // mark repeat
    if (tval)
      {
       z->v[tval]++;                      // count of repeats
       if (z->v[tval] > 255) ans = 0;     // safety limit max repeats
      }
 }            // end of while
 
  *sz += psize;                         // and move pointer
 if (ans) return foff;  else return -foff;

 }


int do_subsig(SIG *z, uint **sz, uint foff)
 {
   // subpattern with optional matches - max of 255 patts (ff) and 255 matches (ff)
   // updates pattern pointer (sz) Needs at least ONE match reqd (I think).
   
   uint *st, *end;
   uint  pattsz, sg;
   
   int ans, score, cmask, ix, psize, counts, max, save;

   // not sure if opt trailing works right
 
   pattsz = (**sz >> 16) & 0xff; 
   end = *sz + pattsz;                  // size of this subpattern (first word)

   sg = *(*sz + 1);                     // next word (subloop master data)
   score = sg & 0xff;                   // count of patts which must match (once) in lower byte
   counts = (sg >> 8) & 0xff;           // number of patterns - in upper nibble
   save = (sg >>16) & 0xf;              // 1f index to save matching mask to
   
   max = counts;       //score;                         // patterns to try
   cmask = 0;
   
   for (; counts > 0; counts--)         // set up 'match' bit mask
   {
     cmask <<= 1;                       // shift up one;
     cmask += 1;                        // add another bit
   }

   counts = 0;

   while (cmask && counts < max)
     {                                  // tries left
      st = *sz+2;                       // first sig of optional list
      ix = 1;                           // first sig index for bitmask

      while (st < end)                  // try all patts within (against cmask)
        {
         psize  = (*st >> 12 & 0xf);        // size of this pattern
         if (cmask & ix)
            {                                      // not matched yet
             memcpy(bgv, z->v, NSGV*sizeof(int));    //save pars
             ans = do_sgentry(z, &st,foff);           // do one (std) entry
             if (ans > 0)                          // matches
               {
                foff = ans;                    // move on in binary
                score --;                      // decr outstanding
                cmask &= ~ix;                  // clear this try flag
                if (save) z->v[save] |= ix;     // save match if reqd
               }
             else
               {
                memcpy(z->v, bgv, NSGV*sizeof(int));       //restore pars
               }
            }
			else st += psize;                             // move pattern if matched already
         ix = ix << 1;
        }      // st < end
      counts++;
     } // tmask

 if (score > 0) return 0;               // > 0 ?? some patt not matched

 *sz += pattsz;                         // and move to next patt
 return foff;

 }



 int do_skpsig(SIG *z, uint **sx, uint foff)
 {
   int ans, counts, max, tval;
   uint *st, sg, ptsz;

   // assumes skip down to ONE defined opcode at the moment....
   // may work for DATA too ?  but not an OPT pattern at the end...

   sg = **sx;
   max =  (sg >> 12) & 0xf;                // max no of opcodes to skip.
   tval = (sg >> 16) & 0xf;                // 1f where no of skips get saved

   sg = *(*sx+1);                          // get the match patt after the skip word
   ptsz = ((sg >> 12) & 0xf) + 1;          // size of that pattern inc match and start word

   counts = 0;

   while (counts < max)
     {                                            // tries left
      st = *sx + 1;                               // reset to 2nd word
      memcpy(bgv, z->v, NSGV*sizeof(int));         // save pars
      if (*st & 0x3f)        
        ans = do_sgentry(z, &st,foff);               // real entry (2nd word), swop ans
      else
        ans = do_subsig(z, &st,foff);                // subpattern 2nd word on (opts and/or variable order)
	  
      if (ans < 0)                                // no match
        {
         foff = (-ans);                           // next opcode (skip if fail)
         memcpy(z->v, bgv, NSGV*sizeof(int));      // restore pars
         counts++;                                // and try again
        }
      else
        {
         foff = ans;
         break;                                   // match
		} 
     }

   if (tval) z->v[tval] = counts;

   *sx += ptsz;                                  // add pattsize
   if (counts < max)  return foff;

 return ans;

 }



SIG* match_sig (PAT *ptn, uint xofst,uint ix)
{
  int ans;

  uint *sx, *end;
  uint sg, ofst, max, foff;
  SIG *z;                         // for signature chaining

  if (anlpass >= ANLPRT) return 0;

  ofst = xofst;
  max = maxpcadd(ofst);             // safety

  if (ofst < minadd(ofst)  || ofst >= max)  return 0;

  z = (SIG *) chmem(&chsig,ix);            // use block specified....
  z->ptn = ptn;

  // check for redundant opcodes at front
  z->skp =  do_optopc(&ofst, max);

  ans  = ofst;                    // non zero is match
  foff = ofst;                    // file binary offset
  sx = ptn->sig;                  // signature entry
  end = sx + ptn->size;           // end of sig

// sx now moved by each sub func called

  while (ans > 0 && sx < end)
   {
    if (foff >= max) return 0;   // fail if drop off end
    sg = *sx;
    
    if (sg & 0x3f)                      // allow up to 63 for sigix..
      ans = do_sgentry(z, &sx,foff);       // std pattern - do one entry
    else
     { // special patterns - sigix is ZERO
       if (sg & OPOPT)
        ans = do_skpsig(z, &sx,foff);         // 'skip to X' pattern
       else
        ans = do_subsig(z, &sx,foff);        // subpattern (opts and/or variable order)
     }
    if (ans > 0) foff = ans;                 // next opcode

    }       // end of while

  tempbank = 0x100000;

  if (ans > 0 && foff > ofst) 
    {  // must have at least one matched patt
      z->start = xofst;            // addr before skips
      z->end = foff;             // end of patt
      return z;                   // match
    }
 return 0;                        // no match
}


SIG* do_sig (PAT *ptn, uint xofst)
{
 SIG *z, *r;                         // for signature chaining

 z = match_sig (ptn, xofst,0);

 if (z && z->start) 
    {
      // add sign (in z) to sig chain
      r = inssig(z);              // insert sig into chain
      if (r != z) return r;       // duplicate, return original
      return z;
    }

  return 0;
 }



SIG* scan_asigs (int ofst)
{
// scan ofst for modifier (signed) signatures

  uint j;
  SIG *s;

  if (PMANL) return 0;
  if (!(PSIG)) return 0;

  for (j = 0; j < NC(apats); j++)
      {
        s = do_sig (apats[j], ofst);        // code list
        if (s) break;                       // stop as soon as one found
      }
 
 return s;
}
 

SIG* scan_sigs (int ofst)
{
// scan ofst for signatures from list supplied. EXTERNALLY visible

  uint j;
  SIG *s;

  if (PMANL) return 0;
  if (!(PSIG)) return 0;

  // only scan cpats if PSIG set
  
  for (j = 0; j < NC(cpats); j++)
      {
        s = do_sig (cpats[j], ofst);        // code list
        if (s) break;                       // stop as soon as one found
      }
 
 return s;
}


void prescan_sigs (void)
{
 // scan whole binary file (exclude fill areas) for signatures first.
 // do preprocess if reqd

  uint ofst;
  uint i, j;
  SIG *s;
  BANK *b;

  if (PMANL) return;
  if (!(PSIG)) return;

  #ifdef XDBGX
   DBGPRT(1,0);
   DBGPRT(1," -- Scan whole bin for pre sigs --");
  #endif

  for (i = 0; i < BMAX; i++)
   {
     b = bkmap + i;
     if (!b->bok) continue;

     for (ofst = b->minpc; ofst < b->maxbk; ofst++)
       {
         show_prog (anlpass);

         for (j = 0; j < NC(prepats); j++)
            {
             s = do_sig (prepats[j], ofst);                  // prescan list
             if (s)
              {
               ofst = s->end;                          //sigs can't overlap
               if (s->ptn->pr_sig) s->ptn->pr_sig(s,0);  // do any required pre-processing
               break;
              }
            }
       }
   }

 #ifdef XDBGX
   DBGPRT(2," -- End Scan whole bin --");
  #endif
}


/// END of sign.cpp
















