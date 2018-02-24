/*****************************************************
 *  Signature Declarations and includes
 ******************************************************/
#include "sign.h"

// include a speed cheat - as sigs are sequential, and all access uses global buffer pointers,
// map ONCE for each do_sig, and keep for full sig check


int    maxadd    (int);
int    minadd    (int);
uchar* map_addr  (int);
int    negval    (int, int);
int    wnprt     (const char *, ...);
int    wnprtn    (const char *, ...);
void*  chmem     (CHAIN *);
SIG*   inssig    (SIG *);
void   clearchain(CHAIN *);
int    get_posn  (int);
void   show_prog (int);
RBT*   add_rbase (int, int);
LBK*   set_cmd   (int, int, int, int);


 #ifdef XDBG
 void prt_sigs(CHAIN *, int);
 #endif


extern OPC   opctbl[];
extern short opcind[];
extern BANK  bkmap[];
extern int   cmdopts;
extern int   anlpass;
extern int   datbank;
extern int   codbank;
extern CHAIN sigch;
extern int casz[];

uchar *sbf;                // 'speed cheat' buffer pointer for s_byte and s_word.
int bgv[NSGV];             // backup signature values
int sgv[NSGV];             // saved signature values
int tempbank;

// see sign.h for description of SIGN arrays.

// startup jump for each bank (save jmp to see if loopstop)

uint initial[] =
{0xa0012010, 0x2000006};                // single sjmp (with destination in v[1])

// int_ignore (ret or retei)

uint ignint[] = {0x80001014};                        // single ret or reti

// vector and/or push types for task lists

uint vect1[] = {                               // tasklist (style 1)

0x8000300a, 0x010032,  0x20098,                // cmpw  R98,32
0x80002013, 0x2000003,                          // jnc
0x80002001, 0x020098,                          // clrw R98
0x8803300e, 0x020098,  0x4004a,                // push[R98+3f9a], must be inx, STORED IN [4]
0x80001014                                     // ret
};


uint vect2[] = {                               // tasklist (indirect, style 2)

0x8000300a,   0x100a4, 0x2009a,                 // cmpb  R9a,a4
0x80002013, 0x200001c,                          // jc    2172
0x400e4000,                                    // skip up to 4 opcodes, (cnt in [14]) to a...
0x8000300c,   0x2009a, 0x30030,                 // ldzbw R30,R9a
0x80003006,   0x4008a, 0x30030,                 // ad2w  R30,218a   STORE IN [4]
0xa006200e, 0x2000051,                          // push  2151 - return addr, store as jump (for bank)
0x8802200e,   0x30030,                          // push [R30]
0x400f8000,                                    // skip up to 8 opcodes, (cnt in [15])
0x80001014                                     // ret
};


uint vect3[] = {                               // generic - push followed by ret (closely)

0xe008200e,  0x2000031,                         // optional front pushw, saved as jump (for bank).

0x00090000,  0x70301,                          // 3 patts, 1 must match, save match in [7])
0x8801200e,  0x50051,                          // (1) push  2151 (imd) store in [6]
//0xa025, 0x000e,  0x20, 0x0051,                          // (1) push  2151 (imd) store in [6]
0x8802200e,  0x60051,                          // (2) push [R30] (ind] store in [6]
0x8803300e,  0x20092, 0x40025,                 // (3) push  [R92+xx] (inx) store in [4]
0x400f3000,                                    // skip up to 3 opcodes, to a ret
0x80001014 
};



// this may now be covered by a vect3 ?

uint avct[] = {
0x8000300c,  0x4008a,  0x00022,                //ldw   R22,af8a       ALTSTACK = af8a;
0x00070000,  0x30201,                       // any order 2 patts, 1 must match, save in [3]
0x8000300c,  0x00011,  0x70011,              // (1) ldb   R11,11         BANK_SEL = 11;
0x8801200e,  0x6007b,                         // (2) push  247b           push(247b);
0x400f3000, 
0x80001014                         //  skip up to 3 opcodes to a ret
};

/* vect list with ALTSTACK

BWAK ...

8 2477: a1,8a,af,22         ldw   R22,af8a       ALTSTACK = af8a;
8 247b: f4                  bnk0
8 247c: b1,11,11
8 247f: b1,ff,9c            ldb   R9c,ff         R9c = ff;
8 2482: c9,7b,24            push  247b           push(247b);
8 2485: fe,f1               retia                return;



AMZ2 ?? background tasks
8 3073: a1,68,03,20           ldw   R20,368        STACK = 368;
8 3077: ef,c0,f0              call  213a           Sub47();
8 307a: a1,b2,a6,22           ldw   R22,a6b2       ALTSTACK = a6b2;
8 307e: f4                    bnk0
8 307f: b1,11,11              ldb   R11,11         BANK_SEL = 11;
8 3082: b1,ff,77              ldb   R77,ff         R77 = ff;
8 3085: c9,7e,30              push  307e           push(307e);
8 3088: fe,f1                 retia                return;


*/

// rbase (cal pointer) signature xdt2 has these first two in opposite order

uint rbse [] = {       // rbase array code

0x8000300c,  0x100f0, 0x20018,                             // ldw   R18,f0
0x8000400c,  0x0,     0x30020, 0x5001a,                   // ldb   R1a,[0+2020];
0x8000300c,  0x60014, 0x7001c,                             // ldw   R1c,[R14++]
0x8000300d,  0x20018, 0x7001c,                             // stw   R1c,[R18++]
0x80003012,  0x5001a, 0x800f7,                             // djnz  R1a,d477
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

//function and table lookup sub signature prefix - checked with and without end jump

uint sbpre1[] = {

0x80003009, 0xd0002, 0x300f6,     // orb   Rf6,n
0xc0003009, 0xe0004, 0x300f6,     // orb   Rf6,n   // optional second orb with new bit
0xe0062010, 0x2000006                // optional jmp, x - must hit master
};

uint sbpre2 [] = {     // alternate bit sets for signed/unsigned

0x8000300c,  0xd0001, 0x300f6,     // ldb   R22,1
0xe0062010, 0x200000c            //  optional sjmp  to master
};

// making opcodes optional fixes BWAK, which has separate funcs for each type...

// ALSO note that AA has jnb, A9L has jb, reversing the sense !!
//can do opposites by checking 0-7 vs 8-15 of opcode, use sgv[17] as marker flags ?


uint fnlu[] = {      //  byte AND WORD function lookup signature  make * optional ?

 //   need to add a remember which bit (or carry state) so can compare with value (f6 here)
 // why was the signed check made optional - for A(L hot wire ? BWAK ??

0x8000400a,  0x40032, 0x20002, 0x80034 ,   // cmpb/w  R34,[R32+2] or 4

0x880f3017,  0x300f6,  0x04,             // *jnb   B1,Rf6,x keep bit - Signed IN ?  **JB** was opt
0x880f2013,  0x000a,                      // *jc    x keep carry state goto unsigned was opt
0x80002010,  0x0002,                      // *sjmp  x was opt

0x80002013,  0x0006,                     // jge   x

0x80003006,  0x20002,  0x40032,            //,0x00, 0 , 0x32,4 ,   // ad2w  R32,2 (or 4)
0x80002010, 0x20000ed,                     // sjmp  x
0xc0003005, 0x20000df,  0x300f6,            // optional an2b RF6
0x8000300c,  0x40032,  0x60036,            // ldb   R36,[R32++]
0x8000300c,  0x40033,  0x70038,            // ldb   R38,[R32++]
0x80003007,  0x40032,  0x60036,            // sb2b  R36,[R32]
0x80003007,  0x40033,  0x80034,            // sb2b  R34,[R32++]
0x80003007,  0x40032,  0x70038,            // sb2b  R38,[R32]

0x880e3017,  0x300f6,  0x0004,            // *jnb   B1,Rf6,x   THIS is signed OUT  **JB***
0x880e2013,  0x0009,                     // *jc, x           these 4 were opt
0x80002010,  0x0002,                     // *sjmp, x
0x80002013,  0x0005,                     // *jge. x

0x80003009, 0x2000001,  0x300f6,            // orb, f6, 1
0x80002002,  0x70038,                      // negb R38
0x80003008,  0x80034,  0x70038,            // ml2b  R38,R34
0x8000300b,  0x60036,  0x70038,            // divb  R38,R36
0x80003017,  0x300f6, 0x200002,          // jnb B0,Rf6, x
0x80002002,  0x70038                          // negb R38

//6  ,0x8030 ,0x32, 4    ,0x38, 7,              // ad2w  R38,[R32]
//5  ,0x8030 ,0xfc, 0x20 ,0xf6, 3,              // an2b flag clear
};


// table signatures

uint subtb2 [] = {    // scaled table lookup (prefix) may need a sign marker at front ?

//9 , 0x9030, 0x2, 0xd,  0xf6, 3,     // orb   Rf6,n
0x80002001,  0x00031,                 //clrb R31       scaled
0x80003003,  0x20004, 0x30030,        //shlw R30,4
0x80002001,  0x00033,                 //clrb R33
0x80003003,  0x40004, 0x50032,        //shlw R32,4
0xe0062010, 0x2000003              // sjmp to master (and save it)
};


uint  intrp[] = {     // table interpolate - need this for signed table. flag is SET for signed

0x880d3017, 0x3002d, 0x200001a,     // jnb   B7,R2d, x    table signed flag;
0x8000300c, 0x10033,  0x2003a,     // ldsbw R3a,R33      SIGNED calc
0x8000300c, 0x40031,  0x5003c,     // ldsbw R3c,R31
0x80003007, 0x5003c,  0x2003a,     // sb2w  R3a,R3c
0x8000300c, 0x60030,  0x5003c,     // ldzbw R3c,R30
0x80003008, 0x2003a,  0x5003c      // sml2w R3c,R3a     (fe skipped)
};


uint  tblu [] = { // table lookup (byte) signed & unsigned

    // tab addr in [9], rows in [8], cols in [2] ??

//5 , 0xc030 ,0xfe,0xe ,0xf7,3 ,              // an2b rf7, fe    optional signed
0x80004008,  0x80033,  0x20034,  0x40036,    // ml3b  R36,R34,R33
0x80003006,  0x60031,  0x40036,             // ad2b  R36,R31

0xC0003006,  0x00,     0x50037,             // adcb  R37,0  OPT   Late LU
0xC0002013,  0x02,                     // jnc carry   OPT   Early LU
0xC0002004,  0x50037,                     // incb R37    OPT   Early LU

0x80003006,  0x40036,  0x90038,             // ad2w  R38,R36
0x8000300c,  0x90039,  0x60031,             // ldb   R31,[R38++]
0x8000300c,  0x90038,  0x80033,             // ldb   R33,[R38]
0xa0002011, 0x200001c,                    // interpolate (save call)
0x80003006,  0x20034,  0x90038,             // ad2w  R38,R34
0x8000300c,  0x90038,  0x80033,             // ldb   R33,[R38]
0x80002004,  0x90038,                     // decw  R38
0x8000300c,  0x90038,  0x60031,             // ldb   R31,[R38]
0x8000300c,  0x7003a,  0x20034,             // ldw   R34,R3a
0x80002011, 0x200000c                       // interpolate
};



uint tbl2 [] = {  // later table lookup   /swop over 1 and 3 ?

      // to match...... tab addr in [3], rows in [8], cols in [2] ??  move tabaddr to [9]
      //cross check via xdt2 r3c tab addr [3](OK)
      //R34,36 are dims, chosse 36 ?
      //R38 is col size [2]  OK

      // addr was 1 and size was 2 ??

0x80004008,  0x80037,  0x20038, 0x1003a,      // ml3b R3a, R38, R37
0x80003006,  0x40035,  0x1003a,                 // ad2b R3a, R35
0x80003006,   0x00,    0x3003b,                 // adcb R3b, 0
0x80003006,  0x1003a,  0x9003c,                 // ad2w R3c, R3a
0x8000300c,  0x9003c,  0x40035,                 // ldb  R35, [R3c++]
0x8000300c,  0x9003c,  0x80037,                 // ldb  R37, [R3c]
0x80003006,  0x20038,  0x9003c,                 // ad2w R3c, R38
0x8000300c,  0x70034,  0x60039,                 // ldb  R39, R34
0x8000300c,  0xa0036,  0x3003b,                 // ldb  R3b, R36
0xa0002011, 0x200001c,                          // interpolate();
0x8000300c,  0x9003c,  0x80037,                 // ldb R37, [R3c]
0x8000400c,  0x9003c,  0x000ff,  0x40035,       // ldb R35, [R3c+ff]
0x8000300c,  0xb003e,  0x9003c,                 // ldb R3c, R3e
0x80002011, 0x200000c,                          // interpolate
0x8000300c,  0xb003e,  0xa0036,                // ldw R36, R3e
0x8000300c,  0x9003c,  0x70034,                 // R34 = R3c
0x8000300c,  0x3003b,  0x60039                   // R39 = R3b
};

// this is currently marked as sub - not sure if reqd any more

uint sshift [] = {   // 8061 stack arguments version of func lookups - SINGLE LEVEL

0x8000200f,  0x60036,                      //popw  R36
0x8000300c,  0x60037,  0x10032,        // ldb   R32,[R36++]
0x8000300c,  0x60036,  0x90033,        // ldb   R33,[R36++]
0x8000300c,  0x90033,  0x20034,        // ldzbw R34,R33
0x80003005,  0xf,      0x90033,        // an2b  R33,f
0x80003003,  0x4,      0x20034,        // shrw  R34,4
0x80004006,  0x20035,  0x300f0, 0x10032,       // ad2w  R32,[R34+f0]
0x8000300c,  0x60037,  0x20034,        // ldb   R34,[R36++]
0x8000300c,  0x60037,  0x20035,       // ldb   R35,[R36++]
0x8000200e,  0x60036,                     // push R36
0x8000300c,  0x20034,  0x20034         // ldw   R34,[R34]
};



/*  NEW par getter STANDARD ....
 * 
 * [1] is level 1 (pops)
 * [2] is level 2 (pops)
 * [3]  is num of l1 pars
 * [4]  is num of l2 pars
 * [5] onwards is param register list (incremented)
 * 
 * others are free .....
 * change to allow [0] for some params ?
 * 
 */




uint vpar5 [] = {

0x8000300c,  0x20,      0xa0040,                  // ldw   R40,[STACK] this is PSW
0x80003003,  0x2,       0x40a0040,                 // shrb  R41,2
0x8400300c,  0x40a0040,  0x11,                     // ldb   R11,R41 or STW R41,11 - set callers bank

0x8000400c,  0x20,      0xf0002,  0x20a0040,       // ldw   R40,[R20+2] (l2)
0x9003300c,  0xa0040,   0x9003e,                  // ldb   R40,[R3e++]   l1 pars = becomes level 2 THIS in [9]
0xc000400d,  0x20,      0xf0002,   0xa0040,       // stw   R42,[R20+2]

0x8000400c,  0x20,      0x2000004,  0x20a0040,      // ldw   R40,[R20+4] (or new reg) THIS IS PSW
0x80003003,  0x2,       0x40a0040,                 // shrb  R40,2

0x8400300c,  0x40a0040,  0x11,                     // ldb   R11,R41  or STW R41, 11 -> set callers bank WORKS !!

0x8000400c,  0x20,      0xd0006,  0x20a0046,       // ldw   R46,[R20+6]
0x8002300c,  0xa0046,   0xb003f,                  // ldb   R3f,[R46++]                 v pars becomes level 1
0x9004300d,  0x2050026, 0xb003f,                  // stw   [R26++] = R3f;          THIS IN [5]
0x80003012,  0x9003e,   0x20000f7,                 // djnz  R3e  from above

0x8000300c,  0x11,      0x11,                     // ldb   R11,11       bank_sel register
0x8000400d,  0x20,      0xd0006,   0xa0046,       // stw   R46,[R20+6]
};

/* spar - single level extract   XTD2 8 4876     OK
  also BWAk has a version of this with a +4 (to cover a pushp)
 [1] is level (pops)
 [2] is num of pars
 [3] onwards is param list (incremented)   */


uint spar5 [] = {
0x8000300c,  0x20,   0xf0030,                    // ldw   R30,[R20] (stack)
0x80003003,  0x2,   0x40e0031,                 // shrb  R31,2

0x8400300c,  0x40e0031, 0x11,                      // ldb   R11,R31  or STW -> set callers bank WORKS !!

//0xc030, 0xc,  0x4e,  0x31,   0, 0x11,                    // ldb   R11,R31   set callers bank
//0xc030, 0xd,  0, 0x11, 0x4e,    0x31,                 // OR  stb R31, 11

0x8000400c, 0x20,    0x20f0002,  0xd0026,        // ldw   R26,[R20+2]   or +4
0x9003300c, 0xd0026, 0x8040336,                   // ldb   R36,[R26++] with  repeat
0x8000300c, 0x11,    0x11,                  // ldb   R11,11  bank_sel register
0x8000400d, 0x20,    0x20f0002,  0xd0026        // stw   R26,[R20+2]
};

// exit jump fixup for 8065 multibanks

uint xpar5 [] = {
0x8000400c,   0x20,   0x10002   , 0xf0046,       // ldw   R46,[R20+2]
0x80004006,  0x20007 ,0         , 0xf0046,       // ad2w  R46, 7  no of pars
0x8000400d,   0x20,   0x10002   , 0xf0046,       // stw   R46,[R20+2]
0x80001014                                         // ret
};


/* enc2 encode is - (EARS)

 6b46: ac,33,34          ldzbw R34,R33        R34 = (uns)R33;
6b49: 71,0f,33          an2b  R33,f          R33 &= f;
6b4c: 08,04,34          shrw  R34,4          R34 = R34 / 10;
6b4f: 67,35,f0,00,32    ad2w  R32,[R34+f0]   R32 += [R34+f0];

*/


uint enc24 [] = { // encoded addresses type 4 and 2
0x8000300c, 0x4080037,  0xa003a,             // ldzbw R3a,R37;
0x80003005,  0x3001f, 0x4020037,              // an2b  R37,0x1f (t4) or 0xf (t2)
0x80003003,     0x04,   0xa003a,                 // shrw  R3a,4
0xC0003005,     0x0e,   0xa003a,                 // an2b  R3a,e     not in type 2
0x80004006,  0xa003a,   0x500f0,  0x80036          // R36 += [R3a+f0];
};


uint enc13 [] = { // encoded addresses type 3 and 1 (A9L etc)

0xC000300c,  0x9003e,  0x80042,            // ldw   R42,[R3e]   optional 

0x80003017,  0x4080042, 0x2000014,         // jnb   B7,R43,3cda  (drop bit)
0x8000300c,  0x4080042,  0x4003a,            // ldzbw R3a,R43      (drop bit)
0x000b0000,  0x303,                       // any order opcodes, mult, 11 entries, all 3 patts must match

0x80003005,   0xf,     0x4080042,         // an2b  R43,f
0x80003005,   0x30070,  0x4003a,            // an2b  R3a,70  (0xfe is a t1, 0x70 is a t3)
0x80003003,   0x3,      0x4003a,            // shrw  R3a,3

0x80004006,   0x4003b,  0x500f0, 0x80042,    // ad2w  R42,[R3a+f0]
0xc000300d,   0x9003e,  0x80042               // stw   R42,[R3e]    optional

};

//0x9033000c,   0xf003a, 0x384003c,      // ldb rxx, [R3a++] + rpt, [3] = num pars save [4] on


uint fp5z [] = {      // par getter 22ca

0x8000400c, 0x20,     0x4,      0x80042,             // ldw R42,[R20+4]
0x9003300c, 0x80042, 0x8040344,            // ldb rxx, [R42++] + rpt, [3]num pars, [4] location
0x8000400d, 0x20,     0x4,      0x80042              // stw R42,[R20+4] 

};
/* 22ca  getpars and encode - why doesn't encode work ?
  Sub216: * 
 * 
 * 
 * 
 * 86f38: 37,45,11             jnb   B7,R45,86f4c     if (B7_R45 = 0) return;            SVA 3[7 7]1, 2[0 45]1, 1[3ff0 86f4c]2, 
86f3b: ac,45,40             ldzbw R40,R45          R40 = (uns)R45;                    SVA *2[0 40]1, 1[0 45]1, 
86f3e: 71,0f,45             an2b  R45,f            R45 &= f;                          SVA *2[0 45]1, 1[f f]1, 
86f41: 18,03,40             shrb  R40,3            R40 /= 8;                          SVA *2[0 40]1, 1[8 3]2, 
86f44: 71,fe,40             an2b  R40,fe           R40 &= fe;                         SVA *2[0 40]1, 1[fe fe]1, 
86f47: 67,41,e0,00,44       ad2w  R44,[R40+e0]     R44 += [R40+e0]; }                 SVA *2[f700 44]2, 1[f700 40]2, 0[e0 100e0 T0]2, 
86f4c: f0                   ret                    return;

 * AD read loop xdt2 and others
 82e46: a1,ee,5b,34          ldw   R34,5bee         R34 = 5bee;                        SVA *2[5bee 34]2, 1[5bee 15bee]2, 
82e4a: fb                   ei                     enable ints;
82e4b: 56,34,00,04          ad3b  R4,R0,[R34]      AD_Cmd = 0 + [R34];                SVA *3[AD_Cmd ee 4]1, 1[ee 34]1, 0[3 15bee T0]0, 
82e4f: de,21                jlt   82e72            if (AD_Cmd < 0) goto 82e72;        SVA 1[3407 82e72]2, 
82e51: 33,0a,fd             jnb   B3,Ra,82e51      if (AD_Ready = 0) goto 82e51;      SVA 3[AD_Ready 3 3]1, 2[1a a]1, 1[a33 82e51]2, 
82e54: fa                   di                     disable ints;
82e55: 51,0f,04,38          an3b  R38,R4,f         R38 = AD_Low & f;                  SVA *3[e 38]1, 2[AD_Low ee 4]1, 1[f f]1, 
82e59: 9a,34,38             cmpb  R38,[R34]                                           SVA 2[e 38]1, 1[ee 34]1, 0[3 15bee T0]0, 
82e5c: d7,ec                jne   82e4a            if (R38 != [R34]) goto 82e4a;      SVA 1[56fb 82e4a]2, 
82e5e: 51,f0,04,38          an3b  R38,R4,f0        R38 = AD_Low & f0;                 SVA *3[e0 38]1, 2[AD_Low ee 4]1, 1[f0 f0]1, 
82e62: b0,05,39             ldb   R39,R5           R39 = AD_High;                     SVA *2[0 39]1, 1[AD_High 0 5]1, 
82e65: 07,34                incw  R34              R34++;                             SVA *1[5bee 34]2, 
82e67: b2,35,36             ldb   R36,[R34++]      R36 = [R34++];                     SVA *2[3 36]1, 1[ee 34]1, 0[3 15bee T0]0, 
82e6a: b2,35,37             ldb   R37,[R34++]      R37 = [R34++];                     SVA *2[3 37]1, 1[ee 34]1, 0[3 15bee T0]0, 
82e6d: c2,36,38             stw   R38,[R36]        [R36] = R38;                       SVA 2[e0 38]2, *1[e0 36]2, 0[84ff 10303 T0]0, 
82e70: 27,d8                sjmp  82e4a            goto 82e4a;                        SVA 1[56fb 82e4a]2, * 
 * 
 * 
 * 
 
*/



uint testx [] = {          // this works for CARD....
0x9001200f,  0xf0036,                       // popw R36             with repeat (= level 1 into [1])
0x9003300c,  0xf0036,  0x8040332,           // ldb R32, [R36++]     with repeat (l2 params into [3])

// encoded addresses type 4 and 2

0x8000300c, 0x50033,  0xa0034,             // ldzbw R34,R33;

0x80003005,  0xe001f, 0x50033,              // an2b  R37,0x1f (t4) or 0xf (t2)
0x80003003,     0x04,   0xa0034,                 // shrw  R3a,4
0xC0003005,     0x0e,   0xa003a,                 // an2b  R3a,e     not in type 2
0x80004006,  0xa0034,   0xb00f0,  0x40032,          // R36 += [R3a+f0]; 

0x9003300c,  0xf0036,  0x8040332,           // ldb R32, [R36++]     continue sequence  OK to here...
0x9000200e,  0xf0036                        // push R36
};          // temp stop here



// this uses a 'cheat' to store end of params, via jump (in [0] and use INC (0x10)

uint gfm [] = {            // grandfather call modifier
0x9001200f,   0xd001e,     // popw  R1e           back up stack by one (or more) calls
0xa00a2011,  0x2000017,    // [s]call <addr>       call subr and save addr in zero
  0x31004,                 // pars                 1 bytes to copy
0x900e200e,   0xd001e      // push R1e             push modded return addr
};



/* multiple FIXED args param extract from stack (multiple levels)
 * used as prefix too (before push) for VPAR
 * includes check for consecutive ldb (for words)
 * at least one push must happen

   [1] is level (pops)
   [3] is num of pars
   [4] onwards is param list (incremented)

 */

  // doesn't work in HHX0 !!!
  // this correct for new setup 1 = level 3 = num pars 4 = list


uint fpar [] = {
0x9001200f,  0x20f003a,                 // popw Rxx         + rpt, different regs, [1] = level
0x9003300c,    0xf003a, 0x804033c,      // ldb rxx, [R3a++] + rpt, [3] = num pars save [4] on

0x00090000,   0xd0301,                  // 3 patts, 1 must match, save match in [d])
0x8000200e,   0xf003a,                  // pushw 
0x880b3017, 0x20e0042, 0x2000014,       // jnb
0x80002010, 0x2000003,                  // sjmp

0xd00c300c,  0xf003a, 0x8090c3c         // ldb rxx, [R3a++] + rpt,opt [c] = num pars save [9] on

//0xc020,  0xe,  0xf,  0x3a,                // optional pushw was 0x20 = any ?  ORIG
//0xc020,  0x10, 0x20, 0x03                 // optional sjmp
} ;

// alternate with extra stuff before the pushw

uint fpar2 [] = {
0x9001200f,  0x20f003a,                 // popw Rxx         + rpt, different regs, [1] = level
0x9003300c,    0xf003a, 0x804033c,      // ldb rxx, [R3a++] + rpt, [3] = num pars save [4] on

0x40006000,                                 // skip up to 6 opcodes

//0x00090000,   0xd0301,                  // 3 patts, 1 must match, save match in [d])
0x8000200e,   0xf003a //,                  // pushw 
//0x880b3017, 0x20e0042, 0x2000014,       // jnb
//0x80002010, 0x2000003,                  // sjmp

//0xd00c300c,  0xf003a, 0x8090c3c         // ldb rxx, [R3a++] + rpt,opt [c] = num pars save [9] on

//0xc020,  0xe,  0xf,  0x3a,                // optional pushw was 0x20 = any ?  ORIG
//0xc020,  0x10, 0x20, 0x03                 // optional sjmp
} ;









/* multibank fixed args 8065 - this is a GRANDCALLER extract....  8 82fd: XDT2  OK
   [1] is level (pops)
   [3] is num of pars
   [4] onwards is param list (incremented)   */

uint fpar5 [] = {

0x8000400c,  0x20,  0x2       , 0xd0036,        // ldw   R36,[R20+2] this is PSW
0x8000400c,  0x20,  0xf0004   , 0x9003a,        // ldw   R3a,[R20+x] 4,6,8 for level, temp into [15]
0x80003003,  0x2 ,  0x40d0036,                 // shrb  R37,2

0x8400300c,  0x40d0036, 0x11,                      // ldb   R11,R37  or STW -> set callers bank WORKS !!

//0xc030,   0xc,  0x4e, 0x36, 0,    0x11,                    // ldb   R11,R37   callers bank
//0xc030,   0xd,  0,    0x11, 0x4e, 0x36,                 // OR  stb R37, 11   *TEMP* fixup 10


0x9003300c,  0x9003a , 0x8040336,                // ldb   R36,[R3a++]  rpt cnt [3], start [4] list [16]
0x8000300c,  0x11,     0x11,                // ldb   R11,11  bank_sel register
0x8000400d,  0x20,     0x4   , 0x9003a        // stw   R3a,[R20+4]
};





/* VPAR - OK, this seems to work OK for now.... checked at start and at second pop 1 or 2 levels */
/*
  [15] first pop level                  = lev 2
  [14] first num pars (bytes)
  [1] second pop level              = lev 1[1-3] to match fpar signature
  [2] second num pars               
  [3] param list (incremented)
  [9] destination of pars
** may need an extra flag here to get the value stored in R16......
 * [1] is level 1 (pops)
 * [2] is level 2 (pops)
 * [3]  is num of l1 pars
 * [4]  is num of l2 pars
 * [5] onwards is param register list (incremented)
*/

// NB. vpar2 skips front 5 for a single level vpar....

uint vpar [] = {

0x9002200f,  0xf003c,                       // popw R3c             with repeat (= level 2 into [2]) 0xd?
0x9004300c,  0xf003c,  0x805043a,            // ldb R3a, [R3c++]     with repeat (l2 params into [4])

0x9001200f,  0xe0042,                       // popw  R42 with repeat (level 1 + l2 into [1])
0x40002000,                                 // skip up to 2 opcodes

0x9003300c,  0xe0042,  0x8003b,              // ldb   R3b,[R42++] with (l1 params into 3)
0xd000300d,  0x90016,  0x8003b,              // and optional stb

0x80003012,  0xd003a, 0x20000f7,           // djnz  R3a, <must return to loop> may be extern reg
0x9000200e,  0xe0042                        // push R42
};


// process func for sigs

void fnplu (CSIG *, SXT *, SBK *);
void tbplu (CSIG *, SXT *, SBK *);
void fparp (CSIG *, SXT *, SBK *);
void encdx (CSIG *, SXT *, SBK *);
void vparp (CSIG *, SXT *, SBK *);
void grnfp (CSIG *, SXT *, SBK *);
void sdumx (CSIG *, SXT *, SBK *);       // dummy
void feparp (CSIG *, SXT *, SBK *);

// preprocess func for sigs

void rbasp  (SIG *, SBK*);  // reg base
void pfparp (SIG *, SBK*);
void pvparp (SIG *, SBK*);
void pvpar5 (SIG *, SBK*);
void pencdx (SIG *, SBK*);
void ptimep (SIG *, SBK*);
void pfp52  (SIG *, SBK*);
void pfp5z  (SIG *, SBK*);
void vct1   (SIG *, SBK*);
void plvdf  (SIG *, SBK*);   // default levels copier
void nldf   (SIG *, SBK*);   // no levels default

// *****  PATS are sign,name,size, l65, skipf, lv1,lv2, preproc, sigproc.

// initial search sigs

// these two specifically called
PAT hdr =    {initial, "INIT",   NC(initial), 16,  nldf, sdumx };         // initial jump
PAT intign = {ignint,  "IGNINT", NC(ignint) , 16,  nldf, sdumx };         // ignore interrupt

// prescan sigs

PAT rbase =  {rbse,    "RBASE",  NC(rbse),   16,  rbasp,  sdumx };    // rbase lookup
PAT *prepats[] = {&rbase};                                          // list of prescan sigs to look for                  

// prefix sigs for signed and scaling etc.


PAT pfn1 = {sbpre1,  "sbp1" , NC(sbpre1),  16, nldf,   sdumx };
PAT pfn2 = {sbpre2,  "sbp2" , NC(sbpre2),  16, nldf,   sdumx };

PAT ptb1 = {subtb2,  "ztbp" , NC(subtb2),  16, nldf,   sdumx };
PAT tint = {intrp,   "iptab", NC(intrp),   16, nldf,   sdumx };    // 22 table interpolate (for signed/unsigned checks)

PAT *apats[] = {&pfn1, &pfn2, &ptb1, &tint};

// scanned with code scans

PAT avcl = {avct,   "AVCT",  NC(avct),   16, nldf,  sdumx  };    // altstack vector list
PAT timr = {times,  "TIMER", NC(times),  16, ptimep, sdumx  };    // timer list
PAT fnlp = {fnlu,   "FnLU",  NC(fnlu),   16, nldf,  fnplu  };    // func lookup - byte or word
PAT tblp = {tbl2,   "TBL2",  NC(tbl2),   16, nldf,  tbplu  };    // table lookup, later multibank
PAT tblx = {tblu,   "TBLU",  NC(tblu),   16, nldf,  tbplu  };    // table lookup, early
PAT fp5x = {fpar5,  "FP52" , NC(fpar5),  16, pfp52 , fparp  };    // 8 - 8065 fixed args (via stack) 
PAT fp5b = {fp5z,   "FP5Z" , NC(fp5z),   16, pfp5z , fparp  };    // 22ca fixed par get 
PAT ec13 = {enc13,  "1ENC3", NC(enc13),  16, pencdx, encdx  };    // encoded address types 3 and 1
PAT ec24 = {enc24,  "2ENC4", NC(enc24),  16, pencdx, encdx  };    // encoded address type 4 and 2,
PAT gfx  = {gfm ,   "GRNF",  NC(gfm),    16, plvdf,  grnfp  };    // grandfather wrapper (extra pops)
PAT vp1  = {vpar,   "VParM", NC(vpar),   16, pvparp, vparp  };    // variable args
PAT vp2  = {vpar+5, "VPar1", NC(vpar)-5, 16, pvparp, vparp  };    // variable pars, 1 level (num pars set outside)

PAT fp1  = {fpar,   "FPar1", NC(fpar),   16, pfparp, fparp  };    // 8061 fixed args (via POP) variable level
PAT fp2  = {fpar2,  "FPar2", NC(fpar2),  16, pfparp, fparp  };    // 8061 fixed args (via POP) variable level
PAT vp5  = {vpar5,  "VPar5", NC(vpar5),  16, pvpar5, vparp  };    // 8065 varargs 2 levels (= vpar)
PAT xp5  = {xpar5,  "XPar5", NC(xpar5),  16, plvdf,  sdumx  };    // 8065 fixed arg EXIT - useful checker
PAT sp5  = {spar5,  "sp51",  NC(spar5),  16, pfp52,  fparp  };    // 8065 fixed args 1 levs (multibank)
PAT vc1  = {vect1,  "vect1", NC(vect1),  16, vct1,   sdumx  };
PAT vc2  = {vect2,  "vect2", NC(vect2),  16, vct1,   sdumx  };
PAT vc3  = {vect3,  "vect3", NC(vect3),  16, vct1,   sdumx  };         // TEST !!
PAT tst  = {testx,  "testx", NC(testx),  16, pfparp, feparp  };   

PAT *cpatts[] = {&avcl, &timr, &fnlp, &tblp, &tblx,&fp5x, &fp5b, &ec13, &ec24, &gfx, &vp1, &vp2, &fp1, &fp2,
&vp5, &sp5, &vc1, &vc2, &vc3, &tst  };


//************************************************************************

// these are the 'fast' byte and word lookups which don't do a remap like main get_byte funcs

int s_byte (int addr)
{                    // addr is  encoded bank | addr
  return (int) sbf[(addr & 0xffff)];
}


int s_word (int addr)                       //  unsigned word
{                                           // addr is  encoded bank | addr
  int val;

  addr &= 0xffff;
  val =  sbf[addr+1];
  val =  val << 8;
  val |= sbf[addr];
  return val;
 }



int do_datsig(int foff, uint *sx, int psize, int dsize)
{
 /* data part of opcode signature
 * (ai) autoinc or int index is ALWAYS for first register....
 *  match the DATA after an opcode, return true or false
 * foff    file offset
 * sx      start point (data or operands) for pattern
 * psize   size of pattern
 * dsize   datasize and flags
 *******************************************/

int ans;
int ix, sval,tval,pval, xval, max;          // for register vals
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
max = maxadd(foff);


 if (dsize & 0x8000) 
  { //12 and 13 swop...do ops in reverse order
    ix = psize -1;
  }  


while (ans && ix < psize && ix >= 0)          // extra check for both ways...
   {
    if (foff >= max) return 0;        // hit end of file

    sg = sx[ix];

    if (sg & OPCOD) return 0;              // ->ix this is an opcode flag, stop

    sval = s_byte(foff);                       // next binfile value

    pval = sg & 0xff;                            // pattern - mod for autoinc fiddles
    tval = (sg >>16) & 0x1f;                     // array index value

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
        xval = s_byte(foff+1);
        foff++;
        sval |= (xval << 8);
        if (sval > PCORG)          // only for non-regs
         { if (tempbank >= 0) sval |= tempbank; else sval |= datbank;}    //correct address for inx
       }

     if (sg & SDINC)
       {                                             // increment value by repeat count for saved reg
         xval = tval;
         tval += sgv[(sg >> 8) & 0x1f];              // where repeat count is stored
         if (tval > NSGV) tval = xval;               // safety check to increment tval
       }

     if (tval && ans && (!sgv[tval] || (sg & SDANY))) sgv[tval] = sval;  // save register to index

    if ((sg & SDANY) == 0)   // not ignore value
      {
       if (tval)
          {
           if (sval != sgv[tval]) ans = 0;
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





int do_jumpsig ( OPC *opl, int foff)
  {            // calc jump address
   int joff;
   joff = 0;               // jump offset

   if (opl->opt & P)
     {                      // pushw
      joff = s_word(foff + 1);                    // direct address
      joff |= (foff & 0xf0000);                   // add bank from offset
     }

   if (opl->opt & J)
     {
      if (opl->opt & (T|D)) joff = negval(s_byte(foff + 2), 0x80) + 3;   // JB or JNB or DJNZ
      else if (opl->opt & K) joff = 2;               // skip - treat as  jump + 2
      else if (opl->opt & B)   // short
         joff = negval(((s_byte(foff) & 7) << 8) + s_byte(foff + 1), 0x400) + 2;
      else                      // long
         joff = negval (s_word(foff+1), 0x8000) + 3;

      joff+=foff;                                    // gets bank from foffset
      if (joff < 0 || joff >= maxadd(foff)) joff = 0;
      if (opl->opt & F) joff = 0;                    // Don't keep if a ret
     }
     
   //joff &= 0xffff;                             /// clear bank  
   if (tempbank >= 0) { joff &= 0xffff; joff |= tempbank;}
   return joff;
  }

int do_optopc(int *foff, int maxadd)
  {
    // skip any 'optional' opcodes.  max of 16 allowed.
    // ignore if f2-f7, fa-ff, NOT 0x10 (+1)

    int ans, opc;

    ans = 0;

    while(1)                     //ans < 16 && foff <= max)
      {
       opc = s_byte(*foff);                        // code file value

       if (opc < 0xf2)  break;          // && opc != 0x10) break;
       if (opc == 0xf8) break;
       if (opc == 0xf9) break;

       (*foff)++;
       ans++;
       if (ans >= 15 ) break;
       if (*foff > maxadd) break;
      }
    return ans;                                   // number of skips
   }

int opsize(const OPC *ope, char opc, int foff)
   {
    int x, gdsize;

    // change answer to include autoinc, op subtype, word
    // size  (&f), nops (&f0), inc (0x1000),  imd word (0x2000), inx word (0x4000)
    // swop operands (stx match to ldx) (0x8000)
    // nops is true number, so +1 for indexes

    x = ope->nops;

    gdsize = x+1;          // basic size - num operands (2 bits) plus 1 opcode

    if (ope->opt & M)
      {

       switch (opc & 3)
       {
        case 1:
          if (casz[ope->rsz1] > 1)
            {
             gdsize++;
             gdsize |= 0x2000;                                 // imd word
            }
          break;

        case 2:
          if (s_byte(foff+1) & 1) gdsize |= 0x1000;            // autoinc (set drop lowest bit)
          break;

        case 3:
          if (s_byte(foff+1) & 1)
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

   if (ope->opt & J)
     {                  // some jump sizes to sort out.....
      if (ope->opt & L) gdsize ++;
     }
  gdsize |= (x << 4);            // keep true numops for matching

  return gdsize;
}


int do_sgentry(uint **sz, int foff)
 {                 // do ONE sign entry (code+data)

  int ans, skp, rpt, max;
  int opc, inx, tval, xval, sval;
  int psize, gdsize;
  OPC *opl;
  uint *sx, sg;

  sg = **sz;
  sx = *sz;

  psize = (sg >> 12) & 0xf;             // size of this pattern (inc data)  ADDED OUTSIDE SUBR
  ans  = 1;                             // 1 is match
  rpt  = 0;
  max = maxadd(foff);

  if (foff >= max)  return 0;           // foff+psize ?

  if (!(sg & OPCOD))
       {
        // CAN have just data (subr pars for example)
        ans = do_datsig(foff, sx, psize, 0);  // data match, no opcode size
        foff += psize;                        // psize, gdsize is not reliable here !
        *sz += psize;                         // and move pointer
        if (ans) return foff;             //continue;
        return -foff;                     // no match.
       }


   // opcode and operands

 while (ans)       //  need loop for repeat and optional patterns...
   {
    skp = do_optopc(&foff, max);

    if (foff >= max)  return 0;         // foff+dsize ?
    if (skp > 15)     return 0;          // too many skips (0xff)

    opc = s_byte(foff);          // code file value - skip prefixes in sigs

     // codbank and datbank OK as default  
     // but NOT for any subsequent opcodes, so check here

    if (opc == 0x10)
      {  // bank change
        tempbank = s_byte(foff+1);
        if (bkmap[tempbank].bok) tempbank <<= 16; else tempbank = -1;
        foff+=2;
        opc = s_byte(foff); 
       }
    else tempbank = -1;
    
    inx = opcind[opc];
    opl = opctbl+inx;               // opcode table entry

    if ((opl->opt & H) && !P8065) inx = 0;       // check 8065
    if (!inx) return 0;             // invalid opcode, match fails

    xval = opl->sigix;              // sval now sig group value but this only a char....128 max
    sval = sg & 0xff;

    gdsize = opsize(opl, opc, foff);

    if (sg & OPSWP)
      {  
        if ((xval + sval) == 25) gdsize |= 0x8000;               // mark Stw or ldw
 //       if ((sval + sg->val) == 42) gdsize |= 0x10000;         // for jb vs JC etc
      } 
    else
    if (xval != sval) ans = 0;   // no opcode match (by sigix)
    

    tval = (sg >> 16) & 0x1f;                                  // array index

    if ((sg & OPSPV) && (opl->opt & M) && tval != (opc & 3)) ans = 0;           // check opcode subtype

    if ((sg & OPJMP)) sgv[tval] = do_jumpsig (opl,foff);      // jump or call - save dest, and can be [0] , only one !!

    if (tval && !sgv[tval]&& (sg & OPSPV))
      {
       if (opl->opt & T)
        {
          sgv[tval] = (1 << (opc & 0x7));         // keep bit mask from jb or jnb
          if (inx & 1)  sgv[tval] |= 0x100;
        }
        
       if (opl->opt & Y) sgv[tval] = inx & 1;                    // stc,clc,jc,jnc - save carry state
      }

      // 26 jnb , 27 jb

    if (sg & OPOPT) memcpy(bgv, sgv, NSGV*sizeof(int));          // save params

    if (ans) ans = do_datsig(foff+1, sx+1, psize-1, gdsize-1);   // try data match if opcode matches

    if (!ans)
      {
       // opcode or data params do not match
       // this is OK  IF optional pattern
       // OR end of a repeat patt with at least one match

       if ((sg & OPOPT) || ((sg & OPRPT) && rpt))
          {
          if (sg & OPOPT) memcpy(sgv, bgv, NSGV*sizeof(int));    // restore params
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
       sgv[tval]++;                      // count of repeats
       if (sgv[tval] > 255) ans = 0;     // safety limit max repeats
      }
 }            // end of while
 
  *sz += psize;                         // and move pointer
 if (ans) return foff;  else return -foff;

 }


int do_subsig(uint **sz, int foff)
 {
   // subpattern with optional matches - max of 255 patts (ff) and 255 matches (ff)
   // updates pattern pointer (sz) Needs at least ONE match reqd I think.
   
   uint *st, *end;
   uint  pattsz, sg;
   
   int ans, score, cmask, ix, psize, counts, max, save;

   // not sure if opt trailing works right
   pattsz = (**sz >> 16) & 0xff; 
   end = *sz + pattsz;                  // size of this subpattern (first word)

   sg = *(*sz + 1);                     // next word (subloop master data)
   score = sg & 0xff;                   // count of patts which must match (once) in lower byte
   counts = (sg >> 8) & 0xff;           // number of patterns - in upper nibble
   save = (sg >>16) & 0x1f;             // index to save matching mask to
   
   max = score;                         // patterns to try
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
             memcpy(bgv, sgv, NSGV*sizeof(int));    //save pars
             ans = do_sgentry(&st,foff);           // do one (std) entry
             if (ans > 0)                          // matches
               {
                foff = ans;                    // move on in binary
                score --;                      // decr outstanding
                cmask &= ~ix;                  // clear this try flag
                if (save) sgv[save] |= ix;     // save match if reqd
               }
             else
               {
                memcpy(sgv, bgv, NSGV*sizeof(int));       //restore pars
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



 int do_skpsig(uint **sx, int foff)
 {
   int ans, counts, max, tval;
   uint *st, sg, ptsz;

   // assumes skip down to ONE defined opcode at the moment....
   // may work for DATA too ?  but not an OPT pattern at the end...

   sg = **sx;
   max =  (sg >> 12) & 0xf;                // max no of opcodes to skip.
   tval = (sg >> 16) & 0x1f;               // where no of skips get saved

   sg = *(*sx+1);                          // get the match patt after the skip word
   ptsz = ((sg >> 12) & 0xf) + 1;          // size of whole pattern inc match and start word

   counts = 0;

   while (counts < max)
     {                                            // tries left
      st = *sx + 1;                               // reset to 2nd word
      memcpy(bgv, sgv, NSGV*sizeof(int));         // save pars
      if (*st & 0x3f)        
        ans = do_sgentry(&st,foff);                 // one entry (2nd word), swop ans
      else
        ans = do_subsig(&st,foff);               // subpattern 2nd word on (opts and/or variable order)
	  
      if (ans < 0)                                // no match
        {
         foff = (-ans);                           // next opcode (skip if fail)
         memcpy(sgv, bgv, NSGV*sizeof(int));      // restore pars;
         counts++;
        }
      else
        {
         foff = ans;
         break;                                   // match
		} 
     }

   if (tval) sgv[tval] = counts;

   *sx += ptsz;                                  // add pattsize
   if (counts < max)  return foff;

 return ans;

 }



SIG* do_sig (PAT *ptn, int xofst)
{
  int ofst, ans, foff, max, skp;

  uint *sx, *end;
  uint sg;
  SIG *z, *r;                         // for signature chaining

  if (anlpass >= ANLPRT) return 0;

  ofst = xofst;
  max = maxadd(ofst);             // safety

  if (ofst < minadd(ofst)  || ofst >= max)  return 0;

  memset(sgv,0,NSGV*sizeof(int));           //  clear sigparams

  sbf = map_addr(ofst);


//if (ptn == &fp1 && ofst == 0x4126)
//{
//    WNPRT("zzzz");
//}



  // if allowed check for redundant opcodes at front

  skp =  do_optopc(&ofst, max);

  ans  = ofst;                    // non zero is match
  foff = ofst;                    // file binary offset
  sx = ptn->sig;                  // signature entry
  end = sx + ptn->size;           // end of sig

// sx now moved along by each subr called

  while (ans > 0 && sx < end)
   {
    if (foff >= max) return 0;   // fail if drop off end
    sg = *sx;
    
    if (sg & 0x3f)                      // allow up to 63 for sigix..
      ans = do_sgentry(&sx,foff);       // std pattern - do one entry
    else
     { // special patterns - sigix is ZERO
       if (sg & OPOPT)
        ans = do_skpsig(&sx,foff);         // 'skip to X' pattern
       else
        ans = do_subsig(&sx,foff);        // subpattern (opts and/or variable order)
     }
    if (ans > 0) foff = ans;                 // next opcode

    }       // end of while


  if (ans > 0 && foff > ofst)                   // must have at least ONE matched param !!
    {
      // add sign to sig chain

      z = (SIG *) chmem(&sigch);

      z->ptn = ptn;
      z->skp = skp;
      z->ofst = xofst;               // before any 'ignores'
      z->xend = foff-1;
      for (max = 0; max < NSGV; max++) z->v[max] = sgv[max];
      r = inssig(z);            // insert sig into chain
      if (r != z) return r;     // duplicate, return original
      return z;
    }

  return 0;
 }



SIG* scan_msigs (int ofst)
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


  for (j = 0; j < NC(cpatts); j++)
      {
        s = do_sig (cpatts[j], ofst);        // code list
        if (s)
        {
            WNPRTN("Add Sig %s at %x", s->ptn->name,ofst);
        } 
        if (s) break;                       // stop as soon as one found
      }
 
 return s;
}
 

void prescan_sigs (void)
{
 // scan whole binary file (exclude fill areas) for signatures first.
 // do preprocess if reqd

  int ofst;
  uint i, j;
  SIG *s;
  BANK *b;

  if (PMANL) return;
  if (!(PSIG)) return;

  WNPRT("\n -- Scan whole bin for init sigs --\n");

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
               ofst = s->xend;                               // assume sigs can't overlap - for now
               if (s->ptn->sig_prep) s->ptn->sig_prep(s,0);  // do any required pre-processing
               break;
              }
            }
       }
   }
  #ifdef XDBG
  prt_sigs(&sigch,2);
  #endif
}



















