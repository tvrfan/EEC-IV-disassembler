/*****************************************************
 *  Signature Module version 5
 ******************************************************/
#include "sign.h"          //calls shared.h


//move to malloc in MMH ?? for recurse
uchar savechild [32];      // child sub holder for subpatterns (x of y), zero if matched
uchar *savestart[32];      // pattern start pointers holder for subpatterns (x of y), zero if matched


//use offsets for separate file ???

// uchar * index [] ??  but must be done as offsets....not direct char* array


/*each full sig starts with header (sx) which is -

 sx[0]             header size = 7 minimum. (= 1 char name+terminator)
 sx[1]             pattern number (from pattern, for duplicate checks)
 sx[2]             flags
 sx[3]             handler subroutine index
 sx[4]             fixed parameter
 sx[5]             fix par index - where to store value
 sx[6]             name from here to end of header (with zero terminator)

 flags (sx[2])

 1 no overlap allowed - for patterns that start with a repeat, mainly
 2 debug flag for reporting and tracing pattern mismatch
 4 copy parent array to child pattern
 8 copy child array back to parent if match

*/

//size,ix, flags,handler,param, name

uchar rbase [] = {
 12, 0, 0, 6, 0, 0, 'r','b','a','s','e',0,    // handler 6 rbase lookup
 0x80, 0x4 , 0x20 , 0xa1,   0x13, 0x0 , 0x2 , 0x18,   0x11, 0x0,  0x1,  0xf0,                // ldw R18, f0; must be imd (1)
 0x80, 0x4 , 0x60 , 0xb3,   0x13, 0x0 , 0x5 , 0x1a ,  0x11, 0x9 , 0x3 , 0x20, 0x20,          // ldb R1a, [0+2020] must be indexed (3) force address
 0x80, 0x0 , 0x0  , 0xa2,   0x13, 0x0 , 0x6 , 0x1c,   0x11, 0x0 , 0x7 , 0x14,                // ldw R1c, [R14++]
 0x80, 0x0 , 0x0  , 0xc2,   0x13, 0x0 , 0x2 , 0x18,   0x11, 0x0 , 0x6 , 0x1c,                // ldw [R18++], R1c  (STX in A9L)
 0x80, 0x0 , 0x0  , 0xe0,   0x13, 0x0 , 0x5 , 0x1a,   0x10, 0xa , 0x0 , 0xff,  0xf7,         // djnz R1a  (offset)
 0xff
};


// Avct using stkptr2 as task pointer for background task list - handler 1

uchar avct [] = {                //bwak3n2 82477

 11, 1, 0, 1, 0 , 0,  'a', 'v','c', 'l',  0,

 0x80, 0x4 , 0x20 , 0xa1 , 0x13, 0x0 , 0x0 , 0x22,   0x11, 0xa , 0x4 , 0xaf, 0x8a,              // ldw  R22,af8a (stackptr2) must be imd
 0x82, 0x0 , 0x0  , 0x3  ,                                                                               // skip up to 3 opcodes
 0x80, 0x4 , 0x20 , 0xb1 , 0x13, 0x0 , 0x0 , 0x11,   0x11, 0x0 , 0x7 , 0x11,                            // R11 = 11  bank select (imd)
 0x82, 0x0 , 0x0  , 0xf  ,                                                                                 // skip up to 15 opcodes
 0x80, 0x0 , 0x0  , 0xc9  ,  //  0x10, 0x2, 0x0, 0x0,   0x10, 0x2, 0x0, 0x0,                                 // push (word)  ignore args...
 0x80, 0x8 , 0x20 , 0xf0,                                                                                 // reta       (FE reqd )
 0xff
};



uchar times [] = {
  12, 2, 0, 2, 1, 0,   't','i','m','e','r',0,            // handler 2 - timer list early AA, A9L

  0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x1 , 0x30,   0x13, 0x0 , 0x2 , 0x3c,    // ldb R3c, [R30++];      (AA 3e73)
  0x80, 0x0 , 0x0 , 0x99,   0x11, 0x0 , 0x0 , 0x0 ,   0x12, 0x0 , 0x2 , 0x3c,    // cmpb  R3c,0
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0x55,                              // je    3ed0

  0x81, 0x01, 0xe,  0x02,                                                        // repeat for word (1 or 2 bytes)
  0x80, 0x10, 0x0 , 0xae,   0x11, 0x0 , 0x1 , 0x30,   0x13, 0x4 , 0x7 , 0x32,    // ldzbw R32,[R30++]

  0x80, 0x0 , 0x0 , 0x30,   0x12, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x12,    // jnb   B0,R3c,3e93
  0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x1 , 0x30,   0x13, 0x0 , 0x3 , 0x3d,    // ldb   R3d,[R30++]
  0x80, 0x0 , 0x0 , 0xae,   0x11, 0x0 , 0x1 , 0x30,   0x13, 0x0 , 0x4 , 0x34,    // ldzbw R34,[R30++]
  0x80, 0x0 , 0x0 , 0x72,   0x11, 0x0 , 0x4 , 0x34,   0x13, 0x0 , 0x3 , 0x3d,    // an2b  R3d,[R34]
  0x80, 0x0 , 0x0 , 0x33,   0x12, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x4 ,    // jnb   B3,R3c,3e91
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x0 , 0x0 , 0x4 ,                              // je    3e93
  0x80, 0x0 , 0x0 , 0x27,   0x10, 0x2 , 0x0 , 0xe2,                              // sjmp  3e73
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0xf , 0xe0,                              // je    3e73
  0x80, 0x0 , 0x0 , 0x50,   0x11, 0x0 , 0x2 , 0x3c,   0x12, 0x0 , 0x5 , 0x3e,   0x13, 0x0 , 0x0 , 0x0,    // an3b  R0,R3e,R3c
  0x80, 0x0 , 0x0 , 0xdf,   0x10, 0x2 , 0x0 , 0xda,                               // je    3e73
  0x80, 0x0 , 0x0 , 0x31,   0x12, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x5 ,     // jnb   B1,R3c,3ea1
  0x80, 0x0 , 0x0 , 0xa2,   0x11, 0x0 , 0x7 , 0x32,   0x13, 0x0 , 0x6 , 0x36,     // ldw   R36,[R32]
  0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x3 ,                               // sjmp  3ea4
  0x80, 0x0 , 0x0 , 0xbe,   0x11, 0x0 , 0x7 , 0x32,   0x13, 0x0 , 0x6 , 0x36,     // ldsbw R36,[R32]
  0x80, 0x0 , 0x0 , 0x32,   0x12, 0x0 , 0x2 , 0x3c,   0x10, 0x0 , 0x0 , 0x4 ,     // jnb   B2,R3c,3eab
  0x80, 0x0 , 0x0 , 0x03 ,  0x11, 0x0 , 0x6 , 0x36,                               // negw R36
 0xff
};




 /*test



prefix - core - postfix

this is all well and good and WORKS!! ,
but might as well have another single fnlu2 as it's the same for everything ?
and one for MAF.


 //centre of func lookup - this seems to work.....but need an In and OUT match to get types accurately
 //can do IN, but not OUT. or, bring back subsigs as options

 //test as subsig here

 //func lookup prefix by flag for signed/unsigned input

// 8 copy svals out to parent

/  BWAK3N2  INput Prefix


82920: f2                 pushp                  push(PSW);
82921: 9b,36,02,38        cmpb  R38,[R36+2]
82925: d6,06              jge   8292d            if (R38 < [R36+2]) {          <<THIS
82927: 65,02,00,36        ad2w  R36,2            R36 += 2;
8292b: 27,f4              sjmp  82921            goto 82921; }

8299d: 9b,36,02,38        cmpb  R38,[R36+2]
829a1: db,06              jc    829a9            if (R38 < [R36+2]) {         <<THIS
829a3: 65,02,00,36        ad2w  R36,2            R36 += 2;
829a7: 27,f4              sjmp  8299d            goto 8299d; }


8292d: 71,df,d7           an2b  Rd7,df           B5_Rd7 = 0;

8295f: 9b,36,02,38        cmpb  R38,[R36+2]
82963: d6,06              jge   8296b            if (R38 < [R36+2]) {        <<THIS
82965: 65,02,00,36        ad2w  R36,2            R36 += 2;
82969: 27,f4              sjmp  8295f            goto 8295f; }

8296b: 71,df,d7           an2b  Rd7,df           B5_Rd7 = 0;






*****************************OUT straight after main (2930 - 293c)
8293f: db,05              jc    82946            if (R3c < 0) {         << THIS ONE ??
82941: 91,20,d7           orb   Rd7,20           B5_Rd7 = 1;
82944: 13,3c              negb  R3c              R3c = -R3c; }
82946: 7c,38,3c           ml2b  R3c,R38          wR3c = yR3c * R38;
82949: 9c,3a,3c           divb  R3c,R3a          yR3c = wR3c / R3a;
                                                 yR3d = wR3c % R3a;

8295f: 9b,36,02,38        cmpb  R38,[R36+2]
82963: d6,06              jge   8296b            if (R38 < [R36+2]) {
82965: 65,02,00,36        ad2w  R36,2            R36 += 2;
82969: 27,f4              sjmp  8295f            goto 8295f; }

8296b: 71,df,d7           an2b  Rd7,df           B5_Rd7 = 0;
8296e: b2,37,3a           ldb   R3a,[R36++]      R3a = [R36++];







uchar prefmaf [] = {
 12, 21, 8, 0, 0, 0, 'f','n','p','f','1', 0,
2df8: 65,04,00,52         ad2w  R52,4            R52 += 4;                         # jmp ahead in the MAF transfer function to the right value
2dfc: 8a,52,50            cmpw  R50,[R52]
2dff: d3,f7               jnc   2df8             if (R50 < [R52]) goto 2df8;       # loop
2e01: 69,04,00,52         sb2w  R52,4            R52 -= 4;



// no bit - jge or jc for sig/unsign      // this seems to work !!
uchar prefnbt [] = {
 12, 21, 8, 0, 0, 0, 'f','n','p','f','2', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x11, 0x0 , 0x4 , 0x32,   0x10, 0x1 , 0x2 , 0x2 ,   0x12, 0x0 , 0x8 , 0x34,          //cmpb 34, [R32+2] or cmpw 34 [R32+4] for word
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x1 , 0x0 , 0x6 ,                                                              // jmp +6   jge or jc  signed/unsigned input
 0x80, 0x0 , 0x0 , 0x65 ,  0x11, 0x1 , 0x2 , 0x2 ,   0x13, 0x0 , 0x4 , 0x32,                                    // ad2w  32, 2, or 4 next row
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x1 , 0x0 , 0xf4,                                                              // loop back f4 or f7

 0x81, 0x0 , 0x0,  0x1,                                                                                         // optional  an2b  bit clear  min = 0, ix, max = 1
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x2 , 0x0 , 0xdf,   0x12, 0x0 , 0x3 , 0xf6,                                    // clear signed input flag, used as neg flag later


 0xff
};


//bit mask for sign unsign IN - most 8061 single banks.

uchar prefbit [] = {
 12, 20, 8, 0, 0, 0, 'f','n','p','f','1', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x11, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x2 , 0x2 ,   0x12, 0x0 , 0x8 , 0x34,          //cmpb 34, [R32+2] or cmpw 34 [R32+4] for word
 0x80, 0x2 , 0xb , 0x31,   0x12, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,                                    // jnb Rx By signed/unsigned input

 0x80, 0x0 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0xa ,                                                              // jc +10   unsigned input
 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,                                                              // jmp +2   down to loop back
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x6 ,                                                              // jmp +6   jge   signed input

 0x80, 0x0 , 0x0 , 0x65 ,  0x11, 0x0 , 0x2 , 0x2 ,   0x13, 0x0 , 0x4 , 0x32,                                    // ad2w  32, 2, or 4 next row
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x0 , 0x0 , 0xed,                                                              // loop back

 0x81, 0x0 , 0x0,  0x1,                                                                                         // optional  an2b  bit clear  min = 0, ix, max = 1
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x2 , 0x0 , 0xdf,   0x12, 0x0 , 0x3 , 0xf6,                                    // clear signed input flag, used as neg flag later

 0xff
};

//bit mask for sign unsig OUT


uchar postfnlu[] = {
 12, 22, 0xc, 0, 0, 0, 'f','n','p','s','t', 0,

 0x80, 0x2 , 0xa , 0x31,   0x12, 0x0 , 0x3 , 0xf6,   0x10, 0x1 , 0x0 , 0x4 ,          // jnb   B1,Rf6,32e8 signed out flag?
 0x80, 0x0 , 0x0 , 0xdb,   0x10, 0x1 , 0x0 , 0x9 ,                                    // jc    32ef

 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x1 , 0x0 , 0x2 ,                                    // sjmp  32ea
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x1 , 0x0 , 0x5 ,                                    // jge   32ef      is result negative

 0x80, 0x0 , 0x0 , 0x91,   0x11, 0x3 , 0x0 , 0x0 ,   0x13, 0x0 , 0x3 , 0xf6,          // orb   Rf6,1
 0x80, 0x0 , 0x0 , 0x13,   0x13, 0x0 , 0x7 , 0x38,                                    // negb  R38         flip if _ve
 0x80, 0x0 , 0x0 , 0x7c,   0x11, 0x0 , 0x8 , 0x34,   0x13, 0x0 , 0x7 , 0x38,          // ml2b  R38,R34
 0x80, 0x0 , 0x0 , 0x9c,   0x11, 0x0 , 0x6 , 0x36,   0x13, 0x0 , 0x7 , 0x38,          // div   R38,R36     // word or byte
 0x80, 0x0 , 0x0 , 0x30,   0x12, 0x0 , 0x3 , 0xf6,   0x10, 0x1 , 0x0 , 0x2 ,          // jnb B0,Rf6,2fda
 0x80, 0x0 , 0x0 , 0x13,   0x13, 0x0 , 0x7 , 0x38,                                    // negb  R38            flip back if negative
 0xff
};

//postfix xx no bit mask - jc or jge

uchar postfnxx[] = {
 12, 23, 0xc, 0, 0, 0, 'f','n','p','s','t', 0,

 0x80, 0x0 , 0x0 , 0xdb,   0x10, 0x1 , 0x0 , 0x5 ,                                    // jc or jge  32ef           //sign
 0x80, 0x0 , 0x0 , 0x91,   0x11, 0x3 , 0x0 , 0x0 ,   0x13, 0x0 , 0x3 , 0xf6,          // orb   Rf6,1
 0x80, 0x0 , 0x0 , 0x13,   0x13, 0x0 , 0x7 , 0x38,                                    // negb  R38         flip if _ve
 0x80, 0x0 , 0x0 , 0x7c,   0x11, 0x0 , 0x8 , 0x34,   0x13, 0x0 , 0x7 , 0x38,          // ml2b  R38,R34
 0x80, 0x0 , 0x0 , 0x9c,   0x11, 0x0 , 0x6 , 0x36,   0x13, 0x0 , 0x7 , 0x38,          // div   R38,R36     // word or byte

 0xff
};


 post for MAF

2e14: 6c,50,54            ml2w  R54,R50          lR54 = wR54 * R50;
2e17: 8c,58,54            divw  R54,R58          wR54 = lR54 / R58;
                                                 wR56 = lR54 % R58;
2e1a: 66,53,54            ad2w  R54,[R52++]      R54 += [R52++];                   # interpolated air flow
2e1d: 69,04,00,52         sb2w  R52,4            R52 -= 4;





//change to 0x53 as 0 of 1 ??

uchar fnht [] = {
 12, 13, 0, 7, 0, 0, 'f','n','h','t','0', 0,                             // dummy handler 7 for now.

 0x53, 0x1 , 0x0 , 0x2 ,
 0x14, 0x15,                                                           // 1 of 2 must match, any order

// 0x81, 0x0 , 0x0,  0x1,                                                                                         // optional  an2b  bit clear  min = 0, ix, max = 1
// 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x2 , 0x0 , 0xdf,   0x12, 0x0 , 0x3 , 0xf6,                                    // clear signed input flag, used as neg flag later

 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // ldw   R36,[R32++]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x7 , 0x38,          // ldw   R38,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // sb2w  R36,[R32]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x8 , 0x34,          // sb2w  R34,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x7 , 0x38,          // sb2w  R38,[R32]

 0x53, 0x1 , 0x0 , 0x2 ,
 0x16, 0x17,                                                           // 1 of 2 must match, any order

 0xff
};


 //kiee has alternate version.. 19bf2

 // func lookup - byte or word handler 3


*/

//bit mask to select signed unsigned in and out. early bins


uchar fnlu [] = {
 12, 3, 0, 3, 0, 0, 'f','n','l','u','1', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x11, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x2 , 0x2 ,   0x12, 0x0 , 0x8 , 0x34,          // cmpb 34, [R32+2] or cmpw 34 [R32+4] for word
 0x80, 0x2 , 0xb , 0x31,   0x12, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,                                    // jnb Rx By signed/unsigned

 0x80, 0x0 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0xa ,                                                              // jc +10   unsigned input
 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,                                                              // jmp +2   down to loop back
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x6 ,                                                              // jmp +6   jge   signed input

 0x80, 0x0 , 0x0 , 0x65 ,  0x11, 0x0 , 0x2 , 0x2 ,   0x13, 0x0 , 0x4 , 0x32,                                    // ad2w  32, 2, or 4 next row
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x0 , 0x0 , 0xed,                                                              // loop back

 0x81, 0x0 , 0x0,  0x1,                                                                                         // optional -  min = 0, ix, max = 1
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x2 , 0x0 , 0xdf,   0x12, 0x0 , 0x3 , 0xf6,                                    // clear signed input flag, used as neg flag later (in A9L)



 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // ldw   R36,[R32++]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x7 , 0x38,          // ldw   R38,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // sb2w  R36,[R32]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x8 , 0x34,          // sb2w  R34,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x7 , 0x38,          // sb2w  R38,[R32]

 0x80, 0x2 , 0xa , 0x31,   0x12, 0x0 , 0x3 , 0xf6,   0x10, 0x0 , 0x0 , 0x4 ,          // jnb   B1,Rf6,32e8 signed out flag?
 0x80, 0x0 , 0x0 , 0xdb,   0x10, 0x0 , 0x0 , 0x9 ,                                    // jc    32ef

 0x80, 0x0 , 0x0 , 0x20,   0x10, 0x0 , 0x0 , 0x2 ,                                    // sjmp  32ea
 0x80, 0x0 , 0x0 , 0xd6,   0x10, 0x0 , 0x0 , 0x5 ,                                    // jge   32ef      is result negative

 0x80, 0x0 , 0x0 , 0x91,   0x11, 0x2 , 0x0 , 0x0 ,   0x13, 0x0 , 0x3 , 0xf6,          // orb   Rf6,1
 0x80, 0x0 , 0x0 , 0x13,   0x13, 0x0 , 0x7 , 0x38,                                    // negb  R38         flip if _ve
 0x80, 0x0 , 0x0 , 0x7c,   0x11, 0x0 , 0x8 , 0x34,   0x13, 0x0 , 0x7 , 0x38,          // ml2b  R38,R34
 0x80, 0x0 , 0x0 , 0x9c,   0x11, 0x0 , 0x6 , 0x36,   0x13, 0x0 , 0x7 , 0x38,          // div   R38,R36     // word or byte

 0xff
};


// no bit select BWAK#N2 etc.

uchar fnlu2 [] = {
 12, 13, 0, 3, 1, 1, 'f','n','l','u','2', 0,

 0x80, 0x0 , 0x0 , 0x9b,   0x11, 0x0 , 0x4 , 0x32,   0x10, 0x0 , 0x2 , 0x2 ,   0x12, 0x0 , 0x8 , 0x34,          // cmpb 34, [R32+2] or cmpw 34 [R32+4] for word
 0x80, 0x0 , 0xa , 0xd6,   0x10, 0x0 , 0x0 , 0x6 ,                                                              // jmp +6   jge or jc  signed input
 0x80, 0x0 , 0x0 , 0x65 ,  0x11, 0x0 , 0x2 , 0x2 ,   0x13, 0x0 , 0x4 , 0x32,                                    // ad2w  32, 2, or 4 next row
 0x80, 0x0 , 0x0 , 0x27,   0x10, 0x0 , 0x0 , 0xf4,                                                              // loop back

 0x81, 0x0 , 0x0,  0x1,                                                                                         // optional -  min = 0, ix, max = 1
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x2 , 0x0 , 0xdf,   0x12, 0x0 , 0x3 , 0xf6,                                    // clear signed input flag, used as neg flag later (in A9L)



 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // ldw   R36,[R32++]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x7 , 0x38,          // ldw   R38,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x6 , 0x36,          // sb2w  R36,[R32]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x33,   0x13, 0x0 , 0x8 , 0x34,          // sb2w  R34,[R32++]
 0x80, 0x0 , 0x0 , 0x7a,   0x11, 0x0 , 0x4 , 0x32,   0x13, 0x0 , 0x7 , 0x38,          // sb2w  R38,[R32]

 0x80, 0x0 , 0xb , 0xdb,   0x10, 0x0 , 0x0 , 0x5 ,                                    // jc or jge  32ef           //sign
 0x80, 0x0 , 0x0 , 0x91,   0x11, 0x2 , 0x0 , 0x0 ,   0x13, 0x0 , 0x3 , 0xf6,          // orb   Rf6,1
 0x80, 0x0 , 0x0 , 0x13,   0x13, 0x0 , 0x7 , 0x38,                                    // negb  R38         flip if _ve
 0x80, 0x0 , 0x0 , 0x7c,   0x11, 0x0 , 0x8 , 0x34,   0x13, 0x0 , 0x7 , 0x38,          // ml2b  R38,R34
 0x80, 0x0 , 0x0 , 0x9c,   0x11, 0x0 , 0x6 , 0x36,   0x13, 0x0 , 0x7 , 0x38,          // div   R38,R36     // word or byte

 0xff
};






























/* INTERPOLATE may be better to look for the signed conversion - A9L ??
3722: bc,33,3a            ldsbw R3a,R33          swR3a = yR33;                     # SIGNED interpolate calc
3725: bc,31,3c            ldsbw R3c,R31          swR3c = yR31;
3728: 68,3c,3a            sb2w  R3a,R3c          R3a -= R3c;
372b: ac,30,3c            ldzbw R3c,R30          wR3c = yR30;
372e: fe,6c,3a,3c         sml2w R3c,R3a          slR3c = swR3c * R3a;
 * unsigned does a direct multiply ml3b.....



xdt2

   Sub_83087:
83087: 11,34              clrb  R34              R34 = 0;
83089: 11,36              clrb  R36              R36 = 0;
   Sub_8308b:
8308b: 11,38              clrb  R38              R38 = 0;
8308d: 37,99,22           jnb   B7,R99,830b2     if (B7_R99 = 1)  {
83090: 08,01,38           shrw  R38,1            R38 >>= 1;
83093: a0,34,3e           ldw   R3e,R34          R3e = R34;
83096: fe,4c,36,38,34     sml3w R34,R38,R36      slR34 = swR38 * R36;
8309b: a0,3e,34           ldw   R34,R3e          R34 = R3e;
8309e: 0a,01,3e           asrw  R3e,1            swR3e >>=  1;
830a1: 64,36,3e           ad2w  R3e,R36          R3e += R36;
830a4: fe,6c,38,34        sml2w R34,R38          slR34 = swR34 * R38;
830a8: 68,36,3e           sb2w  R3e,R36          R3e -= R36;
830ab: 09,01,3e           shlw  R3e,1            R3e <<= 1;
830ae: 09,01,38           shlw  R38,1            R38 <<= 1;
830b1: f0                 ret                    return; }


*/

// this works for A9L type tblu, and xdt2 (tblu2)
// table interpolate - no specific handler - for signed/unsigned checks
//no hanlder - maybe there should be one ?
//AA doesn't have signed unsigned so no sig match.

//this seems to work too.....

uchar ttrps [] = {
 14, 14, 2, 0, 0, 0, 't','a','b','i','n','t','p',0,

 0x81, 0x0, 0x0, 0x4 ,                                                         // 0 to 4 repeats of ...
 0x80, 0x0, 0x0, 0x1 ,   0x13, 0x2 , 0x0 , 0x33,                               // clr

 0x80, 0x2 , 0xa , 0x37,   0x12, 0x0 , 0x3 , 0x2d,   0x10, 0x0 , 0x7 , 0x1a,   // jnb   B7,R2d,373c      if (Tblsflg = 1)  but opt too ?

 0x82 ,0x0, 0x0, 0x8,                                                          // skip up to any 8 opcodes to -
 0x80, 0x8, 0x2, 0x6c,   0x11, 0x2 , 0x0 , 0x3a,   0x13, 0x2 , 0x5 , 0x3c,    // (0xfe) sml2w R3c,R3a     slR3c = swR3c * R3a;  for sign-ness

 0xff };

/*maybe chop here ??  only need jb/jnb really.
 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x1,  0x33,   0x10, 0x0 , 0x2 , 0x3a,  // ldsbw R3a,R33          swR3a = yR33;
 0x80, 0x0 , 0x0 , 0xbc,   0x10, 0x0 , 0x4,  0x31,   0x10, 0x0 , 0x5 , 0x3c,  // ldsbw R3c,R31          swR3c = yR31;
 0x80, 0x0 , 0x0 , 0x68,   0x10, 0x0 , 0x5,  0x3c,   0x10, 0x0 , 0x2 , 0x3a,  // sb2w  R3a,R3c          R3a -= R3c;
 0x80, 0x0 , 0x0 , 0xac,   0x10, 0x0 , 0xc,  0x30,   0x10, 0x0 , 0x5 , 0x3c,  // ldzbw R3c,R30          wR3c = yR30;
 0x80, 0x6 , 0x0 , 0x6c,   0x10, 0x0 , 0x2 , 0x3a,   0x10, 0x0 , 0x5 , 0x3c,  // (0xfe) sml2w R3c,R3a     slR3c = swR3c * R3a;  for sign- ness
 0xff
};  */

//need 22CA one, and xdt2 one, and ....
/************** word tables ->
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

 // table lookup, byte, early      - handler 4  3277 AA

uchar tblu [] = {
 12, 4, 0, 4,0, 0,'t','b','l','u','1' ,0,

// 0x81, 0x0,  0x0,  0x1 ,                  //optional
// 0x80, 0x0 , 0x0 , 0x71,   0x10, 0x2 , 0x0 , 0xfe,   0x10, 0x2 , 0x0,  0xf8,                              // an2b  Rf8,fe

 0x80, 0x0 , 0x0 , 0x5c,   0x11, 0x0 , 0x8 , 0x33,   0x12, 0x0 , 0x3 , 0x34,   0x13, 0x0 , 0x9 , 0x36,     // ml3b  R36,R34,R33 ml3b
 0x80, 0x0 , 0x0 , 0x74,   0x11, 0x0 , 0x6 , 0x31,   0x13, 0x0 , 0x9 , 0x36,                               // ad2b  R36,R31

 0x80, 0x0,  0x0 , 0xd3,   0x10, 0x0 , 0x0 , 0x2 ,                                                         // jnc
 0x80, 0x0,  0x0 , 0x17,   0x13, 0x0 , 0x5 , 0x37,                                                         // incb  R37
 0x80, 0x0 , 0x0 , 0x64,   0x11, 0x0 , 0x9 , 0x36,   0x13, 0x0 , 0x4 , 0x38,                               // ad2w  R38,R36
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x39,   0x13, 0x0 , 0x6 , 0x31,                               // ldb   R31,[R38++]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x38,   0x13, 0x0 , 0x8 , 0x33,                               // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x28,   0x11, 0x3 , 0x1 , 0x0 ,                                                         // call to interpolate, save address
 0x80, 0x0 , 0x0 , 0x64,   0x11, 0x0 , 0x3 , 0x34,   0x13, 0x0 , 0x4 , 0x38,                               // ad2w  R38,R34
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x38,   0x13, 0x0 , 0x8 , 0x33,                               // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x05,   0x13, 0x0 , 0x4 , 0x38,                                                         // decw  R38
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x38,   0x13, 0x0 , 0x6 , 0x31,                               // ldb   R31,[R38]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x7 , 0x3a,   0x13, 0x0 , 0x3 , 0x34,                               // ldb   R34,R3b
 0x80, 0x0 , 0x0 , 0x28,   //0x10, 0x2 , 0x0 , 0xc ,                                                         // call
 0xff
};

 /* table lookup, later multibank
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

uchar tbl2 [] = {                         //handler 4  BWAK3N2 82c93
 12, 5, 0, 4, 0, 0, 't','b','l','u','2',0,

 0x80, 0x0 , 0x0 , 0x5c,   0x11, 0x0 , 0x8 , 0x37,   0x12, 0x0 , 0x3 , 0x38,   0x13, 0x0 , 0x5 , 0x3a,    // ml3b  R36,R34,R33
 0x80, 0x0 , 0x0 , 0x74,   0x11, 0x0 , 0x9 , 0x35,   0x12, 0x0 , 0x5 , 0x3a,                              // ad2b  R36,R31
 0x80, 0x0 , 0x0 , 0xb4,   0x11, 0x0 , 0x0 , 0x0 ,   0x12, 0x0 , 0x2 , 0x3b,                              // adcb  R37,R0
 0x80, 0x0 , 0x0 , 0x64,   0x11, 0x0 , 0x5 , 0x3a,   0x12, 0x0 , 0x4 , 0x3c,                              // ad2w  R38,R36
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x3c,   0x12, 0x0 , 0x9 , 0x35,                              // ldb   R31,[R38++]
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x3c,   0x12, 0x0 , 0x8 , 0x37,                              // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0x64,   0x11, 0x0 , 0x3 , 0x38,   0x12, 0x0 , 0x4 , 0x3c,                              // ad2w  R38,R34
 0x80, 0x0 , 0x0 , 0xb0,   0x11, 0x0 , 0x7 , 0x34,   0x12, 0x0 , 0x6 , 0x39,                              // ldb   R35,R30
 0x80, 0x0 , 0x0 , 0xb0,   0x11, 0x0 , 0xe , 0x36,   0x12, 0x0 , 0x2 , 0x3b,                              // ldb   R37,R32
 0x80, 0x0 , 0x0 , 0x28,   0x11, 0x2 , 0x1 , 0x1c,                                                        // call
 0x80, 0x0 , 0x0 , 0xb2,   0x11, 0x0 , 0x4 , 0x3c,   0x12, 0x0 , 0x8 , 0x37,                              // ldb   R33,[R38]
 0x80, 0x0 , 0x0 , 0xb3,   0x11, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x0 , 0xff,   0x12, 0x0 , 0x9 , 0x35,    // ldb   R31,[R38+ff]
 0x80, 0x0 , 0x0 , 0xa0,   0x11, 0x0 , 0xb , 0x3e,   0x12, 0x0 , 0x4 , 0x3c,                              // ldw   R38,R3a
 0x80, 0x0 , 0x0 , 0x28,   0x11, 0x2 , 0x1 , 0xc ,                                                        // call

 /*
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0xe , 0x36,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0x4 , 0x3c,   0x10, 0x0 , 0x7 , 0x34,
 0x80, 0x0 , 0x0 , 0xb0,   0x10, 0x0 , 0x2 , 0x3b,   0x10, 0x0 , 0x6 , 0x39,
 0x80, 0x0 , 0x0 , 0x28,   0x10, 0x2 , 0x0 , 0x1c ,
 0x80, 0x0 , 0x0 , 0xa0,   0x10, 0x0 , 0xb , 0x3e,   0x10, 0x0 , 0x4 , 0x3c,
 0x80, 0x8 , 0xa , 0x37,   0x10, 0x0 , 0xf , 0xd8,   0x10, 0x2 , 0x0 , 0x1a,      //jnb is sign marker   */
 0xff
};

/* tbl3 ??
5548: ad,07,34            ldzbw R34,7            wR34 = 7;
554b: 45,fa,00,88,38      ad3w  R38,R88,fa       R38 = 3792;

 / table lookup, later multibank
  table addr   = s->v[4]
  table cols   = s->v[3]
   * from interpolate sig
  bit register = interp->v[3]
  bit mask     = interp->v[10]
 */

uchar tbl3 [] = {                         //handler 4          (1AGB)

12, 6, 0, 4,0, 0, 't','b','l','u','3',0,


0x80, 0x00, 0x00, 0x5c,   0x11, 0x00, 0x08, 0x33,    0x12, 0x00, 0x03, 0x34,    0x13, 0x00, 0x05, 0x36,   // ml3b  R36,R34,R33
0x80, 0x00, 0x00, 0x74,   0x11, 0x00, 0x06, 0x31,    0x12, 0x00, 0x05, 0x36,                              // ad2b  R36,R31
0x80, 0x00, 0x00, 0xb4,   0x11, 0x02, 0x00, 0x00,    0x12, 0x00, 0x07, 0x37,                              // adcb  R37,R0
0x80, 0x00, 0x00, 0x64,   0x11, 0x00, 0x05, 0x36,    0x12, 0x00, 0x04, 0x38,                              // ad2w  R38,R36
0x80, 0x00, 0x00, 0xb2,   0x11, 0x00, 0x04, 0x38,    0x12, 0x00, 0x06, 0x31,                              // ldb   R31,[R38++]
0x80, 0x00, 0x00, 0xb2,   0x11, 0x00, 0x04, 0x38,    0x12, 0x00, 0x08, 0x33,                              // ldb   R33,[R38]
0x80, 0x00, 0x00, 0x28,   0x11, 0x02, 0x01, 0x39,                                                         // scall 2181
0x80, 0x00, 0x00, 0x64,   0x11, 0x00, 0x03, 0x34,    0x12, 0x00, 0x04, 0x38,                              // ad2w  R38,R34
0x80, 0x00, 0x00, 0xb2,   0x11, 0x00, 0x04, 0x38,    0x12, 0x00, 0x08, 0x33,                              // ldb   R33,[R38]
0x80, 0x00, 0x00, 0x05,   0x11, 0x00, 0x04, 0x38,                                                         // decw  R38
0x80, 0x00, 0x00, 0xb2,   0x11, 0x00, 0x04, 0x38,    0x12, 0x00, 0x06, 0x31,                              // ldb   R31,[R38]
0x80, 0x00, 0x00, 0xa0,   0x11, 0x00, 0x0a, 0x3a,    0x12, 0x00, 0x03, 0x34,                              // ldw   R34,R3a
0x80, 0x00, 0x00, 0x28,   0x11, 0x02, 0x00, 0x29,                                                         // scall 2181
0xff
};


// setbank in all multibanks, to identify a [R20+x] as a popp... but not always adjacent to the pop/ldw

uchar setbank[] = {
13, 10, 0, 0, 0,0, 's', 'e', 't', 'b', 'n','k', 0,                              //calls setbnk (= 1)
// 0x80, 0x4 , 0x40 , 0xa3,   0x11, 0x0 , 0x0 , 0x20,   0x10, 0x2 , 0x0 , 0x2,   0x13, 0x16 , 0x1 , 0x36,    // ldw   R36,[R20+ff]

//a few skips here ? or just the shift an ldx
0x80, 0x00, 0x00, 0x18,   0x11, 0x00, 0x00, 0x02,    0x13, 0x10, 0x01, 0x37,                              // shrb  R37,2 (saved as R36)
0x80, 0x00, 0x00, 0xb0,   0x11, 0x10, 0x01, 0x37,    0x13, 0x00, 0x00, 0x11,                              // ldb   R11,R37

0xff
};

uchar fixarg[] = {
13, 11, 0, 0, 0, 0,'f', 'i', 'x', 'a', 'r','g', 0,

0x81, 0x1 , 0xf,  0x4,                                                           //       repeat between 1 and 4 times - pop level
0x80, 0x00, 0x00, 0xcc,   0x13, 0x06, 0x2, 0x38,                                // pop   Rx location and match cnt in index[f].

//0x82 ,0x8, 0x0, 0x8,          //skip up to 8 opcodes ??


0x81, 0x1 , 0xe,  0x8,                                                           //       repeat between 1 and 8 times
0x80, 0x00, 0x00, 0xb2,   0x11, 0x22, 0x2, 0x3a,    0x13, 0x06, 0x10, 0x3c,       // ldb   R3c,[Rx++] list in [16] onwards

//0x80, 0x00, 0x00, 0xc8,   0x10, 0x02, 0x00, 0x3a,                                // push  Rx

0xff
};

/*   xdt2
82f93: a3,20,02,36        ldw   R36,[R20+2]      R36 = [StackPtr+2];
82f97: a3,20,04,3a        ldw   R3a,[R20+4]      R3a = [StackPtr+4];
82f9b: f2                 pushp                  push(PSW);
82f9c: fa                 di                     interrupts OFF;
82f9d: 18,02,37           shrb  R37,2            R37 >>= 2;
82fa0: b0,37,11           ldb   R11,R37          BANK_Select = R37;
82fa3: b2,3b,36           ldb   R36,[R3a++]      R36 = [R3a++];
82fa6: b2,3b,37           ldb   R37,[R3a++]      R37 = [R3a++];
82fa9: b2,3b,38           ldb   R38,[R3a++]      R38 = [R3a++];

*/
uchar fixargm[] = {
13, 12, 0, 0, 0, 0, 'f', 'x', 'a', 'r', 'g','m', 0,
0x81, 0x1 , 0xf,  0x4,                                                           //       repeat between 1 and 4 times - pop level
0x80, 0x4 , 0x40 , 0xa3,   0x11, 0x0 , 0x0 , 0x20,   0x10, 0x2 , 0x0 , 0x2,   0x13, 0x6 , 0x2 , 0x35,    // ldw   R31,[R20+x]

0x82 ,0x1, 0x0, 0x8,          // skip up to 8 opcodes - don't include match pattern
0x80, 0x00, 0x00, 0xb2,   0x11, 0x22, 0x2, 0x3a,    0x13, 0x02, 0x0, 0x3c,

0x81, 0x1 , 0xe,  0x8,                                                           //       repeat between 1 and 8 times
0x80, 0x00, 0x00, 0xb2,   0x11, 0x22, 0x2, 0x3a,    0x13, 0x6, 0x10, 0x3c,       // ldb   R3c,[Rx++] list in [16] onwards

//0x80, 0x00, 0x00, 0xc8,   0x10, 0x02, 0x00, 0x3a,                                // push  Rx

0xff
};



/*maybe TFR ?? for args (others)

8a548: b2,47,3f           ldb   R3f,[R46++]      R3f = [R46++];
8a54b: c6,27,3f           stb   R3f,[R26++]      [R26++] = R3f;

24 bit add ?
24 bit ldx
24 bit subtract

16f07: 67,d4,5c,48        ad2w  R48,[Rd4+5c]     R48 += [Rd4+5c];
16f0b: b7,d4,5e,4a        adcb  R4a,[Rd4+5e]     R4a += [Rd4+5e] + CY; }
16f0f: 71,7f,aa           an2b  Raa,7f           B7_Raa = 0;
16f12: c3,d4,5c,48        stw   R48,[Rd4+5c]     [Rd4+5c] = R48;
16f16: c7,d4,5e,4a        stb   R4a,[Rd4+5e]     [Rd4+5e] = R4a; }

32 bit add ?
32 bit ldx
32 bit subtract.

17701: 67,d4,e0,48        ad2w  R48,[Rd4+e0]     R48 += [Rd4-20];
17705: a7,d4,e2,4a        adcw  R4a,[Rd4+e2]     R4a += [Rd4-1e] + CY;

subtracts as well ?
*/





/* word table lookup
  table addr   = s->v[4]
  table cols   = s->v[3]
  bit register = interp->v[3]
  bit mask     = interp->v[10]
*/

// R38 [3] is no of word cols  R3c [4] is tab address
// [1] is jump address
//par1 set for word interpolate           RZASAM5 92e4a

uchar wtbl [] = {
 12, 9, 0, 4, 1, 0, 'w', 't','b','l','u',0,                                    // 4  table lookup, par 1 for word sized

0x80, 0x00, 0x00, 0x5c,   0x11, 0x00, 0x0b, 0x37,    0x12, 0x00, 0x03, 0x38,    0x13, 0x00, 0x02, 0x3e,   // ml3b  R3e,R38,R37
0x80, 0x00, 0x00, 0xb0,   0x11, 0x00, 0x0a, 0x34,    0x12, 0x00, 0x05, 0x3a,                              // ldb   R3a,R34
0x80, 0x00, 0x00, 0xb0,   0x11, 0x00, 0x06, 0x36,    0x12, 0x00, 0x07, 0x3b,                              // ldb   R3b,R36
0x80, 0x00, 0x00, 0xac,   0x11, 0x10, 0x0a, 0x35,    0x12, 0x00, 0x0a, 0x34,                              // ldzbw R34,R35
0x80, 0x00, 0x00, 0x64,   0x11, 0x00, 0x0a, 0x34,    0x12, 0x00, 0x02, 0x3e,                              // ad2w  R3e,R34
0x80, 0x00, 0x00, 0x09,   0x11, 0x00, 0x09, 0x01,    0x12, 0x00, 0x02, 0x3e,                              // shlw  R3e,1
0x80, 0x00, 0x00, 0x64,   0x11, 0x00, 0x02, 0x3e,    0x12, 0x00, 0x08, 0x3c,                              // ad2w  R3c,R3e
0x80, 0x00, 0x00, 0xa2,   0x11, 0x00, 0x08, 0x3c,    0x12, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c++]
0x80, 0x00, 0x00, 0xa2,   0x11, 0x00, 0x08, 0x3c,    0x12, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x28,   0x11, 0x02, 0x01, 0x25,                                                         // scall 92e8e
0x80, 0x00, 0x00, 0x09,   0x11, 0x00, 0x09, 0x01,    0x12, 0x00, 0x03, 0x38,                              // shlw  R38,1
0x80, 0x00, 0x00, 0x64,   0x11, 0x00, 0x03, 0x38,    0x12, 0x00, 0x08, 0x3c,                              // ad2w  R3c,R38
0x80, 0x00, 0x00, 0xa2,   0x11, 0x00, 0x04, 0x3c,    0x12, 0x00, 0x06, 0x36,                              // ldw   R36,[R3c]
0x80, 0x00, 0x00, 0x05,   0x11, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0x05,   0x11, 0x00, 0x04, 0x3c,                                                         // decw  R3c
0x80, 0x00, 0x00, 0xa2,   0x11, 0x00, 0x04, 0x3c,    0x12, 0x00, 0x0a, 0x34,                              // ldw   R34,[R3c]
0x80, 0x00, 0x00, 0xa0,   0x11, 0x00, 0x02, 0x3e,    0x12, 0x00, 0x03, 0x38,                              // ldw   R38,R3e
0x80, 0x00, 0x00, 0x28,   0x11, 0x02, 0x00, 0x10,                                                         // scall 92e8e

0xff

};

//interpolate for word table. B7 R96 is sign bit.
// bit register = interp->v[3]
//  bit mask     = interp->v[10]            RZASAM5  92e8e

uchar wintp[] = {

11, 15, 2, 0, 0, 0, 'w', 't', 'b', 'p', 0,

0x80, 0x00, 0x00, 0xac,   0x11, 0x00, 0x01, 0x3a,    0x12, 0x00, 0x02, 0x3e,                              // ldzbw R3e,R3a
0x80, 0x02, 0x0a, 0x37,   0x12, 0x00, 0x03, 0x96,    0x10, 0x00, 0x00, 0x01,                              // jnb   B7,R96,92e95

0x80, 0x00, 0x00, 0x4c,   0x11, 0x00, 0x06, 0x36,    0x12, 0x00, 0x02, 0x3e,  0x13, 0x00, 0x07, 0x44,    //  sml3w R44,R3e,R36                                                       // sml3w R44,R3e,R36
0x80, 0x02, 0x0a, 0x37,   0x12, 0x00, 0x03, 0x96,    0x10, 0x00, 0x00, 0x01,                              // jnb   B7,R96,92e9d
0x80, 0x00, 0x00, 0x4c,   0x11, 0x00, 0x08, 0x34,    0x12, 0x00, 0x02, 0x3e,  0x13, 0x00, 0x09, 0x40,   // sml3w R40,R3e,R34
0x80, 0x00, 0x00, 0x68,   0x11, 0x00, 0x09, 0x40,    0x12, 0x00, 0x07, 0x44,                              // sb2w  R44,R40

// matchint interp for tblu2 ??

0xff
};


 // encoded address type 4 and 2, (xdt2)      handler 5

uchar enc24 [] = {
 12, 8, 0, 5 ,4, 0, 'e','n','c','2','4',0,
 0x80, 0x0 , 0x0 , 0xac,    0x11, 0x4 , 0x8 , 0x37,   0x13, 0x0 , 0xa , 0x3a,    // ldzbw R3a,R37;
 0x80, 0x0 , 0x0 , 0x71 ,   0x11, 0x1 , 0x3 , 0xf,    0x13, 0x0 , 0x8 , 0x37,    // an2b  R37,0x1f (t4) or 0xf (t2)
 0x80, 0x0 , 0x0 , 0x08 ,   0x11, 0x1 , 0x0 , 0x4 ,   0x13, 0x0 , 0xa , 0x3a,    // shrw  R3a,4

 0x81, 0x0 , 0x0,  0x1,                                                           // optional -  min, ix, max
 0x80, 0x00, 0x0 , 0x71 ,   0x11, 0x1 , 0x0 , 0xe ,   0x13, 0x0 , 0xa , 0x3a,    // an2b  R3a,e     not in type 2

 0x80, 0x0 , 0x0 , 0x67 ,   0x11, 0x0 , 0xa , 0x3a,   0x10, 0x1 , 0x5 , 0xf0,   0x13, 0x0 , 0x9 , 0x36,   //   ad2w  R36,[R3a+f0]
 0xff
};

 // encoded address types 3 and 1    - handler 5

 //nb. (R43 lbt to match R42)

uchar enc13 [] = {
 12, 7, 0, 5, 3, 0, 'e','n','c','1','3', 0,
 0x80, 0x0 , 0xd , 0x37,   0x12, 0x10, 0x8 , 0x43,                                // jnb   B7,R43,3cda  (drop bit)
 0x80, 0x0 , 0x0 , 0xac,   0x11, 0x10, 0x8 , 0x43,   0x13, 0x0 , 0x4 , 0x3a,      // ldzbw R3a,R43      (drop bit)

 0x83, 0x3 , 0x0 , 0x3 ,                                                          // 3 of 3 must match, any order
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x1 , 0x0 , 0xf ,   0x13, 0x10, 0x8 , 0x43,      // an2b  R43,f
 0x80, 0x0 , 0x0 , 0x71,   0x11, 0x1 , 0x3 , 0x70,   0x13, 0x0 , 0x4 , 0x3a,      // an2b  R3a,70  (0xfe is a t1, 0x70 is a t3)
 0x80, 0x0 , 0x0 , 0x18,   0x11, 0x1 , 0x0 , 0x3 ,   0x13, 0x0 , 0x4 , 0x3a,      // shrw  R3a,3

 0x80, 0x0 , 0x0 , 0x67,   0x11, 0x0 , 0x4 , 0x3a,   0x10, 0x1 , 0x5 , 0xf0,  0x13, 0x0 , 0x8 , 0x42, // ad2w  R42,[R3a+f0]   (R42 = R43)
 0x81, 0x0 , 0x0,  0x1 ,                                                            // 1 optional opcode
 0x80, 0x04, 0x1 , 0xc2 ,  0x13, 0x0 , 0x9 , 0x3e,   0x11, 0x0 , 0x8 , 0x42,      // stw   R42,[R3e]    optional (as ldx)
 0xff
};


/*
32 and 24 bit add ??

double increment [Rx++] = [Ry++]     (arg getters....

*/


// defines in shared.h

// handlers

void fnplu  (SIG*, SBK*);
void tbplu  (SIG*, SBK*);
void encdx  (SIG*, SBK*);
void ptimep (SIG*, SBK*);
void avcf   (SIG*, SBK*);
void rbasp  (SIG*, SBK*);
void sdummy  (SIG*, SBK*);

void subdummy  (MMH *h);
// for indexed lookup list

//could assemble pattern lists on the fly, but need to have subpatts identified somehow, or put them in main list ??
//last two outside std scan range


#define PATRBSE          0    // rbase lookup
#define SCANPATSTART     1     //limits for scan_sig, others sepcified explicitly
#define SCANPATEND       13

//same order at sig number (hdr[1])

//                     0                              5                                10
uchar *patterns[] =
{
  rbase  , avct  , times  , fnlu  , tblu,         // 0 - 4
  tbl2   , tbl3  , enc13  , enc24 , wtbl,         // 5 - 9
  setbank, fixarg, fixargm, fnlu2 , ttrps,        // 10-14
  wintp,   0,      0,       0,      0,            // 15-19

 // prefbit, prefnbt, postfnlu, postfnxx                                           //20 - subpatts from here ?
 };


// indexed sig handlers 1     2     3      4      5       6      7      8        9     10
SIGP sigproc[] =
{
  sdummy,  avcf  , ptimep , fnplu,  tbplu,        // 0 - 4
  encdx,   rbasp , sdummy                         // 5 - 9
  };

CHLDSUB  chldproc[] = {subdummy};  // csetbnk, cpoph, xdummy, xdummy};          //child pattern handlers  (none yet)




//************************************************************************







//************************************************************************
   #ifdef XDBGX

uchar* get_signame(uint patno)
{
   uchar *pat;
   pat = patterns[patno];
 return pat + 6;
}

void dbg_sig(MMH *ch)
{
    // debug
  uint i;

  if (!ch) return;

  DBGPRT(0,"[%2d %2x %2x]", ch->mc, ch->cinx1, ch->cinx2);
  for (i = 0; i < 32; i++) DBGPRT(0," %2x", ch->lvals[i]);
  DBGPRT(1, "  %s", get_signame(ch->patno));
}



void dbg_sigs(MMH *ch)
{

    //debug
  MMH* c;

  c = ch;

  while (c)
   {
    dbg_sig(c);
    c = c->caller;
   }
  DBGPRT(2,0);
}

   #endif




uint sigfail(MMH *m, uint ix)
 { //uint so that can do 'return sigfail(..)' and common exit for debugging
//  INST *c;

 //c = m->cinst;

// if (m->patno == 5 && m->sigstart == 0x92c94)                 //         !m->rptset && m->sigstart == 0x92f9f)
//  {
//    DBGPRT(0,0);
 //}




// for better debug stop.....


  m->sigmatch = 0;

  if (m->dbgflag && !m->failok)
     {
       #ifdef XDBGX

       DBGPRT(0, "sig failed (%d) %d, %s at %x, %x - ",m->patno, ix, get_signame(m->patno), m->sigstart, c->ofst);
       dbg_sig(m);
       #endif
     }
  return 1;
}



void subdummy  (MMH *h)
{
//maybe update addresses in here so it's optional ??



 #ifdef XDBGX
 DBGPRT(1,"** in dummy subsig handler for %s", get_signame(h->patno));
 #endif
}


void sdummy  (SIG* s, SBK* b)
{

 #ifdef XDBGX
 DBGPRT(1,"** in dummy sig handler for %s", get_signame(s->patno));
 #endif
}


void xdummy  (MMH *ch)
{

   #ifdef XDBGX
      DBGPRT(1,"** in dummy child handler");
      dbg_sigs(ch);

 #endif
}




uchar *skip_operands(uchar *start)
{
   uchar   *px;      // pointer within current pattern

    if (*start == SIGOCOD) start += 4;        // 0x80, skip opcode

    px = start;

    while (*px >= 0x10 && *px <= 0x13)    // operand patterns
       {
         if (px[1] & SIGWORD)  px += 5;
         else px +=4;
       }

  return px;
}


MMH* set_sigholder(uchar patix, MMH *caller)
{           //set up local holder for pattern match, child or caller sig

  MMH *mh;
  char *x;
  size_t sz;

// set pointers. Hold struct =  inst + scan + 2 uint arrays (1 saved + 1 local)

//Note. subsigs can get at svals/lvals via caller...

  sz = sizeof(MMH) + sizeof(INST) + sizeof(SBK) + (sizeof(uint) * (NSGV*2));        //add lvals

  x = (char*) cmem (0,0, sz);               // cleared on malloc

  mh          = (MMH*)   x;
  mh->cinst   = (INST*)  (x + sizeof(MMH));
  mh->scan    = (SBK*)   ((char*) mh->cinst + sizeof(INST));
  mh->lvals   = (uint*)  ((char*) mh->scan  + sizeof(SBK));    // local values
  mh->svals   = mh->lvals + NSGV;                              // saved values
  mh->msize   = sz;

 // process header, all or part

  mh->sx =  patterns[patix];

  if (mh->sx[0] < 7)
    {                   // must have at least one char for name (+ terminator zero)
       mh->sigmatch = 0;    // fail immediately
       return mh;
    }

  if (caller)
   {           //inherit flags first
    mh->caller  = caller;
    mh->skipcod = caller->skipcod;
    mh->dbgflag = caller->dbgflag;
    mh->sigstart = caller->sigstart;            // debug
 //   mh->subno    = mh->sx[1];                   // pattern num

   }

  mh->sigmatch = 1;
  mh->patno = mh->sx[1];                              //pattern number

  if (mh->sx[2] & 1) mh->novlp     = 1;               // no overlaps.
  if (mh->sx[2] & 2) mh->dbgflag   = 1;
  if (mh->sx[2] & 4) mh->cpyarrin  = 1;
  if (mh->sx[2] & 8) mh->cpyarrout = 1;

  mh->hix  = mh->sx[3];                     // handler index
  if (mh->sx[4]) mh->lvals[mh->sx[5]] = mh->sx[4];         // save fixed par if set
//  mh->sname = (char *)(mh->sx + 6);         // name       use patno

  mh->sx += mh->sx[0];                      // skip header block

  if (mh->sx[0] ==  SIGORPT) mh->novlp = 1;        // sig begins with repeat, set no overlap flag.

  if (caller && mh->cpyarrin) memcpy(mh->lvals, caller->lvals, NSGV*sizeof(uint));

  return mh;
}


void free_sigholder(MMH *mh)
{
   if (mh->caller && mh->cpyarrout && mh->sigmatch)        // sigmatch may not be reqd...
      memcpy(mh->caller->lvals, mh->lvals, NSGV*sizeof(uint));  // copy array back to parent
   mfree (mh, mh->msize);
}



void set_sigscan(uint xofst, SBK* ts)
  {
   memset(ts,0,sizeof(SBK));    // safety clear
   ts->curaddr = xofst;
   ts->start   = xofst;
   ts->nextaddr = xofst;
  }




uint match_operand(MMH *m)
{
    //match and save a single operand.

  uint val, ix, pval, sz;     //, mv;
       uint mask;
  INST *c;
  OPER *o;

  c = m->cinst;
  o = c->opnd + m->opno;
  sz = 4;

  ix  = m->sx[2];        // save index

  if (m->sx[1] & SIGLST)
    {
      ix += m->mc;                        // index is a LIST START
      if (ix > 31) ix =  m->sx[2];        // safety wrap, restart
    }

  pval = m->sx[3];               // pattern value (use if no index)

  if (m->sx[1] & SIGWORD)
    {                               // word match
      sz++;                         // word pattern
      pval = (pval << 8) | m->sx[4];        // assume no bank ?
    }

 if (m->sx[1] & SIGAINC)
    {
      if (o->optype != OPAINC)  return sigfail(m,21);
    }

//  address or reg check maybe addr should be default ??

  switch (o->optype)
  {
    case OPIMD:
    case OPADDR:
       val = o->addr;
       break;

    case OPOFF:
       val = o->val;
       break;

    default:
       val = o->reg;        // default is register
       break;
  }

//force address field.

 if (m->sx[1] & SIGADDR) val = o->addr;

// drop lowest bit in pattern and value, for some options

//optype = OPAINC, INX maybe

 if (m->sx[1] & SIGLBT) {val &= 0xfe; pval &= 0xfe;}

// if negative, and OFFSET type, mask down to relevant size.
/* code operand types ->optypes

 contents of operand
 type       reg         addr        val  fields

0  dflt     reg         reg         [reg]
1 OPIMD      0          imd+bank    imd (16 bit max)
2 OPBIT     reg         bit
3 OPADDR                addr+bank
4 OPOFF                             offset
5 OPIND     reg         [reg]       [[reg]]
6 OPINX     reg         [reg+off]   [[reg+off]]
7 OPAINC    reg         [reg]       [[reg]]


*/

if (o->optype == OPOFF)  // OPIMD OPADDR
   {

 //    val = o->addr;           //autswop to address field ??
     if (m->sx[1] & SIGWORD) mask = 0xffff; else mask = 0xff;
     val  &= mask;
     pval &= mask;

   }



/* jumps
[1] = OPADDR          target addr
[0] = OPOFF             jump offset

 if (o->optype == OPOFF && o->neg)
   {
     uint mask;
   //  val = o->addr;            // use val if negative offset. BUT NOT FOR JUMPS !!
     if (m->sx[1] & SIGIGN)
       { if (m->sx[1] & SIGWORD) mask = 0x8000; else  mask = 0x80; }  //test sign bit only
     else
       { if (m->sx[1] & SIGWORD) mask = 0xffff; else mask = 0xff; }

 //    val  &= mask;
   //  pval &= mask;

   } */


 if (ix)
   {
    if (!m->lvals[ix]) m->lvals[ix] = val;             // save value
    if (val != m->lvals[ix] && !(m->sx[1] & SIGIGN)) return sigfail(m,1);
   }
 else
   if ( pval != nobank(val) && !(m->sx[1] & SIGIGN)) return sigfail(m,1);           // compare without bank

  if(m->sigmatch) m->sx += sz; else return sigfail(m,1);

  return 0;
}



uint do_opcskp(uint foff)
  {
    // skip any 'optional' opcodes.  max of 16 allowed.
    // skippable are f2-f7, fa-ff,

    int ans, opc;

    ans = 0;

    while(1)
      {
       opc = g_byte(foff);                // code file value

       if (opc < 0xf2)  break;           // not skippable (all up to pushp)
       if (opc == 0xf8) break;           // clear carry
       if (opc == 0xf9) break;           // set   carry
       if (opc == 0xfe) break;           // sign or alt prefix

       foff++;
       ans++;
       if (ans >= 15 ) break;            // skip up to 15 opcodes
       if (!val_rom_addr(foff)) break;
      }
    return ans;                          // number of skips
   }




uint match_operands(MMH *m)
 {                 // do operands for one opcode
   // match operands following do_code, so all in inst structure

  int ix;

  if (!m->sx) return 0;           // safety
  if (!m->sigmatch) return 0;

 // if (m->sx[0] == SIGOCOD) m->sx += 4;       // skip opcode pattern

  while (1)
   {
    ix = m->sx[0];
    if (m->sx[0] == SIGOCOD) break;            // at next opcode pattern
    if (ix < 0x10 || ix > 0x13)
     {         // invalid operand number - flag as fail ??
      break;
     }

    m->opno = m->sx[0] & 3;           //operand 0 - 3
    match_operand(m);
    if (!m->sigmatch) break;
   }

  return 0;

 }



uint match_opcode(MMH *m)
 // verify opcode match, save bit number, check for ldx versus stx
 {
  uint mix, lix, sub, mask,  sz;
  const OPC *opl;
  INST *c;

  c = m->cinst;

  lix = get_opc_inx(m->sx[3]);                // ix is actual opcode reqd
  opl  = get_opc_entry(lix);                  // opcode table entry

  mix = m->sx[2];                             // and keep this
  sz = 4;

// match by sig index - ldx (12) and stx (13) are interchangeable.

  if ((c->sigix & 0x7e) == 12 && (opl->sigix & 0x7e) == 12) {}
  else  if (!c->sigix || c->sigix != opl->sigix) return sigfail(m,5);


// no save option for plain opcode ?? there should be....
// #define  SIGBIT   0x2         // save/match bit number (as mask) for bit jump  (index split into 4+4 bits = bit 0-7,  must match if ix & 80)

//needs more !!

 if (m->sx[1] & SIGBIT)
    {
        // split up sx[2] for sigbit
        mask = 1 << (c->opcode & 7);     // turn bit into mask (max is 80)
        sub =  1 << (mix >> 5);          // top 3 bits are bitno, turned into mask
        lix = mix & 0x1f;                // mask off save index

     if (c->sigix == 23 )          //why check opcode as well..??
       {                                   //jb (1000), jnb (0)
        if (lix)
          {
           // not quite same as standard operand match  *** WHAT ABOUT BIT ZERO ?? *** works as mask....
           if (!m->lvals[lix]) m->lvals[lix] = mask;       // save bitno
           if (m->lvals[lix] != mask) sigfail(m,6);        // must match saved bitno
           if ((c->opcix & 1))  m->lvals[lix] |= 0x1000;   // save bit jump type  JNB=0, JB=1
          }
        else
         {
           if (mask != sub)  sigfail(m,6);          // must match specified bitno
         }
       }

    }    //end SIGBIT

     if (c->sigix == 19 || c->sigix == 21)   // 19 = cond, 21 = stc,clc
       {
         lix = m->sx[2] & 0x1f;                                  // keep state
         if (lix && (c->opcix & 1))  m->lvals[lix] |= 0x1000;              //on or off  (e.g. JC versus JNC)
         if (lix && c->opcix >= 62 && c->opcix <= 69)  m->lvals[lix] |= 0x4000;    //signed or not
       }



/*
  if (c->sigix == 23 )                //jb, jnb  //jb (1000), jnb (0)
    {

      // split up sx[2] for bit matching or saving
      rqd = sx[2] & 80;                // is match required ?

      if (rqd)
        {
          rqdmask = (sx[2] >> 4) & 7;                    // bit number required
          if (rqd != (c->opcode & 7) sigfail(m,6);       // must match specified bitno
        }

      if (

  rqdmask = (sx[2] >> 4) & 7;      //bit number


          rqdmask = 1 << rqdmask;         // turn into bit into mask (max is 80)
      ix = sx[2] >> 4;                 // top nibble 3 bits are bitno reqd, + 'must match' 0x8)
      rqd =  1 << (ix & 7);            // required bit mask
      if (ix & 8) rqd |= 0x1000;       // set ' bit match required'

      ix = sx[2] & 0xf;                 // bottom nibble - save index
    mask = 1 << (c->opcode & 7);     // turn bit into mask (max is 80)

      if (ix)
          {         //index set
            if ((rqd & 0x1000)
              {
                if (rqd & 0xff) != mask)  sigfail(m,6);          // must match specified bitno
              }

           // not quite same as standard operand match
           if (!m->lvals[lix]) m->lvals[lix] = mask;       // save bitno
           if (m->lvals[lix] != mask) sigfail(m,6);        // must match saved bitno
           if ((c->opcix & 1))  m->lvals[lix] |= 0x1000;   // save bit jump type  JNB=0, JB=1
          }
        else
         {

         }
       }

    }
     if (c->sigix == 19 || c->sigix == 21)   // 19 = cond, 21 = stc,clc
       {                                        // keep state
         if (lix && (c->opcix & 1))  m->lvals[lix] |= 0x1000;

         if (lix && c->opcix >=62 && c->opcix < 69)  m->lvals[lix] |= 0x4000;
      //   62-69 are signed ops
          }
        //end SIGBIT
*/

 if (m->sx[1] & SIGSOPC)
   {       // save or match opcode address mode (autoinc ??)

     sub = (mix & 0x60) >> 5;     // opcsub (0 - 3) to match
     lix =  mix & 0x1f;           // index

     // (index split into 31 + 2 bits must match if ix = 0)

     if (!lix)
      {
            // allow 2 and 3 interchangeably 2 = [Rx+0], 3 = [Rx + y]
        if (sub > 1)         // indirect or indexed
          {
            if (c->opcsub < 2) sigfail(m,7);     // allow 2 or 3 to match (as [rx+0])
          }
        else
         {         // must match (0 or 1)
            if (c->opcsub != sub) sigfail(m,8);
         }
      }
     else
     {      //save and match
       if (!m->lvals[lix]) m->lvals[lix] = c->opcsub;      // save
       if (m->lvals[lix] != c->opcsub) sigfail(m,6);      // must match saved mode
     }
   }


   if (m->sx[1] & SIGFE)
    {                         // save/match sign flag
       sub =  mix >> 5;       // zero or 1 required
       lix = mix & 0x1f;      // mask off save index

       if (!lix)
         {
           if (c->feflg != sub) sigfail(m,6);         //must match
         }
      else
        {                //save sign bit
         if (!m->lvals[lix]) m->lvals[lix] = c->feflg;   // 0 or 1  save TEMP fix.
         if (m->lvals[lix] != c->feflg) sigfail(m,6);      // must match saved mode
        }
    }

 m->sx += sz;                        //skip to operands.
 match_operands(m);

 m->skipcod = 1;                          // skips allowed after first opcode processed
 return 1;
}



void match_data(MMH *m)
{
    // plain to start with
  uint ix, mp, mv;

  SBK *dsc;

  dsc = m->scan;

  if (m->sx[1] & SIGIGN)
    {
     m->sx += 4;
     dsc->curaddr++;              // next byte
     dsc->nextaddr++;
     return;
    }         // ignore value

  ix = m->sx[2];           // and its index
  mp = m->sx[3];                 // pattern value (use if no index

  mv = g_byte(dsc->curaddr);      // match value                            //but make this from the INST  (output)

  if (ix)
    {
      if (!m->lvals[ix])  m->lvals[ix] = mv;          // save data value
      if (m->lvals[ix] != mv) sigfail(m,9);           //m->smatch = 0;   // must match
    }
  else
    {
      if (mp != mv) sigfail(m,10);           //m->smatch = 0;      // must match
    }

  if (m->sigmatch)
    {
     m->sx += 4;                 // next pattern
     dsc->curaddr++;              // next byte
     dsc->nextaddr++;
    }

}




uint do_sigopcode(MMH *m)
{
    // get the next opcode and operands, only
  int skp;
  SBK *dsc;

  dsc = m->scan;

// if datapatt return datamatch() ?? ....

  if (!m->sx) return 0;           // safety

 if (m->sx[0] != SIGOCOD)
     {
      return sigfail(m,11);           //m->smatch = 0;   // not valid opcode sig - redundant ???
    //  return;
     }

  skp = 0;
  if (m->skipcod)
    {
      skp = do_opcskp(dsc->curaddr);
      dsc->curaddr += skp;
    }

  if (!val_rom_addr(dsc->curaddr) || skp > 16)
     {
      return sigfail(m,12);           // too many skips or hit end bank
     }

  do_code (m->scan, m->cinst);    //dsc...                 // one opcode at a time,like a scan

  if (dsc->inv) return sigfail(m,22);
  // but now can match the OUTPUT of do code - which is simpler...
  // and this includes the handler (i.e. swop for stx/ldx....)
  return 0;
}





 void do_stdpat(MMH *m)
  {  // split up for multipatterns get/match opcode, then operands.

     do_sigopcode(m);
     match_opcode(m);
   }


 void do_datpat(MMH *m)
  {
    match_data(m);

  }

void do_subpat(MMH *m)
 {
   // "x of y opcodes must match",  in any order.
   // max of 32 patts.

  int score, ix, i, min, num;
  uchar *start;
 // uint curaddr;

  min  = m->sx[1];          // num of opcode patts which must match (once each)
  num  = m->sx[3];          // num of patterns
  ix   = m->sx[2];          // index where to save count

  m->sx += 4;
  start = m->sx;          // start of patterns

 // assemble pat starts in an array. Keep one too many for resume after pattern....
 // don't know where opcode patterns are so scan for them

  for (i = 0; i <= num; i++)
   {
    savestart[i] = start;                // next opcode start (or end)
    if (*start != SIGOCOD) break;
    start = skip_operands(start);
   }

  score = 0;
  start = savestart[0];

  memcpy(m->svals, m->lvals, NSGV*sizeof(int));     // save pars

// do opcodes once, in order and loop around patterns for match

  while (score < min  && m->sigmatch)
    {
      do_sigopcode(m);

      for (i = 0; i < num; i++)
       {
        m->mc = 0;

        if (savestart[i])
          {
           m->sx = savestart[i];
           m->sigmatch = 1;                             // reset match
           memcpy(m->svals, m->lvals, NSGV*sizeof(int));   // save pars if match

           match_opcode(m);

           if (m->sigmatch)
            {
             score++;
             m->mc++;
             savestart[i] = 0;          // matched
             break;                      // to next opcode
            }
           else
            {
             memcpy(m->lvals, m->svals, NSGV*sizeof(int));       //restore pars if no match
            }
          }    // savep[i]
       }      //  while i < patts



      if (!m->mc)
         {
           sigfail(m,13);    // at least one patt must match for each pass
           break;            // outer loop
         }
    }  // while score

      if (score >= min)
         {                     // pass
          m->sigmatch = 1;
          if (ix)   m->lvals[ix] = score;
          }

   m->sx = savestart[num];         // next pattern after this subpattern
 }



uint match_sig (MMH *m, uint ofst);

void do_subchild(MMH *caller)
 {
   // "x of y childsigs must match",  in any order.
   // max of 32 patts.

  int score, ix, i, min, num;

  MMH *lhold;                    // local MMH structure


//  if (caller->scan->curaddr == 0x92b55)
//  {
//      DBGPRT(0,0);
//  }


  min  = caller->sx[1];          // num of child sigs which must match (once each)
  num  = caller->sx[3];          // num of child sigs
  ix   = caller->sx[2];          // index where to save count

  caller->sx += 4;


 // assemble pat starts in an array. Keep one too many for resume after pattern....
 // don't know where opcode patterns are so scan for them

  for (i = 0; i <= num; i++)
   {
    savechild[i] = caller->sx[i];                // next indexes (or end) can't do this...ptr
   }
  caller->sx += num;                       // where to resume

  score = 0;
//  start = savepstart[0];

  memcpy(caller->svals, caller->lvals, NSGV*sizeof(int));     // save pars

// do child sigs once, in order and loop around them for match

  while (score < min  && caller->sigmatch)
    {
      for (i = 0; i < num; i++)
       {
        caller->mc = 0;                    //match count
        if (savechild[i])
          {
            caller->sigmatch = 1;                             // reset match
            memcpy(caller->svals, caller->lvals, NSGV*sizeof(int));   // save pars if match
            lhold =  set_sigholder(savechild[i], caller);
            match_sig(lhold, caller->scan->curaddr);

            if (lhold->sigmatch)
             {
              caller->mc++;
              score++;
              savechild[i] = 0;          // matched
              chldproc[lhold->hix].pr_sig(lhold);
              caller->scan->curaddr  = lhold->scan->curaddr;     //move past matched sig
              caller->scan->nextaddr = lhold->scan->nextaddr;
              break;                      // to next child
             }
            else
             {
               memcpy(caller->lvals, caller->svals, NSGV*sizeof(int));       //restore pars if no match
             }
            free_sigholder(lhold);
          }    // sigp
       }      //  while i < num

      if (!caller->mc)
         {
           sigfail(caller,13);    // at least one patt must match for each pass
           break;            // outer loop
         }
    }  // while score & sigmatch

      if (score >= min)
         {                     // pass
          caller->sigmatch = 1;
          if (ix)   caller->lvals[ix] = score;
          }

 //  m->sx = savepstart[num];         // next pattern after this subpattern   done.
 }

















 void do_datsub(MMH *m)
  {


  }

 uint do_skppat(MMH *m)
 {
   // 'skip to defined pattern'
// min - max

  int ix, max, flags;
  uint cur;
  uchar *start;

//m->sx is uchar, sp could save for later instead of three vals ?

  flags   = m->sx[1];          // step back op if set - maybe this becomes default ?
  ix   = m->sx[2];          // index where to save count
  max  = m->sx[3];          // num of skips

  start = m->sx + 4;        // start of next pattern, assume ONE to match

  m->mc = 0;                   //skip count


  if (max < 1) max = 1;       //safety

 m->failok = 1;               //for debug
  while (m->mc < max)
   {
     m->sx = start;          // reset for match
     cur = m->scan->curaddr;  // save curaddr before match
     do_sigopcode(m);
     match_opcode(m);

     if (m->sigmatch) break;    // matched opcode and operands

     m->mc++;                    // skip count
     m->sigmatch = 1;           // reset match
   }

//check skip count.......??
 m->failok = 0;
 if (m->mc >= max) return sigfail(m,15);

 if (ix) m->lvals[ix] = m->mc;
 if (flags & 1) m->scan->curaddr = cur;        // restore start of matched opcode

return 1;
 }



 uint do_rptpat(MMH *m)
  {
    // to match between x and y repeats, for single pattern
    //  where x can be zero and y is one (= optional)
   int ix, min, max;
   uchar *start;
   uint curaddr;

//if (m->sigstart == 0x92f9f)
//{
 //   DBGPRT(0,0);
//}



  min   = m->sx[1];          // min repeats
  max   = m->sx[3];          // max repeats
  ix    = m->sx[2];          // index where to save count
  start = m->sx + 4;

  m->mc = 0;                     // repeat count

  while (m->mc < max)
   {
     m->failok = 1;               //for debug
     m->sx = start;               // reset pattern
     curaddr = m->scan->curaddr;  // save curaddr
     do_sigopcode(m);             // this advances curaddr =nextaddr
     match_opcode(m);

     if (!m->sigmatch)
       {
         m->sx = skip_operands(m->sx);   // skip rest of pattern, find next opcode...
         m->scan->curaddr = curaddr;      //restore start
         break;
       }
     m->mc++;                              // match count
   }


 //  if (m->mc < min || m->mc > max) m->sigmatch = 0; else m->sigmatch = 1;
  m->failok = 0;
  m->sigmatch = 1;               // can fail once in a repeat
  if (m->mc < min || m->mc > max) return sigfail(m,15);

  if (ix) m->lvals[ix] = m->mc;

//would call a handler here ??

 return 1;
 }




uint do_childsig (MMH *caller)
{
   // true child signature

  MMH *lhold;                    // local MMH structure for recursive call.

  lhold =  set_sigholder(caller->sx[1], caller);

  match_sig(lhold, caller->scan->curaddr);

 if (!lhold->sigmatch) return sigfail(caller,14);           // does caller->smatch in subr

  // update caller struct with matched address if sig takes space.....
  // NB lhold caller = caller at this point. move address to handler ??

 chldproc[lhold->hix].pr_sig(lhold);
 caller->scan->curaddr  = lhold->scan->curaddr;
 caller->scan->nextaddr = lhold->scan->nextaddr;

 caller->sx += 2;             // skip child sig entry to next pattern

 free_sigholder(lhold);
return 1;
}

 uint do_rptchild(MMH *caller)
  {
    // match childsig between x and y repeats, for one child sig
    //  where x can be zero and y is one (= optional)

    // MUST HAVE A SIG HANDLER, or dummy

   int min, max;
   uchar *start;

   //6 byte header

//need to add handler maybe, e.g. for paramter changes after child sig (e.g. bank select drops a par from 'pop'


  MMH *lhold;           //local sig holder for recursive call.

  lhold =  set_sigholder(caller->sx[4], caller);    // child sig to call is in [4]

  min   = caller->sx[1];             // min repeats
  max   = caller->sx[3];             // max repeats

  lhold->cinx1 = caller->sx[2];     // remote index for value list or count, in caller
  lhold->cinx2 = caller->sx[5];     // second remote index

  caller->sx += 6;                   // skip entry in caller pattern

  start = lhold->sx;                 // to restart the pattern

  lhold->mc = 0;                     // repeat count

  while (lhold->mc < max)
   {
     lhold->sx = start;
     match_sig(lhold, caller->scan->curaddr);

     if (!lhold->sigmatch)  break;             // can fail match if not repeat

    // update caller struct, address and pattern

    lhold->mc++;

    caller->scan->curaddr  = lhold->scan->curaddr;
    caller->scan->nextaddr = lhold->scan->nextaddr;

   }

  lhold->sigmatch = 1;                      // can fail once and still match
  if (lhold->mc < min || lhold->mc > max)
      {
           free_sigholder(lhold);
          return sigfail(caller,15);
      }

 // matched, run handler, once for this loop

 chldproc[lhold->hix].pr_sig(lhold);

 caller->skipcod = 1;      //can skip opcodes once passed

 free_sigholder(lhold);

 return 1;

 }


 void do_datrpt(MMH *m)
  {


  }












uint match_sig (MMH *m, uint ofst)
{
  uchar fl;                        // flags

  SBK *dsc;

  dsc = m->scan;

  set_sigscan(ofst, dsc);         // set up scan block for do_code calls

  while (m->sigmatch)
   {
     if (!m->sx)  sigfail(m,16);                        // no pattern, safety
     if (!val_rom_addr(dsc->curaddr)) sigfail(m,17);    // fail if invalid addr
     if (m->sigmatch == 0) break;                       // match failed

     fl = *(m->sx);                // next pattern flag
     if (fl == 0xff) break;        // end of pattern - safety

     switch(fl)
     {       //should only ever be one value set

      //opcode patterns, match opcode + operands

      case SIGOCOD :        // std opcode pattern - do one entry. NB. used in multi patterns
        do_stdpat(m);
        break;

      case SIGOSKP:         // 'skip to opcode X' pattern
        do_skppat (m);
        break;

      case SIGOSUB:         // subpattern  (any order match x of y)
        do_subpat (m);
        break;

      case SIGORPT:         // opcode repeat and optional
        do_rptpat(m);
        break;

      case SIGCHLD:        // full sig pattern as child sig         [1] is flags [2] is subpattern index
        do_childsig(m);
        break;

      case SCHORPT:
        do_rptchild(m);
        break;

      case SCHOSUB:
        do_subchild (m);
        break;


      case SIGDATA:         // single data pattern
        do_datpat (m);
        break;
/*
      case SIGDRPT:         // data pattern - optional or repeat
        do_datrpt (m);
        break;

      case SIGDSUB:         // data pattern - match x of y in any order
        do_datsub (m);
        break;      */

      default:
        sigfail(m,18);           //m->smatch = 0;
        break;              // failed (safety)
     }        // end of switch
    }       // end of while smatch

  if (dsc->curaddr <= ofst) sigfail(m,19);           //m->smatch = 0;      // must have at least one matched patt


  if (m->sigmatch == 0) return 0;

  return ofst;
}








//maybe modify to allow params array to be fed in, so that sigs can be matchd in segments....


SIG* do_sig (uint patix, uint ofst)
{
 SIG *z, *r;
 //CHAIN *x;
 MMH *mhold;

  if (get_anlpass() >= ANLPRT) return 0;        //get_anlpass() >= ANLPRT) return 0;
  if (!val_rom_addr(ofst))     return 0;

  // should check 'pat' is valid...............




  r = 0;

  mhold = set_sigholder(patix, 0);

  mhold->sigstart = ofst;               // debug

  // check if sig previously found and overlaps already ?? only if same sig and no overlaps set ??


  match_sig (mhold, ofst);

  if (mhold->sigmatch)
    {
      // match - copy details and add valid sig
 //     x = get_chain(CHSIG);

// x = get_chain(CHSIG);
      z = (SIG *) chimem(CHSIG);

     // z = (SIG*) x->ptrs[ix];
      z->start = ofst;                      // update start and end addresses
      z->end   = mhold->scan->curaddr-1;
      z->hix   = mhold->hix;
      z->patno = mhold->patno;
      memcpy( z->vx,   mhold->lvals, sizeof(uint) * NSGV);

      r = add_sig(z);                    // insert sig into chain

  /*    #ifdef XDBGX
        DBGPRT(0, "add sig");
        if (!r) DBGPRT(0, " FAIL");
        DBGPRT(1, " %s %x %x", z->name,z->start,z->end);
      #endif  */

    }

   free_sigholder(mhold);
  return r;
 }


void do_sigproc (SIG* g, SBK* s)
{
  if (g && g->hix)
    {
     sigproc[g->hix].pr_sig(g,s);      // call signature processor
    }
}


SIG* scan_sigs (int ofst)
{
// scan ofst for signatures from list supplied. EXTERNALLY visible

  uint j;
  SIG *s;

  if (get_cmdopt(OPTMAN))  return 0;              // manual option set
  if (!get_cmdopt(OPTSIG)) return 0;              // signature option not set


//if (ofst == 0x9371f)
//{
 //   DBGPRT(0,0);
//}

   s = get_sig(ofst);           // check for sig already at this address

   if (s) return s;


//allow for some to be dropped off by defines

//TEMP 11 test split sig
 //  s = do_sig (13, ofst);


  for (j = SCANPATSTART; j <= SCANPATEND; j++)
      {
        s = do_sig (j, ofst);        // code list
        if (s) break;                // stop as soon as one found
      }



 return s;
}



/*
 *
 * only bank 8 and rbase sig to find ???
 */

void prescan_sigs (void)
{
 // scan whole binary file (exclude fill areas) for signatures first.
 // do preprocess if reqd

  uint ofst;
  SIG *s;
  BANK *b;

  if (get_cmdopt(OPTMAN)) return ;             //get_cmdopt(OPTMANL)) return ;             // manual stops it ??
 // if (!get_cmdopt(OPTSIG)) return ;          //not if rbases, must ALWAYS look for it ?.

  #ifdef XDBGX
   DBGPRT(1,0);
   DBGPRT(1," -- Scan bank 8 for pre sigs --");
  #endif


//  for (i = 0; i < BMAX; i++)
//   {
     b =  get_bankmap(9);
   //  if (!b->bok) continue;

     for (ofst = b->minromadd; ofst < b->maxromadd; ofst++)
       {
         show_prog ();       //anlpass);               //get_anlpass());

    //     for (j = SCANPATSTART; j <= SCANPATEND; j++)
     //       {
             s = do_sig (PATRBSE, ofst);                  // prescan list
             if (s)
              {
               ofst = s->end;
               sigproc[s->hix].pr_sig(s,0);    // call handler for signature
               break;
              }
         //   }
       }
  // }

 #ifdef XDBGX
   DBGPRT(2," -- End Scan whole bin --");
  #endif
}



/// END of sign.cpp
