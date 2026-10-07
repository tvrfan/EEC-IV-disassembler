

#include "command.h"

cchar *cempty = "\0\0\0";      // safety. allows ptr increments, fixes compiler warnings

char  flbuf [256];            // for directives and comment file reads

CPS   cmnd;                   // command structure

char  nm    [300];            // for temp name construction, string hacking, etc.


// for temp and number assembly for syms, nm is const when accessed via tempstr
CSTR tempstr = {0,0,0,0, (cchar *) &nm};

HDATA flhdr;                 // files path and bin size (declared in shared.h)
FDATA fldat[7];              // each file (handle, name, etc)


//********** file extensions and flags for linux and Win32 ********************

#ifdef __linux__

// linux (gcc)

  HOP fsfx[] ={{".bin","r"},    {"_lst.txt","w"}, {"_msg.txt","w"},
             {"_dir.txt","r"},{"_cmt.txt","r"}, {"_dbg.txt","w"}, {"sad.ini","r"}};


#endif

#if defined __MINGW32__ || defined __MINGW64__

// windows (mingw)

   HOP fsfx[] ={{".bin","rb"},    {"_lst.txt","wt"}, {"_msg.txt","wt"},
              {"_dir.txt","rt"},{"_cmt.txt","rt"}, {"_dbg.txt","wt"}, {"sad.ini","rt"}  };


#endif

// number files, does not include sad.ini (= entry [6]), but does include debug file.

#ifdef XDBGX
  int numfiles = 6;
#else
  int numfiles = 5;
#endif

struct                //could become a CSTR ?
{
    uint err;
    cchar *txt;
} etxts[]
  = {
{ 0, cempty},
{ 2, "Duplicate Command Ignored"},
{ 1, "Invalid Address"},
{ 1, "Banks Not Match"},
{ 1, "Odd address Invalid"},
{ 1, "Commands Overlap"},
{ 1, "Ranges Overlap"},
{ 2, "XCODE bans Command"},
{ 2, "New Symname replaces previous one"}

 };

 cchar *htxt[] = {cempty,"Cmd Rejected - ", "Cmd Info - "}; // "info" /

/*
#define E_DUPL  1          // duplicate
#define E_INVA  2          // invalid address
#define E_BKNM  3          // banks no match
#define E_ODDB  4          //odd address (WORD etc)
#define E_OVLP  5          //overlap
#define E_OVRG  6          //overlapping ranges
#define E_XCOD  7          // XCODE bans

*/
/*****************************************************************************
* declarations for subroutines in command structure declaration
***************************************************************************/
//preset strings for symbol names and command strings


// 'setopt' set/clear  strings
// params =  selected (0/1), has param,  minmatch, strlen, name
// put param ones first for printout

CSTR optstrs[] =  {
  { 3 , 1, 3, 7,  "dplaces"    } ,        // decimal places, calcs too.  par1 is value.
  { 1 , 0, 3, 6,  "sceprt"     } ,
  { 1 , 0, 3, 8,  "tabnames"   } ,
  { 1 , 0, 3, 9,  "funcnames"  } ,
  { 1 , 0, 3, 9,  "ssubnames"  } ,
  { 0 , 0, 3, 10, "labelnames" } ,
  { 0 , 0, 3, 6,  "manual"     } ,
  { 1 , 0, 3, 9,  "intrnames"  } ,
  { 1 , 0, 3, 8,  "subnames"   } ,
  { 1 , 0, 3, 10, "signatures" } ,
  { 1 , 0, 3, 10, "sympresets" } ,
  { 0 , 0, 3, 4,  "8065"       } ,
  { 0 , 0, 4, 7,  "cptargs"    } ,
  { 1 , 0, 4, 7,  "cptdata"    } ,
  { 1 , 0, 3, 6,  "braces"     } ,         // turn off the 'if () {}' for goto


 // { 0 , 0, 3, 9,  "acomments"  }

  };


//special function subroutines - lookups
 // spf func, index into autonames, min match, strlen, name

CSTR spfstrs[] = {
  { 0,  0,  6, 0, cempty },
  { 4,  5,  3, 6,"uuyflu" },     //starts at index [1]
  { 4,  6,  3, 6,"usyflu" } ,
  { 4,  7,  3, 6,"suyflu" },
  { 4,  8,  3, 6,"ssyflu" },

  { 4,  9,  3, 6,"uuwflu" },
  { 4, 10,  3, 6,"uswflu" },
  { 4, 11,  3, 6,"suwflu" },
  { 4, 12,  3, 6,"sswflu" },

  { 5, 13,  3, 5,"uytlu" } , //could be 2
  { 5, 14,  3, 5,"sytlu" } ,

  { 5, 15,  3, 5,"uwtlu" } ,
  { 5, 16,  3, 5,"swtlu" }

};



//option,  fendin, fendout (with sign),  strlen, name
//maybe autonumber in par1 too for some...

CSTR autonames [] = {
 { 0        ,    0,    0, 0, cempty    },
 { OPTSUB   ,    0,    0, 3, "Sub"     },          // 1   PPROC   auto proc names (func)
 { OPTLAB   ,    0,    0, 3, "Lab"     },          // 2   PLABEL  label names (lab)
 { OPTFUNC  ,    0,    0, 4, "Func"    },          // 3   PFUNC   function data names
 { OPTTBL   ,    0,    0, 5, "Table"   },          // 4   PTABL   table data names

//uufunc,usfunc,sufunc,ssfunc ??



 { OPTLU    ,    7,    7, 9, "UUYFuncLU" },      // 5   PSTRS   auto subroutine names for function and table lookup procs
 { OPTLU    ,    7, 0x27, 9, "USYFuncLU" },
 { OPTLU    , 0x27,    7, 9, "SUYFuncLU" },
 { OPTLU    , 0x27, 0x27, 9, "SSYFuncLU" },

 { OPTLU    , 0xf ,  0xf, 9, "UUWFuncLU" },        // 9
 { OPTLU    , 0xf , 0x2f, 9, "USWFuncLU" },
 { OPTLU    , 0x2f,  0xf, 9, "SUWFuncLU" },
 { OPTLU    , 0x2f, 0x2f, 9, "SSWFuncLU" },


 { OPTLU    ,    7,    7, 7, "UYTabLU"  },         // 13
 { OPTLU    , 0x27, 0x27, 7, "SYTabLU"  },
 { OPTLU    ,  0xf,  0xf, 7, "UWTabLU"  },         // 15
 { OPTLU    , 0x2f, 0x2f, 7, "SWTabLU"  },

 { 0        ,    7,    7, 5, "Interpolate"    },          // 17

 { 0        ,    0,    0, 0, cempty      },                 // 18 null entry
 { 0        ,    0,    0, 0, cempty      },                 // 19 null entry

 { 0        ,    1,    0, 1, "_Enc" }   //ncode"  }               // 20 - sys math names from here ?? NEED AUTONUMBER ??
 };


//separate list for sysm math func names ??





// main command strings
//unused,unused, min match, strlen, name

CSTR cmds[] = {
   { 0, 0,  3, 4, "fill" },           //zero is valid answer
   { 0, 0,  3, 4, "byte" },
   { 0, 0,  3, 4, "word" },
   { 0, 0,  3, 6, "triple" },
   { 0, 0,  3, 4, "long" },
   { 0, 0,  3, 4, "text" },
   { 0, 0,  3, 4, "vect" },
   { 0, 0,  3, 5, "table" },
   { 0, 0,  3, 4, "func" },
   { 0, 0,  3, 6, "struct" },
   { 0, 0,  3, 5, "timer" },
   { 0, 0,  3, 4, "code" },
   { 0, 0,  3, 4, "args" },
   { 0, 0,  3, 5, "xcode" },
   { 0, 0,  3, 4, "subr" },
   { 0, 0,  3, 4, "scan" },
   { 0, 0,  3, 5, "rbase" },
   { 0, 0,  3, 6, "symbol" },
   { 0, 0,  3, 4, "bank" },
   { 0, 0,  3, 6, "clropt" },
   { 0, 0,  3, 6, "setopt" },
   { 0, 0,  3, 6, "pswset" },
   { 0, 0,  3, 4, "calc" }
 };

//math calc builtin functions

// unused, unused, min match, strlen, name

CSTR mathfs [] = {
   { 0, 0,  6, 0, cempty},
   { 0, 0,  1, 1, "+"},           //starts at index 1
   { 0, 0,  1, 1, "-"},
   { 0, 0,  1, 1, "*"},
   { 0, 0,  1, 1, "/"},
   { 0, 0,  3, 3, "enc"},
   { 0, 0,  3, 3, "pwr"},
   { 0, 0,  3, 4, "root"},
   { 0, 0,  3, 4, "sqrt"},
   { 0, 0,  3, 5, "volts"},
   { 0, 0,  3, 5, "bound" }
  };

//  n.B. cube root = pwr (= pow) 1/3;   scalers used a lot, allow for a percentage setup ???

//hex, shift, part field would allow encodes as calcs.

//bits (a,b)  shift >> << , ashift S>> ??
// getaddr(address bitstart,bitend);

//calc type command strings
//calctytpe is offset
//unused, unused, min match, strlen, name

CSTR calcstr[] = {
  { 0, 0,        6 , 0, cempty},         //dummy zero
  { 0, DP_HEX  , 6 , 0, cempty},         //starts at index 1
  { 0, DP_DADD , 3 , 7, "address"},
  { 0, DP_BIN  , 3 , 6, cempty},
  { 0, DP_DEC  , 3 , 7, "integer"},
  { 0, DP_FLT  , 3 , 5, "float"}

  //other not valid as calc type
 };  //hex, bin ??

/*
#define DP_HEX      1               // hex          integer
#define DP_DADD     2               // hex+bank     data address (16 bit wrap)
#define DP_BIN      3               // binary       integer
#define DP_DEC      4               // decimal      integer
#define DP_FLT      5               // float        float
#define DP_REF      6               // 'x' ref      extract from ADT definition (= data field) maths only
#define DP_SUB      7               //  -           subterm to calc, maths only
#define DP_CADD     8               // hex+bank     CODE address (16 bit wrap)
*/





 /* interrupt subroutine names arrays, for auto naming
  * "HSI_" and default name is output as prefix in code
  * params -
  * 1 -unused 2 unused 3 start 4 end  address(es) of interrupt (+ 0xB2000)
  * number appended if end-start > 1 (8065)
  * name
  */

  //interrupt handler names 8061, 8065

//unused, start,end, strlen, name    (start, end adresses - bank+2000)

//should never have odd address input, this is just done for safety



CSTR inames61[] = {
{ 0, 0x10,  0x11,  5,  "HSO_2" },            //  8061 ints - actual at 2010
{ 0, 0x12,  0x13,  9,  "Timer_OVF" },
{ 0, 0x14,  0x15,  6,  "AD_Rdy" },
{ 0, 0x16,  0x17,  8,  "HSI_Data" },
{ 0, 0x18,  0x19,  8,  "External" },
{ 0, 0x1a,  0x1b,  5,  "HSO_1" },
{ 0, 0x1c,  0x1d,  5,  "HSI_1" },
{ 0, 0x1e,  0x1f,  5,  "HSI_0" }            // 201f  - end 8061 list
};

CSTR inames65[] = {
{ 0, 0x10,  0x2f,  4,  "HSO_" },           // 8065 ints - 2010-205f (plus int num)
{ 0, 0x30,  0x31,  8,  "HSI_FIFO" },
{ 0, 0x32,  0x33,  8,  "External" },
{ 0, 0x34,  0x35,  5,  "HSI_0" },
{ 0, 0x36,  0x37,  8,  "HSI_Data" },
{ 0, 0x38,  0x39,  5,  "HSI_1" },
{ 0, 0x3a,  0x3b,  10, "AD_Imm_Rdy" },
{ 0, 0x3c,  0x3d,  12, "AD_Timed_Rdy" },
{ 0, 0x3e,  0x3f,  12, "AltTimer_OVF" },
{ 0, 0x40,  0x41,  14, "AD_Timed_Start" },
{ 0, 0x42,  0x42,  14, "AltTimer_reset" },
{ 0, 0x44,  0x4b,  8,  "Counter_" },            // add num
{ 0, 0x4c,  0x5f,  9,  "Software_" }          // add num 2040-205f    end 8065
};




//master index to allow other progs to get at the above strings
//strings array, size of array, min index (to add),  delimiter required

CSINX strindex [] = {
    {optstrs   , NC(optstrs)   ,  1 },
    {spfstrs   , NC(spfstrs)   ,  1 },
    {cmds      , NC(cmds)      ,  1 },
    {mathfs    , NC(mathfs)    ,  0 },
    {calcstr   , NC(calcstr)   ,  1 },
    {inames61  , NC(inames61)  ,  1 },
    {inames65  , NC(inames65)  ,  1 },
    {autonames , NC(autonames) ,  1 }

};
  /* CSTR* strlist;
   uint size;       //number of entries (cells)

   uint startix  : 2;   // (0 or 1) gets added to answer....
   uint delim    : 1;    //  9for
*/

/***** Auto default symbols *******

 * added to analysis if PRSYM set
 * fixed names for special regs as Ford Handbook
 * Params in order -
  start bit number      as byte 0-7
  end bit number         with sign (0x20) write (0x40 and 'whole' flags (0x80)
  address (as byte)
  strlen               SFR word overlaps with next byte
  name;                 name
*/



/********************************
 * Preset sym names structure (SFRs)
 one for 8061, one for 8065
 ********************************/

 // 8061 SFR list

 /*
 0 1 6 7 are read only.

 d is byte write only  E F are word read only

 4 byte write only
 4 5  is byte read only

 */






CSTR d61syms [] = {


 { 6,  0x6 , 0x2,   6,  "CPU_OK"        },     // 2->6 are byte only
 { 0,  0x87, 0x2,  10,  "LSOut_Port"      },   // LSO 0-7 as bits ?

 { 0,  0x87, 0x3,   9,  "BIDI_Port"      },
 { 0,  0xc7, 0x3,   9,  "BIDO_Port"      },       //write
 //only 0 and 1 ??   2-7 don't care ??
 { 0,  0x87, 0x4,  12,  "AD_Result_Lo"  },     //read
 { 0,  0xc7, 0x4,   6,  "AD_Cmd"        },     // Write

 //    {0, 0, 3, 0x4, "AD_Channel"      },
 //4,5 don't care 0xffc0 is max.
    // {4 7, 32, 0x4, "AD_Low"      },
    // {1, 0, 3, 0x4, "AD_Cmd"       },          // Write symbols from here test with 4 bits
    // {0, 6, 15, 0x4, "AD_Result"      },       // TEST !! crosses bytes

 { 0,  0x87, 0x5,  12,  "AD_result_Hi"  },     // read
 { 0,  0xc7, 0x5,   9,  "WDG_Timer"     },     // write
 { 0,  0x8f, 0x6,   8,  "IO_Timer"      },     // word only, 7  not valid by itself
 { 0,  0x87, 0x8,   8,  "INT_Mask"      },

/*
0 HSO_PORT_2 enabled           AND 9  (next)
1 I?O timer ovf
2 A?D ready
3 HSI data ready
4 External int
5 HSO PORT-1
6 HSI 1
7 HSI 2
*/

 { 0,  0x87, 0x9,   8,  "INT_Pend"      },


 /*
0 HSO_PORT_2 pending
1 I?O timer ovf
2 A?D ready
3 HSI data ready
4 External int
5 HSO PORT-1
6 HSI 1
7 HSI 2
*/





 { 0,     0, 0xa,   7,  "HSO_OVF"       },
 { 1,     1, 0xa,   7,  "HSI_OVF"       },
 { 2,     2, 0xa,   9,  "HSI_Ready"     },
 { 3,     3, 0xa,   8,  "AD_Ready"      },
 { 4,     4, 0xa,  13,  "Int_Servicing" },
 { 5,     5, 0xa,  12,  "Int_Priority"  },
 { 0,  0x87, 0xa,   9,  "IO_Status"     },

 { 0,  0x87, 0xb,  10,  "HSI_Sample"    },
 { 0,  0x87, 0xc,   8,  "HSI_Mask"      },
 { 0,  0x87, 0xd,   8,  "HSI_Data"      },     // read
 { 0,  0xc7, 0xd,   7,  "HSO_Cmd"       },     // write
 { 0,  0x8f, 0xe,   8,  "HSI_Time"      },     // word only, 0xf not valid by itself
 { 0,  0x8f, 0x10,  8,  "StackPtr"      }

};


//8065 SFR list

//NB we meed some kind of marker here to stop runover into next byte for certain SFR registers.

/*
overlaps for names 0and 1  byte or word
 6/7 e/F 10/11 12/13 14/15 16/17 18/19 1C/1D
 Read only
 0/1 6/7  f  18/19  1C/1D (200/201, 300 301)

 but handbook does imply only way to get to shared is
12+13 as WORD
13 as BYTE   so overlap is fine.

15 17 19 1D have different names R and W and are also
overlapped, but NOT R/W .

4 6 7 D 13 15 17 19 1D appear to be byte read/write ONLY

EF 1C 1D is WORD only.

text implies 6/7  E/F 10/11  12/13 14/15 16/17 18/19 1c/1d are WORD ONLY.


Note -

*/

CSTR d65syms [] = {

 { 6,     6, 0x2 ,  6,  "CPU_OK"              },
 { 0,     7, 0x2 ,  8,  "LSO_Port"            },
 { 0,     7, 0x3 ,  8,  "LIO_Port"            },
 { 0,   0x7, 0x4 , 16,  "AD_Imm_Result_Lo"    },      // as byte
 { 0,  0x8f, 0x4 , 14,  "AD_Imm_Result"       },      // as word
 { 0,  0xc7, 0x4 ,  6,  "AD_Cmd"              },      // Write
 { 0,  0x87, 0x5 , 18,  "AD_Imm_Result_High"  },
 { 0,  0xc7, 0x5 ,  9,  "WDG_Timer"           },      // write

 { 0,  0x87, 0x6 , 11,  "IO_Timer_Lo"         },      // as byte
 { 0,  0x8f, 0x6 ,  8,  "IO_Timer"            },      // overlap
 { 0,  0x87, 0x7 , 18,  "AD_Timed_Result_Lo"  },      // read
 { 0,  0xc7, 0x7 , 12,  "AD_Timed_Cmd"        },      // write

 { 0,  0x87, 0x8 ,  8,  "INT_Mask"            },
 { 0,  0x87, 0x9 ,  9,  "INT_Pend"            },

 { 0,    0, 0xa,    7,  "HSO_OVF"             },
 { 1,    1, 0xa,    7,  "HSI_OVF"             },
 { 2,    2, 0xa,    9,  "HSI_Ready"    },
 { 3,    3, 0xa,   12,  "AD_Imm_Ready"     },
 { 4,    4, 0xa,   10,  "MEM_Expand"    },
 { 5,    5, 0xa,   12,  "HSO_Read_OVF"  },
 { 5,    6, 0xa,   12,  "HSO_Pcnt_OVF"  },
 { 6,    6, 0xa,   14,  "Timed_AD_Ready"},
 { 7,    7, 0xa,   12,  "HSO_Port_OVF"  },

 { 0,  0x87, 0xa ,   9, "IO_Status"     },
 { 0,  0x87, 0xb ,  10, "HSI_Sample"    },
 { 0,  0xc7, 0xb ,  14, "IDDQ_Test Mode"    },       //write
 { 0,  0x87, 0xc ,  14, "HSI_Trans_Mask"  },
 { 0,  0x87, 0xd ,   8, "HSI_Data"  },
 { 0,  0xc7, 0xd ,   7, "HSO_Cmd"      },

 { 0,  0x8f, 0xe ,  13, "HSI_Time_Hold"  },
 { 0,  0x87, 0xf ,  18, "AD_Timed_Result_Hi" },

 { 0,  0x8f, 0x10,  12, "HSO_Intpend1" },
 { 0,  0x87, 0x11,  11, "BANK_Select"  },         //mem expand only

 { 0,     3, 0x11,   9, "Data_Bank"    },  // mem expand only
 { 4,     7, 0x11,  10, "Stack_Bank"   },

 { 0,  0x8f, 0x12,  12, "HSO_IntMask1" },
 { 0,  0x87, 0x13,  11, "IO_Timer_Hi"  },
 { 0,  0x8f, 0x14,  12, "HSO_IntPend2" },
 { 0,  0x87, 0x15,   6, "LSSI_A"       },       // read
 { 0,  0xc7, 0x15,   6, "LSSO_A"       },       // write
 { 0,  0x8f, 0x16,  12, "HSO_IntMask2" },
 { 0,  0x87, 0x17,   6, "LSSI_B"       },
 { 0,  0xc7, 0x17,   6, "LSSO_B"       },       // write
 { 0,  0x8f, 0x18,   9, "HSO_State"    },
 { 0,  0x87, 0x19,   6, "LSSI_C"       },
 { 0,  0xc7, 0x19,   6, "LSSO_C"       },      // write
 { 0,  0x87, 0x1a,   8, "HSI_Mode"     },
 { 0,  0x87, 0x1b,  11, "HSO_UsedCnt"  },
 { 0,  0x8f, 0x1c,  11, "HSO_TimeCnt"  },
 { 0,  0x87, 0x1d,   6, "LSSI_D"       },
 { 0,  0xc7, 0x1d,   6, "LSSO_D"       },       // write
 { 0,  0x87, 0x1e,  12, "HSO_LastSlot" },
 { 0,  0x87, 0x1f,  11, "HSO_SlotSel"  },

 { 0,  0x8f, 0x20,   9,  "StackPtr"     },
 { 0,  0x8f, 0x22,  11,  "AltStackPtr"  }

};



//SIZE for x parameter ??  done in adt

void xplus  (MHLD *);
void xminus (MHLD *);
void xmult  (MHLD *);   //do these make sense for address calcs ?? should they be shifts ??
void xdiv   (MHLD *);
void xdmy   (MHLD *);
void xenc   (MHLD *);

void xpwr   (MHLD *);
void xroot  (MHLD *);
void xsqrt  (MHLD *);
void xoff   (MHLD *);
void xvolts (MHLD *);
void xbnd   (MHLD *);




// number of pars (3 max), func must have brackets  ( + - / *  do not)
// default calctype,       not used


MXF mathc [] = {

{ 0,  0, 0, 0     },             // dummy
{ 1,  0, DP_FLT, 1, xplus  },    // integer
{ 1,  0, DP_FLT, 1, xminus },
{ 1,  0, DP_FLT, 1, xmult  },
{ 1,  0, DP_FLT, 1, xdiv   },
{ 3,  1, DP_DADD, 1, xenc   },      // enc (x,n,b) address,type,base reg   may not need 3 ...
{ 2,  1, DP_FLT, 1, xpwr   },      // pwr(x,n)   float
{ 1,  1, DP_FLT, 1, xroot   },      // root
{ 1,  1, DP_FLT, 1, xsqrt   },      // sqrt

{ 1,  1, DP_FLT, 1, xvolts   },      // a to d (volts)
{ 1,  1, DP_FLT, 1, xbnd   }       // xupb  (upper byte ?)  bnd) ???  bound(x) for funcs (=x/128) or whatever

     // { 1,  1, 0, xdmy }  last entry

};

// xget(DPADD, size); ??
//xfld () ??





/********** COMMAND Option letters -
*
* Global opts
  A   Args mode print (one per line)  default for ARGS and subrs
  C   Compact mode print default for all data structs
  F   special Function (subr)
  Q   Quit, terminator byte(s) 1-3

* Standard, data items

 =   Answer definition ?
 A
 B   Bit symbol (in sym)
 C   Cell  for subword fields [start,width]
 D   Displacement - Data offset
 E   Encoded address
 F   Flags byte/word (symbol)
 G    Gen Type ?? [ text, mS, Volts, A/d Time/mS/scale etc ?] or Hybrid ??
 H
 I   imd name OK for syms
 J
 K   banK  replace bank where applicable
 L   Long
 M         [ mask ? ](byte or word pair) not done yet
 N   Name lookup, find symbol
 O   Count (cols etc)
 P   Minimum Print fieldwidth
 Q
 R   Reference pointer (vector) in struct
 S   Signed
 T   Triple (24 bit)
 U   Unsigned
 V   diVisor - Float value
 W   Word,  Write (symbol)
 X   Print RadiX
 Y   bYte
 Z
 ***************************************************************/

// print processors

uint pp_dflt  (uint, LBK *); uint pp_wdbl  (uint, LBK *); uint pp_text  (uint, LBK *);
uint pp_vect  (uint, LBK *); uint pp_code  (uint, LBK *); uint pp_stct  (uint, LBK *);
uint pp_timer (uint, LBK *); uint pp_dmy   (uint, LBK *);

// command processors

uint set_vect (CPS*); uint set_code (CPS*); uint set_scan (CPS*); uint set_cdih (CPS*);
uint set_args (CPS*); uint set_data (CPS*); uint set_rbas (CPS*); uint set_sym  (CPS*);
uint set_subr (CPS*); uint set_bnk  (CPS*); uint set_time (CPS*); uint set_tab  (CPS*);
uint set_func (CPS*); uint set_stct (CPS*);
uint set_psw  (CPS*);




// what about decimal versus hex prints for each data type ??? (too much ?)
// other default layouts and types DONE A & C
// symbol types and print options ?




// Params -
// command processor set subroutine, command print subroutine,
// min pars, max pars,  4 param types (0 - none, 1 start addr, 2 end addr, 3 register, 4 st range, 5 end range,   )
// compact args, compact data  (if options affect this command),
// name allowed (1), expected(2),
// <gap> max addnl levels, min addnl levels, default size,
// <gap>  merge allowed, string opt type,
// <gap>  global options (scanf), data options string (scanf), suboptions


//NB cannot have 1 and 2 for DATA commands, as RZAS series has high ram, outside VAL_ROM constraints.

 // 0 none, 1 start address, 2 end address (same bank) 3 register,
 // 4 start address (no ROM valid) 5 end address (no ROM valid)
  // 6 range start 7 range end (can cross banks)

//allow an R to print end value ??

DIRS dirs[23] = {

// these go in cmd chain
{ set_data, pp_dflt,  2, 2,  4,5,0,0,  0,0,  0,    0,  0,  1, 1,   1, 0,   0,           0 ,           0       },                 // fill  (default) MUST be entry zero
{ set_data, pp_wdbl,  1, 2,  4,5,0,0,  0,0,  1,    0,  1,  1, 1,   1, 0,   0,           "PSUXV=",     "BUS="  },              // byte
{ set_data, pp_wdbl,  1, 2,  4,5,0,0,  0,0,  1,    0,  1,  2, 1,   1, 0,   0,           "KPRSUXV/*",  "BUSY=" },          // word (can be ptr)
{ set_data, pp_wdbl,  1, 2,  4,5,0,0,  0,0,  1,    0,  1,  3, 1,   0, 0,   0,           "PSUXV/"   ,  "BUSY=" },             // triple
{ set_data, pp_wdbl,  1, 2,  4,5,0,0,  0,0,  1,    0,  1,  4, 1,   0, 0,   0,           "PSUXV/*"  ,  "BUSY=" },            // long

{ set_data, pp_text,  2, 2,  4,5,0,0,  0,0,  0,    0,  0,  1, 1,  1, 0,   0,           0,  0} ,                  // text
{ set_vect, pp_vect,  1, 2,  1,2,0,0,  0,0,  1,    0,  1,  2, 9,  0, 0,   0,           "DKQ",   "D" },                 // vect "R" assumed (allow sub offsets)
{ set_tab,  pp_stct,  2, 2,  4,5,0,0,  0,1,  1,    1,  1,  0, 4,  0, 0,   "AC",        "OPSUVWXY/*=", 0 },         // table+ word
{ set_func, pp_stct,  2, 2,  4,5,0,0,  0,1,  1,    1,  2,  0, 4,  0, 0,   "AC",        "LPSUVWXY/*=", 0 } ,        // func
{ set_stct, pp_stct,  2, 2,  4,5,0,0,  0,1,  1,    1, 15,  0, 1,  0, 0,   "QAC",       "DELKNOPRSUVWXY=|/*", "NBUSY=" },  // struct

{ set_time, pp_timer, 2, 2,  4,5,0,0,  0,1,  1,    0,  2,  0, 1,  0, 0,   "AC",        "NTUWY" , 0},              // timer
{ set_code, pp_code,  2, 2,  1,2,0,0,  0,0,  0,    0,  0,  0, 8,  1, 0,   0,           0 , 0 } ,                  // code
//these go in aux chain
{ set_args, pp_dflt,  1, 2,  1,2,0,0,  1,0,  0,    1, 15,  0, 1,  0, 0,   0,           "DELKNOPRSUVWXY/|*" , 0},    // args
{ set_cdih, pp_dmy,   2, 2,  4,5,0,0,  0,0,  0,    0,  0,  0, 0,  1, 0,   0,           0  } ,                    // xcode
{ set_subr, pp_dmy,   1, 1,  1,0,0,0,  1,0,  1,    0, 15,  0, 1,  0, 0,   "ACF",       "DELKNOPRSUVWXY/|=", 0 },   // subr

{ set_scan, pp_dmy ,  1, 1,  1,0,0,0,  0,0,  0,    0,  0,  0, 0,  0, 0,   0,           0 , 0  },                   // scan
{ set_rbas, pp_dmy,   2, 4,  3,4,6,7,  0,0,  0,    0,  0,  0, 0,  0, 0,   0,           0 , 0  },                   // rbase

{ set_sym,  pp_dmy,   1, 3,  4,6,7,0,  0,0,  2,    0,  1,  0, 0,  0, 0,   0,           "BFITWNZ" , 0   },       // sym
{ set_bnk,  pp_dmy,   2, 4,  0,0,0,0,  0,0,  0,    0,  0,  0, 0,  0, 0,   0,           0 , 0},                  // bank

{       0,  pp_dmy,   0, 0,  0,0,0,0,  0,0,  0,    0,  0,  0, 0,  0, 1,   0,           0, 0 },                  // clear options
{       0,  pp_dmy,   0, 0,  1,0,0,0,  0,0,  0,    0,  0,  0, 0,  0, 1,   0,           0, 0 },                  // set options

{ set_psw,  pp_dmy,   2, 2,  1,1,0,0,  0,0,  0,    0,  0,  0, 0,  0, 0,   0,           0, 0 },                  // psw setter for '0=0' jumps

{       0,  pp_dmy,   0, 0,  0,0,0,0,  1,0,  2,    0,  0,  0, 0,  0, 2,   0,           0, 0 }                  // set math formula 'calc' (name =)

};




HDATA* get_flhdr(void)
{
    return  &flhdr;          // single struct !!
}

FDATA* get_fldata(uint ix)
{
    return  fldat+ix;          // single struct !!
}



CSINX *get_strinx(uint ix)
{

//command strings, minimum match size, then char string


if (ix >= NC(strindex)) return NULL;

return  strindex+ix;


}

CSTR *get_cstr (uint s, uint ix)
{
 CSINX *x;
 CSTR *str;
 x = get_strinx(s);

 if (ix > x->size) return NULL;

 str = x->strlist;

 return str+ ix;

}

uint get_cmdopt(uint x)
{
  CSTR *s;
  s = get_cstr(OPTSTR,x);
  if (!s) return 0;
  return s->par1;
}


//used in core.cpp to set 8065 status
void set_cmdopt(uint x, uint state)
{
 CSTR *s;
  s = get_cstr(OPTSTR,x);
  if (!s) return;
  s->par1 = state;

}


DIRS *get_dirs(uint ix)
{
  return dirs + ix;
}


MXF * get_mathfunc(uint ix)
{
    if (ix < NC(mathc)) return mathc + ix;
    return NULL;
}

uint get_defcalctype(uint ix)
{
  if (ix < NC(mathc)) return mathc[ix].defctype;
    return 0;
}





uint valid_bank(uint bk)
{
 //for bank number
  BANK *b;

  if (bk > 16) return 0;

  b = get_bankmap(bk);
  if (b->bok) return 1;
  return 0;
}








int do_error(CPS *c, uint err, cchar *fmt, ...)
  {
    // cmd string in flbuf....
    // use vfprintf directly as it does not work
    // if called via wnprt

   va_list args;
   char *t;


 if (!c->firsterr)
     {
      t = flbuf;
      c->cmpos +=2;
      if (c->cmpos > c->cmend) c->cmpos = c->cmend;

      while (t < c->cmpos) pchar(MSGFILE, *t++);     //    fputc(*t++, msgfl->fh);

      c->firsterr = 1;
     }

   if (err)
     {
       p_pad(MSGFILE, 50);
       xprt(MSGFILE, 0, " << %s",htxt[err]);
     }

   if (fmt)
    {
     va_start (args, fmt);
     xprt(MSGFILE,0, fmt, args);
     va_end (args);
     pchar(MSGFILE,'\n');
    }


   if (err == 1)  c->error = err;
   return err;
  }



int do_ch_error(CPS *c, uint ch)
{
CHAIN *x;
x = get_chain(ch);

return do_error(c,etxts[x->lasterr].err, etxts[x->lasterr].txt);

}



void csprt (cchar *fmt, ...)
{  // appends to to tempstr and keeps track of strlen
   //for auto numbered symbols mainly

  va_list args;
  va_start (args, fmt);
  tempstr.len += vsprintf(nm+tempstr.len, fmt, args);
  va_end (args);
}



void add_autosym(uint ix, uint addr)
{

// add auto sym with address
// all of these are whole read symbols
// add SYSTEM and WHOLE flags

  int  opts;

  if (get_anlpass() >= ANLPRT) return; // 0;

  opts = (C_SYS | C_NOBIT) ;

  if (ix & C_USER)     opts |= C_USER;
  if (ix & C_RENAME)   opts |= C_RENAME;
  if (ix & C_TEST)     opts |= C_TEST;

  ix &= 0x7f;
  CSTR *a;

  a = get_cstr(AUTONAMES,ix);

  if (!a) return ; //NULL;              // safety check

  if (!get_cmdopt(a->par1)) return; // NULL;  // autoname option not set

  // make name first

  tempstr.len =  0;
  csprt("%s", a->string);
  // add address
  csprt("_");
  if (get_numbanks())  csprt( "%x", (addr >> 16) -1);      // bank
  csprt("%04x", nobank(addr));                     //address

  add_sym(&tempstr, addr,0, opts|7, 0,0xfffff);     // add new (whole,read) symbol

}


int isdelpunc (char *c)
{
  if (*c == ' ')  return 1;
  if (*c == ',')  return 1;
  if (*c == '\t') return 1;

  if (*c == ']')  return 2;
  if (*c == '[')  return 2;
  if (*c == '(')  return 2;
  if (*c == '(')  return 2;

  // {} too ?
  return 0;
}





int readpunc(CPS *c)
 {
  // read punctuation between commands and numbers etc.
  // keeps track of position (for errors)

    while (c->cmpos <= c->cmend)
     {
       if (isdelpunc(c->cmpos) == 1) c->cmpos++;
       else break;
     }

  if (c->cmpos >= c->cmend) return 0;            // hit end of line

  return 1;
 }




int get_delim_char (CPS *c)    //, int *type)
{
  // get next char and skip, but keep count of delimiters/brackets

  int ans;

  readpunc(c);

//  if (type) *type = -1;
  if (c->cmpos >= c->cmend) return -1;  // beyond end ??

  ans = *c->cmpos;

   switch(ans)
    {
      case '[' :        //0x5b
         c->sqcnt++;    //track square bracket counts....
   //      if (type) *type = 1;
         break;

      case ']' :        //0x5d
         c->sqcnt--;    //track square bracket counts....
  // if (type)       *type = 2;
          break;
      case '(' :          // 0x28
         c->rbcnt++;    //track round bracket counts....
 // if (type)                 *type = 3;
         break;

      case ')' :        // 0x29
         c->rbcnt--;    //track round bracket counts....
 //  if (type)                *type = 4;
         break;

      default:         // not valid

 //   if (type)      *type = 0;
        ans = -1;
         break;
    }                 // end switch

  if (ans > 0) c->cmpos++;        // and skip valid char at c->compos
  return ans;

}


uint match_str(CPS *c, cchar* tst)
{

 // check string against string tst
 //strncmp but returns how many chars matched and stops
 // at delimters and end.

  uint len;
  char *t;

  len = 0;
  t = c->cmpos;

  while (t < c->cmend)
     {
      if (isdelpunc(t)) break;               // hit whitespace or delim
      if (toupper(*t) != toupper(*tst)) break;

      tst++;
      t++;
      len++;
     }

 return len;


}
/* mystrcmp: return <0 if s <t , 0 if s==t, > 0 if s > t /
// lifted for mods

// change to match only up to next delim.....

int mystrncmp(char *lhs, char *rhs, int n) {
    for (; *lhs == *rhs; lhs++, rhs++)
        if (*lhs == '\0' || --n <= 0)
            return 0;
    return *lhs - *rhs;

}

int strncmp(char *s, char *t, int n)
{

     if(strlen(s) == strlen(t)) {

         while(*s == *t && *s && n)
            n--, s++, t++;

         if(!n)
             return 0; // same length and same characters
         else
             return 1; // same length, doesnt has the same characters
     }
     else
        return strlen(s) - strlen(t);


int mystrnlen(char *s) {
    char *p = s;
    while (*s != '\0') {
        s = s + 1;
    }
    return s - p;
}


uint sizestr(CPS *c)
{
  //strlen but stops at delimiters or whitespace

  uint len;
  char *t;
  len = 0;

  t = c->cmpos;
  while (t < c->cmend)
     {
      if (isdelpunc(t)) break;               // hit whitespace or delim
  //    if (ismydelim(t)) break;              // hit delimiter
      len++;
      t++;
     }
 return len;
}
*/






int get_optchar(CPS *c, cchar *lt)
{
   // check delimiter option letters against supplied array (of letters).

   int ans;
   char t;

   readpunc(c);

   if (c->cmpos >= c->cmend) return -2;      //end of line

   ans = get_delim_char(c);

   if (ans > 0) return ans;    //include delims as option char (for nesting)

   if (!lt) return -1;    // zero opt list (safety) zero is valid answer

   ans = -1;

   t = toupper(*c->cmpos);     // where we are in cmd string

   while (*lt)
     {
      if (t == *lt)
        {
         ans = t; // matched an option from list
         break;
        }
      lt++;
     }

   if (ans > 0) c->cmpos++;            // and skip the char in cmd string
  return ans;
}



int getpx(CPS *c, int ix, int num)
 {
  /* ix is where to start in p params array, max 8,
  *  and set flags companion
  *  for max of 8-ix parameters
  *  answer is num params read
  * params arry in CPS struct =
  * int   p [8];        params
  * uint  pf[8];        length, flags
#define HXRL      0x3ff       max read length mask
#define HXLZ      0x400       leading zero
#define HXNG      0x800       negative
*/

  int ans, rlen;
  uint n;

  ans = 0;

  num += (ix -1);  //max index used
  num &= 7;

  while (ix <= num)
   {
     // allow a '-' in 4 char hexes, for offsets
     // allow leading zero for bank
     readpunc(c);
     if (sscanf(c->cmpos, "%5x%n", &n, &rlen) <= 0) break;   // 5 chars max
     c->p[ix]  = n;
     c->pf[ix] = rlen & HXRLM;    // maxlen safety mask
     if (*c->cmpos == '0') c->pf[ix] |= HXLZ;      // leading zero
     if (*c->cmpos == '-') c->pf[ix] |= HXNG;      // leading -ve
     c->cmpos += rlen;
     ans++;
     ix++;
   }

  return ans;
 }






int getpd(CPS *c, int ix, int limit)
 {
  // ix is where to start in p params array, 2 max decimal pars
  // answer is params read - only ever 1 param ??
   int ans, rlen;

  ans = 0;

  limit += (ix -1);
  limit &= 7;

  while(ix <= limit)
   {
     readpunc(c);
     if (sscanf(c->cmpos, "%d%n", c->p+ix, &rlen) <= 0) break;
     c->cmpos += rlen;
     c->pf[ix] = rlen;
     ans++;
    ix++;
   }

  return ans;
 }




int get_float(CPS *c, float *num)
 {
  // ix is where to start in p params array, 2 max decimal pars
  // answer is params read - only ever 1 param at a time

 // isnan()

   int rlen, ans;

   *num = NAN;

  //now read number

   if (c->cmpos > c->cmend) return -1;

   ans = sscanf(c->cmpos,"%f%n", num, &rlen);
   if (ans > 0)
     {
       c->cmpos += rlen;
       readpunc(c);
     }

/*One way to check for NaN would be:

#include <math.h>
if (isnan(a)) { ... }

You can also do: a != a to test if a is NaN.

There is also isfinite(), isinf(), isnormal(), and signbit() macros in math.h in C99.

C99 also has nan functions:

#include <math.h>
double nan(const char *tagp);
float nanf(const char *tagp);
long double nanl(const char *tagp);  */

// INF  check ?
   return ans;
 }


//makes sense to have a special for quoted strings, but command strings cannot have delimiters.







int get_cmdstr(CPS *c, uint ix)
{
  // match test str (c->cmpos) to any preset cmd list set ix as ans and return CSTR ??

  uint j, tlen;
  int ans;

  CSINX *x;
  CSTR *s;

 readpunc(c);
 if (c->cmpos >= c->cmend) return -1;

  x = get_strinx(ix);          // need no of strings

  if (!x) return -1;

  ans = -1;

 // tlen = sizestr(c);          // max match. up to next whitespace or delim (why ??)

  for (j = 0; j < x->size; j++)
   {
    s = x->strlist+j;                    // map to correct entry

    tlen = match_str(c,s->string);         // number chars matched

    if (tlen >= s->par3 &&  tlen <= s->len)
       {
           // match between min and max OK
           ans = j;
           c->cmpos += tlen;
           break;
       }
   }

  if (x->delim && !isdelpunc(c->cmpos) ) ans = -1;     //invalid if not delim or whitespace
 // not true for calcs !!

  return ans;
}




int get_qstring(CPS *c, uint ixm)
{
   // return quoted string as terminated string for a name.
   // must set if name must be there (calc name).
   // ixm is 0 or 1, plus 4 for must have name


  uint rlen;
  CSTR *str;

  readpunc(c);

  if (c->cmpos > c->cmend ) return -1;

  if (*c->cmpos != '\"')
     {
       if (ixm & 4) return do_error(c, 1, "must have name");
       else return 0;
     }

  str = c->names + (ixm & 1);
  rlen = 0;

//scanf won't do whitespaces.

  str->len = 0;      // string length;
  c->cmpos++;         //skip first quote
  str->string = c->cmpos;            // [ixm & 1] = c->cmpos;

  while (c->cmpos < c->cmend)
     {
      if (rlen >= SYMSZE) break;
      if (*c->cmpos == '\"') break;        // end of name

 //  if (*c->cmpos == '\ ') *c->cmpos = '\_' ;        // replace spaces with underscore
      c->cmpos++;
      rlen++;
     }

  if (rlen >= SYMSZE)       return do_error(c, 1, "name too large");
  if (c->cmpos >= c->cmend) return do_error(c, 1, "missing close quote");    //may allow ??

  str->len = rlen;                 // set string length without terminator
  c->cmpos++;                           //skip close quote

/* if (c->fcom == C_CALC)
   {
     if (match_str(c, CALCSTR) > 0)
        {
          return do_error(c, 1,"cannot use a calc type as a name");
    //      return 0;
        }

     readpunc(c);
     if (*c->cmpos != '=') return do_error(c, 1,"calc must start with = ");     //not in here !!!

     c->cmpos++;
}
*/

 if (ixm & 4 && !str->len) return do_error(c, 1,"No name specified");


return 0;
}



/*

int read_calctype(CPS *c)
{
 int ans;
    // name read in parse cmd

 ans = get_cmdstr(c, CALCSTR);

 //neat, but isn't names+2 used for calc name ??
 // if (ans > 0) memcpy(c->names+2, get_cstr(CALCSTR,ans), sizeof(CSTR));

 readpunc(c);

 if (c->cmpos > c->cmend) return do_error(c, 1, "Line incomplete");

return ans;
}
*/


uint do_math_term(CPS *c, void *fid, int pix)
{
  int opt, cmdread, bktlev;
  char *lastpos;
  float fx;
  MATHX *a;
  MNUM *n;
  MXF *mref;       // reference for func details

/* fid starts out as name reference, or ADT if no name

 *
 * each term is func (par1, par2, par3).  Read func (name) OR float OR ref each time,
 * and if it's first term in this level and a number first, add a dummy '+' func to start.
 *
 * each parameter has a type, int/float etc, but also includes SUB and a REF so that a
 * subterm can be hung anywhere there is a parameter
 *
 * to do.... SET flag somewhere in C to mark as single term only for short formula - and terminate on end ')'
 */

  readpunc(c);

  a = 0;
  mref = 0;

  bktlev = c->rbcnt;              //round bracket count

  while (c->cmpos < c->cmend)
    {

     cmdread = 0;              // read type
     readpunc(c);

     if (c->cmpos > c->cmend) break;         //  end of cmd line
     lastpos = c->cmpos;                     //  to check something has been read

    // attempt to read a number, reference, function or special char (e.g. open bracket)

    // should always be a func first, then pars/numbers expected.
    // special if first term is number, add an implied '+' to make func, par -> func, par(s) etc.

    //TO DO - if func is + or - and func already set, assume attached to number.....
    // also no space after the -/+

    // recursive for bracketed subterms (treated as subs of params)

    // have 'expected' type to define what's next ??  maybe.


//bracket first ??

     c->mfunc = get_cmdstr(c, MATHSTR);

// if (cmfunc = -1 && a && a->npars??  must have function ?
     if (c->mfunc > 0)
       {
         cmdread = 4;               // valid command read
         a = (MATHX *) chimem(CHMATHX);
         a->fid = fid;    //parent
         a->pix = pix;    // and param index

                                                                        //  a = add_mterm(fid,pix);    // this is new function - always add a new term
         fid = a;                   // new term is now fid
         if (!pix && !c->mcnt) {a->mfirst = 1; c->mterm = a;}
         pix = 0;                   // and param is now zero
         c->mcnt++;
         a->func = c->mfunc;
         mref = get_mathfunc(a->func);       // func details
         a->fbkt = mref->bkt;          // and mark in ADT
         a->calctype = c->mcalctype;  // master calc type

         if (mref->bkt)        // skipobkt = 1;   // set marker to skip one bracket.....
           {    // skip one bracket (for function) mark close
            opt = get_optchar(c,0);      //read next
            if (opt != '(' ) return do_error(c, 1," '(' expected");
           }
       }
       else
       {
          // c->mfunc = 0;     redundant     // 1?        //add
          //cmdread = 4 ???
          if (!a)
            {
             // this is first term of a new level and number read in to start, not func
             // so create a dummy '+' function
             a = (MATHX *) chimem(CHMATHX);
             a->fid = fid;    //parent
             a->pix = pix;    // and param index                 //a = add_mterm(fid,pix);
             fid = a;
             if (!pix && !c->mcnt) {a->mfirst = 1; c->mterm = a;}              // first term of whole math sequence
             pix = 0;
             c->mcnt++;
             a->func = 1;
             mref = get_mathfunc(a->func);     // func details
             a->lfirst = 1;                    // flag as first local to stop print of '+'
             a->calctype = c->mcalctype;       //mr->defctype;   //not always !!


            }

            // if (a)  then error ?       cmdread ??


       }


     opt = get_optchar(c,0);

     if (opt == '(')       // open bkt flag  //
       {       // nested bkt '('  c->rbcnt incremented;
         n = a->fpar + a->npars;
         n->dptype = DP_SUB;                 //subterm
         a->npars++;
         do_math_term (c,fid, a->npars);     // recursive for subterm(s)
         continue;                           // this seems to work....??
       }

   // return if all bkts (for this term level) closed  (empty bracket not legal ?
     if (opt == ')' && c->rbcnt < bktlev) return 0;

     readpunc(c);

     //here maybe, a func must have been read?

     //this next bit is 'get_number?"

     if (toupper(*c->cmpos) == 'X')
       {  // a reference - links to relevant ADT entry, including a subfield....
   //      opt = get_ref(c);
      //   if (opt)
          {
            c->p[5] = 1;           //opt;         // ref num may be redundant.
            cmdread = 2;            //reference
      //      n->dptype = DPREF;    // see BELOW....
            c->cmpos++;  //skip X (no numbers for now)
          }
       }
     else
   //  if (isdigit(*c->cmpos))      //can just call get_float ?? Yes.
       {

         //calctype must define whether float int or adress - c->mastercalc or mr->defctype (float or add) ??
        opt = 0;

        if (mref && c->mcalctype <= DP_DADD)         // mr->defctype == DPADD)
        {
          opt = getpx(c,5,1);
          if (opt > 0) cmdread = 3;     //hex (address) OK
        }

        if (!opt)          //float is default
        {
          opt = get_float(c,&fx);    //  term must begin null func with number...........THEN func/opt and number

          if (opt > 0)
           {
             if (!isnormal(fx)) {do_error(c, 1, "Invalid floating point number"); return 0;}
             cmdread = 1;            // float OK
           }
        }



       }


     if (cmdread > 0 && cmdread < 4)
        { // number or ref read in

          n = a->fpar + a->npars;

          if (cmdread == 1) {n->fval = fx; n->dptype = DP_FLT;}     //float - no not always............

          if (cmdread == 2) n->dptype = DP_REF;              //n->refno = c->p[0]; if numbers with X
          if (cmdread == 3) {n->ival = c->p[5]; n->dptype = DP_HEX;}     //float - no not always............

          a->npars ++;
        }


     if (a) mref = mathc + a->func;      // func lookup details - set here in case of auto func



     if (c->error) break;
     if (lastpos == c->cmpos) do_error(c, 1, "Parse fail");    // catchall safety check - nothing has been read

    }  // end while chars to read

// now insert into chain, after fixing up fid (chksum)

// add_mterms(c->mterm); move outside due to recursive.
//then copy local term to a valid structure............

  return 0;
}




uint fix_input_addr_bank(CPS *c, int ix)
 {
     // answers 0 is OK
     // 1 is invalid bank

   uint rlen, bk, addr;


   rlen = c->pf[ix] & HXRLM;    // read length
   bk = g_bank  (c->p[ix]);       // 0 - 15 in 0xf0000
   addr = nobank(c->p[ix]);     // address part

   if (valid_reg(addr))
     {
       if (bk) return 1;       // registers don't ever have a bank
       return 0;               // register valid
     }


   if (!get_numbanks())
     {                    //single bank
       if (rlen > 4 && bk != 0x80000 ) return 1;      // only bank 8 allowed
       bk = 0x90000;                                  //force bank 8
     }
   else
     {          // multibank
  //     BASP *bpars;
   //    bpars = get_basepars();
   //    get_basepars(&bk,0);        //bk = basepars.datbnk;   // single bank force 8 (9)

       if (bk & 0x60000) return 1;       // invalid bank

       if (rlen < 5)
         {
          if (addr > 0x1fff) return 1;    // 0x2000 on MUST have a bank.
          bk = 0x20000;                   // set bank 1 for RAM
         }
       else
         {
          bk += 0x10000;                   // add one to valid bank
         }
     }
   c->p[ix] = addr | bk;

   //check address is valid in bank ??


   return 0;
 }


MATHN *add_sys_mname(void *fid, uint ix, uint num)
{
  CSTR *name;

  name = get_cstr(AUTONAMES,ix);

  tempstr.len = 0;
  csprt (name->string);
  csprt("%u", num);

  return add_mname(fid,&tempstr);
}


MATHX *add_encode(uint chn, void *fid, uint type, uint reg)
{
  // add encode type to adt cell.
  // defined as func as used in core by argument code
  //names for diplay, so return a mathx for linking.

  //add a dumy name ???   may be easier....

  MATHX *x;
  MATHN *n;


  x = (MATHX*) chimem(CHMATHX);          // add_mterm (fid,0);
  if (!x) return NULL;

  x->calctype = DP_DADD;
  x->mfirst = 1;
  x->func = 5;
  x->fbkt = 1;
  x->npars = 3;

  x->fpar[0].dptype = DP_REF;
  x->fpar[1].ival = type;    //enc type
  x->fpar[1].dptype = DP_DEC;
  x->fpar[2].ival = reg;    //register address
  x->fpar[2].dptype = DP_DADD;

  add_mterms(x);
  add_link(chn,fid,CHMATHX,x,1);                // adt-> mterm

  n = add_sys_mname(x,20, type);        // 20 = encode + num, link to calc

  if (n)
    {
      n->sys = 1;
      add_link(CHMATHN, n,CHMATHX,x,1);           // name -> mterm
      add_link(CHMATHX,x,CHMATHN,n,1);            // mterm -> mname
    }
  return x;
}


MATHX *add_mcalc(ADT *a, uint func, uint par1)
{
    //to add single math term, e.g. divide
    //this for D (offsets) must modify for V

  MATHX *x,*y;
//  MATHN *n;

 // n = add_mname(a,0);
// n = (MATHN*) 1234;

//  n->mcalctype = DP_DEC;
//  add_link(CHADNL,a,CHMATHN,n,1);                // mterm -> adt (fwds)

  x = (MATHX*) chimem(CHMATHX);            //add_mterm (n,0);

  if (!x)  return NULL;

  // first term. add the 'x' reference
  x->mfirst = 1;
  x->func = 1;
  x->npars = 1;
  x->fpar[0].dptype = DP_REF;
  x->calctype = DP_DEC;
  x->lfirst = 1;
 // x->noname = 1;
 // add_link(CHADNL,a,CHMATHX,x,1);               // mterm -> adt (fwds)

  // 2nd term

  y = (MATHX*) chimem(CHMATHX);  //x = add_mterm (x,0);              // 2nd -> 1st term

  y->fid = x;
  y->calctype = DP_DEC;           //default
  y->func = func;
 // x->calc = &xplus;
  y->npars = 1;
  y->fpar[0].ival = par1;    //offset value
  y->fpar[0].dptype = DP_DEC;

add_mterms(x);

  return x;
 }






int do_math_str ( CPS *c, void *fid)
{
   MATHN *n;
 //  CSTR  *name;
   uint ix;

   //called from an '=' (fid = adt) or a 'calc' command (fid = 0)

  // adds a named calc - name is in c->string if fid = cmnd.
  // for both named and unnamed, should be at first '('
  // named via 'calc' command, unnamed via embedded '=' in an ADT

  // need to consider SUFFIX string as well....e.g. volts, RPM, degrees, percent

   readpunc(c);

   if (c->cmpos > c->cmend) return do_error(c, 1, "Line incomplete");

// readpunc(c);

//no name - calctype in c command struct

 ix = get_optchar(c,0);

 if (ix != '(') do_error(c, 1, "Calc must begin with '('");

 do_math_term(c,0,0);              // fid = currently linked to name.....

 //error check here ??

 add_mterms(c->mterm);        //add calc terms to adt

 if (!fid )
      {     // calc command, not inline    name in c->names, first math term in c->mterm

        n = add_mname(c->mterm, c->names);                  // first name from cmd

        //add name <-> term BOTH ways to be able to go from name or calc

        add_link(CHMATHN,n,CHMATHX,c->mterm,1);     // add name to calc
        add_link(CHMATHX,c->mterm,CHMATHN,n,1);     // add calc to name

        // mterm <-> adt
// must link to calc to allow fid to be chksum.


   //     name = get_cstr(CALCSTR,c->ansreg);
   //     c->mcalctype = c->ansreg;     //name->par2;                     // set master calctype for following terms
   //     fid = n;
      }









return 0;

}



uint validate_input_addr(CPS *c, int ix, uint type)
{
  // force single bank addrs to bank 9.

  // 0 none, 1 start address, 2 end address (same bank) 3 register,
 // 4 start address (no ROM valid) 5 end address (no ROM valid)
  // 6 range start 7 range end (can cross banks)

  switch(type)

  {
    default:           //do nothing
      break;

      case 2:       // end address within valid ROM

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRLM) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c, ix))    return do_error(c,1, "Invalid bank");;

             if (!val_rom_addr(c->p[ix]))       return do_error(c,1,"Invalid address");
            }
         // extra end validation against start

           if (!bankeq(c->p[ix-1], c->p[ix]))   return do_error(c,1, "Banks must match");
           if (c->p[ix] < c->p[ix-1])           return do_error(c,1, "End is less than Start");

           break;
          }
       // else fall through as a type 1

      case 1:       // start address within valid ROM

         if (get_numbanks() < 0)              return do_error(c,1, "No banks defined");    //but what if bank command ???
         if (fix_input_addr_bank(c,ix)) return do_error(c,1, "Invalid bank");
         if (!val_rom_addr(c->p[ix]))   return do_error(c,1, "Invalid address");
         break;



      case 3:       //register

        c->p[ix] = nobank(c->p[ix]);
        if (!valid_reg(c->p[ix]))       return do_error(c,1, "Invalid Register ");
        break;

     case 5:       // end address (0-0xffff) without val_rom

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRLM) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c, ix))    return do_error(c,1, "Invalid bank");;
            }
         // extra end validation against start

           if (!bankeq(c->p[ix-1], c->p[ix]))   return do_error(c,1, "Banks must match");
           if (c->p[ix] < c->p[ix-1])           return do_error(c,1, "End is less than Start");

           break;
          }
       // else fall through as a type 4


   case 7:       // full range end address across banks

         if (ix)
          {

           if (!c->p[ix]) c->p[ix] = c->p[ix-1];
           else
            {
             if ((c->pf[ix] & HXRLM) < 5) c->p[ix] |= g_bank(c->p[ix-1]);
             else if (fix_input_addr_bank(c, ix))    return do_error(c,1, "Invalid bank");;
            }

           if (c->p[ix] < c->p[ix-1])           return do_error(c,1, "End is less than Start");

           break;
          }
       // else fall through as a type 6

      case 4:       // start address (0-0xffff) without val_rom
      case 6:       // full range start address (same as 4)

         if (get_numbanks() < 0)              return do_error(c,1, "No banks defined");    //but what if bank command ???
         if (fix_input_addr_bank(c,ix)) return do_error(c,1, "Invalid bank");
         break;



  }
return 0;
}


uint set_rbas (CPS *c)
{
 RBT *x;

//addresses checked already in parse cmd

  if (c->npars < 3)
   {
    c->p[2] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[3] = 0xfffff;   // max possible address
   }

 x = add_rbase(c->p[0], c->p[1], c->p[2], c->p[3]);

 // if (chbase.lasterr) return do_ch_error(c, &chbase);

  if (get_lasterr(CHBASE)) return do_ch_error(c, CHBASE);

 x->usrcmd = 1;              // by command
 return 0;
}


/*
uint set_opts (CPS *c)
{
  //opts in p[0]

  set_cmdopt(c->p[0], 1);

  if (get_numbanks())  set_cmdopt(OPT8065,1);

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"SetOpts = ");
    prtcmdopts(DBGFILE,1);
    DBGPRT(2,0);
  #endif
return 0;

}

uint clr_opts (CPS *c)
{
    set_cmdopt(c->p[0], 0);       //cmdopts &= (~c->p[0]);

  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(0,"ClrOpts = ");
    prtcmdopts(DBGFILE,0);
    DBGPRT(2,0);
  #endif
return 0;

}
*/



int chk_csize(CPS *c)
 {
  int bsize, adtsize, row, bytes;
  DIRS *d;

 // if (!c->fcom) return 0;       // not for fill cmd

  d = dirs + c->fcom;
 // check addnl levels allowed

  bsize = c->p[1] - c->p[0]+(1-c->term);    // byte size of cmd entry in total

  if (d->defsze)  adtsize = d->defsze; else adtsize = c->adtsize;

  if (!adtsize)
    {
      do_error(c,1, "Row size is ZERO !!");
      adtsize = 1;
     }

   if (adtsize > bsize)
     {
      if (c->p[0] == c->p[1] || d->defsze)
        {  // if end = start, or preset sizes, allow change
         c->p[1] = c->p[0] + adtsize-1;
         bsize = adtsize;
         do_error(c,2, "Start->End address too small, end set to ");
         paddr(MSGFILE,c->p[1],0);
        }
   //      else  return do_error(c,1, "End address too low for start+data definition");
     }

   row = bsize/adtsize;                  // number of whole rows
   bytes = row * adtsize;                // number of bytes for whole rows

   if (bytes != bsize)
     {

// if close to right ??
       bytes += c->p[0] + c->term - 1;
       c->p[1] = bytes;

       // check y against end ?

       do_error(c,2, "End inconsistent with size, set to to %x", bytes);

     }

 /* if (c->adtsize)
    {       // if additional data v5
      int row, newbsize;

      row = bsize/c->adtsize;           // number of whole rows
      newbsize  = row * c->adtsize;     // number of bytes for whole rows

      if (newbsize != bsize)
        {
            //not whole rows.
          c->p[1] = c->p[0] + newbsize -1;
          c->p[1] += c->term;
          do_error(c, 2,2, "End inconsistent with size, reset to ");
          prtaddr(c->p[1],0,wnprt);
       }
    } */
   return 0;
 }


uint set_layout(CPS *c, uint fcom)
{
  DIRS *d;
//for setting cptl (1 compact, 0 extended)

  if (c->extcpt)
    {    //overrides options     uint  extcpt   : 2;       // use extended (1) or compact (2) layout
      if (c->extcpt & 2) return 1; else return 0;

    }

  //not set use default globals

  d = dirs + (fcom & 31);

  if (d->cmpargs) return get_cmdopt(OPTCMPA);
  if (d->cmpdata) return get_cmdopt(OPTCMPD);
return 0;
}




void cpy_adt(CPS *c, void *newfid)
{
 // copy adt blocks from cmd chain to std chain with new fid

  ADT *a;
  uint ix ;      //, fix;

  if (!newfid) return;                      // must have an fid
 // if (!c->seq) return;                   // no addnl blocks


//  fix = 0;

//  if (c->fcom == C_ARGS)  fix = 1;
//  if (c->fcom == C_SUBR)  fix = 1;
//  if (c->fcom == C_TABLE) fix = 2;
//  if (c->fcom == C_FUNC)  fix = 2;


//fid for a itself doesn't change - conneted stuff OK

   a = get_adt(&cmnd,0);    // get first block (dummy fid)

  if (a)
    {
     ix = get_lastix(CHADNL);         // save index of block
     a->fid = newfid;
     chupdate(CHADNL,ix);             // rechain after new FK



  // set pfw zero for args and subr
  // set print radix to decimal for tab and func

 /* if (fix) {

  while ((a = get_adt(newfid,0)))
    {
      if (a->cnt)
       {          // use cnt as null checker
        if (fix == 1) a->pfw = 0;    // clear all pfw
        if (fix == 2 && a->dptype == 0) a->dptype = DP_DEC;  //set decimal if not set
       }
      newfid = a;
    }
  }*/
     #ifdef XDBGX
    DBGPRT(0,"      with addnl params");
    DBG_adt(a,0);
  #endif
    }
}


uint set_func (CPS *c)
{
  // for functions.  2 levels only

  int val, fend, startval;
  LBK *blk;
  ADT *a, *b;

  //check have at least one data term ??

  // size row vs. start>end check first

  a = get_adt(c,0);

  if (c->adtcnt < 2)
    {   // add another level if only one specified
     b = (ADT*) chimem(CHADNL);      //append_adt(a,0);                    //&chadcm, 0, 256);
     *b = *a;               // copy all data
     b->fid = a;            // but reset seq
  //   c->seq = 2;            // levels = 1;         // up levels
     append_adt(b);        //a,0);                    //&chadcm, 0, 256);
     c->adtsize = totsize(c);
    }

  // c->tsize must be correct first

  if (chk_csize(c)) return 1;

  fend = a->fend;                   //size and sign as specified

  // check start value consistent with sign

  val   = g_val(c->p[0], 0, fend);
  startval = get_startval(fend);              // start value from fend SIGN

  if (val != startval)  // try alternate sign....does not change command though
    {
     fend ^= 32;                             // swop sign flag
     startval = get_startval(fend);          // new start value
     if (val != startval) do_error(c,0,"Function Start value invalid");
     else  do_error(c,0,"First value (%x) indicates wrong sign specified", val);
    }

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USER);  // start,end, cmd. by cmd
  if (get_lasterr(CHCMD)) return  do_ch_error(c, CHCMD);

  cpy_adt(c,vconvi(blk->start));
  blk->size = totsize(vconvi(blk->start));
  blk->usrcmd =1;
//  if (blk->size) blk->adt = 1;
 blk->cptl = set_layout(c,blk->fcom);
  return 0;
}





uint set_tab (CPS *c)
{
   // tables, ONE level only

  LBK *blk;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  blk = add_cmd (c->p[0], c->p[1], c->fcom|C_USER);  // start,end, cmd. by cmd
  if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);


  cpy_adt(c,vconvi(blk->start));
  blk->size = totsize(vconvi(blk->start));
  blk->usrcmd = 1;

  blk->cptl = set_layout(c,blk->fcom);



 return 0;
}


uint set_stct (CPS *c)
{
   // structures

  LBK *blk;
  uint cmd;

  // size row vs. start>end check first
  if (chk_csize(c)) return 1;

  cmd = c->fcom|C_USER;
  if (c->adtsize) cmd |= C_NOMERGE;
//can't merge if c->adtsize...

  blk = add_cmd (c->p[0], c->p[1], cmd);  // start,end, cmd. by cmd
  if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);

  cpy_adt(c,vconvi(blk->start));
  blk->size = totsize(vconvi(blk->start));
  blk->usrcmd =1;
 // if (blk->size) blk->adt = 1;

  blk->term = c->term;                    // terminating byte flag (struct only)
  blk->cptl = set_layout(c,blk->fcom);

 // set_data_vect(blk);   // for Refs out in structs
  return 0;
}





uint set_data (CPS *c)
{
   // change for word, byte, long
   // no addnl is fine

  LBK *blk;
  ADT *a;
  uint cmd;

  if (chk_csize(c)) return 1;


  cmd = c->fcom|C_USER;
  if (c->adtsize) cmd |= C_NOMERGE;

//  can't merge if c->adtsize...cant add size until blk made....

  blk = add_cmd (c->p[0], c->p[1], cmd);  // start,end, cmd. by cmd
   if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);

  //for text and fill, this can return blk = ZERO

  if (!blk) return 0;

  a = c->adnl;

  if (a && bytes(a->fend) < c->fcom)  a->fend = (c->fcom * 8) -1;     // safety for command sizes

  //if (a && a->bsize < c->fcom)  a->bsize = c->fcom;

  cpy_adt(c,vconvi(blk->start));
  blk->size = totsize(vconvi(blk->start));
  blk->usrcmd =1;
 // if (blk->size) blk->adt = 1;
  return 0;
}


uint set_sym (CPS *c)
{
  int fstart,fend;
  ADT *a;        //, *b;
  SYM  *s;

// flagsword.
// allow/inhibit if immediate...

//if (c->debug)
//{
 //   DBGPRT(0,0);

//}

  //check command levels    c->seq
//  if (c->seq > 1) return do_error(c,0,"Only one data level allowed");

   if (c->npars < 3)
   {
    c->p[1] = 0;         // default address range to ALL (zeroes) otherwise done by cmd
    c->p[2] = 0xfffff;   // max possible address
   }


    a = get_adt(c,0);
    if (a)    // size specified
     {
      fstart = a->fstart;
      fend   = a->fend;
     }
    else
     {
       fstart = 0;
       fend   = 7;
     }

//if (c->write)    fend |= C_WRITE;      // set write in fend...
if (!c->bitfld)  fend |= C_NOBIT;        //safety

//can't have YTL sizes in cmd, W for write

  s = add_sym(c->names, c->p[0], fstart, fend|C_USER|C_RENAME, c->p[1], c->p[2]);

 if (get_lasterr(CHSYM)) return do_ch_error(c,CHSYM);

 // if (c->flags) s->flags = 1;          // with flags (not for bits...)
 // if (c->names) s->names = 1;          // with flags (not for bits...)
  if (c->imm)   s->immok = 1;
  return 0;
}




uint set_subr (CPS *c)
{
      //ALWAYS  from a user command.........

  SUB *xsub;
  SPF *f;
  uint err;
  CSTR *names;

  xsub = add_subr(0, c->p[0]);

  // check if duplicate, as allowed to override a system one

  err = get_lasterr(CHSUBR);

  if (err == E_DUPL)
    {
      // allow redef of existing system sub
       if (xsub->sys) err = 0;
    }

  if (err) return do_ch_error(c,CHCMD);

  xsub->usrcmd = 1;
  xsub->sys = 0;

  // do any special functions first
  //remove any spfs ??

  f = 0;

  if (c->spf)
     {            // special func is marked

      names = get_cstr(AUTONAMES, c->spf);               /// XXXXXXX

      f = append_spf(vconvi(xsub->start), c->spf > 12  ? 5 : 4, 0);

      f->fendin  = names->par2;
      f->fendout = names->par3;
      f->addrreg = c->adreg;          // register
      f->sizereg = c->szreg;          // cols
      f->usercmd = 1;

      #ifdef XDBGX
        DBGPRT(0, "  with spf = %x ", f->spf);
      #endif
    }  // end special func

  add_scan (c->p[0], J_SUB,0);

//need to chek the copy !!

//if (xsub->start == 0x968fe)
//{
 //   DBGPRT(0,0);

//}


  cpy_adt(c, vconvi(xsub->start));
  xsub->size = totsize(vconvi(xsub->start));

  xsub->cptl =set_layout(c,C_SUBR);    // default is argl ON


  if (c->ansreg)
   {
       f = append_spf(vconvi(xsub->start), 1, 0);
       f->addrreg = c->ansreg;                 //pars[0].reg = c->ansreg;
        #ifdef XDBGX
        DBGPRT(0, "  with ans = %x ", f->addrreg);     //pars[0].reg);
      #endif
   }






 return 0;
 }


uint set_time(CPS *c)
 {
  LBK *blk;
 // int val,type,bank;
 // uint xofst;
 // short b, m, sn;
 // SYM *s;
 // char *z;

  blk = add_cmd (c->p[0],c->p[1],c->fcom|C_USER);
  if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);
  cpy_adt(c,vconvi(blk->start));
  //s =
 // add_sym(C_CMD,c->symname,c->p[0],-1, 0,0);  // safe for null name
 // if (s) s->cmd = 1;

  /* up to here is same as set_prm...now add names

  if (!blk) return 1;          // command didn't stick
  a = blk->adnl;
  if (!a) return 0;                     //  nothing extra to do
  sn = 0;                             // temp flag after SIGN removed
  if (a->data) sn = 1;
  if (!a->name && !sn) return 0;       // nothing extra to do

  if (a->ssize < 1) a->ssize = 1;  // safety

  xofst = c->p[0];
  bank = g_bank(xofst);
  while (xofst < maxadd(xofst))
   {
    type = g_byte (xofst++);                  // first byte - flags of time size
    if (!type) break;                         // end of list;

    val = g_val (xofst, a->ssize);            // address of counter
    val |= bank;

    if (a->name)
    {
     z = nm;
     z += sprintf(z, "%s", anames[0].pname);
     z += sprintf(z, "%d", anames[0].nval++);
     if (type& 0x4)  z += sprintf(z,"D"); else z+=sprintf(z,"U");
     if (type&0x20) z += sprintf(z,"_mS");
     else
     if (type&0x40) z += sprintf(z,"_8thS");
     else
     if (type&0x80) z += sprintf(z,"_S");
     s = add_sym(0,nm,a->data,0,0);     // add new (read) sym
    }

    xofst+= casz[a->ssize];                           // jump over address (word or byte)

    z = nm;
    if (type&1)      // int entry
     {
      if (s && sn)
        {                      // add flags word name too
        m = g_byte (xofst++); // bit mask
        b = 0;
        while (!(m&1)) {m /= 2; b++;}    // bit number
        val = g_byte (xofst++);    // address of bit mask
        val |= bank;
        if (type & 8) z+= sprintf(z,"Stop_"); else z += sprintf(z,"Go_");
        sprintf(z,"%s",s->name);
        add_symbit(0 ,nm, val, b, 0, 0);
        }
      else     xofst+=2;                // jump over extras
     }
   }
*/

      // upd sym to add flags and times
      return 0;
 }



uint set_bnk (CPS *c)
{
  int i, bk, bank, add;
  BANK *b;

  // bank no , file offset,  (opt) pcstart, (opt)  bkend  , (opt) fillstart

   // use bk[2].cmd as marker to indicate bank command used already for
   // following bank commands, as this would not get set otherwise.

   // validate all params before overwriting anything

   if (c->npars < 2) return do_error(c,1, "need two parameters");

   b = get_bankmap(0);        // base bkmap

   bk = c->p[0];          // first param is BANK

   if (c->p[1] < 0 || c->p[1] > (int) flhdr.fillen) return do_error(c,1, "invalid File Offset");

   if ((bk & 6) || bk > 15) return do_error(c,1, "Invalid Bank Number");

   bk++;                      // internally always bank+1
   bank = bk << 16;          // bank in correct form

//  b = getxbank(bk);
  if (b[bk].usrcmd)   return do_error(c,1, "Bank already defined");

  // 2nd param is file offset


  if (bk > 0 && b[bk-1].bok)
    {        //check overlap in file offset
          if (c->p[1] <= (int) b->filend) return do_error(c,1, "File offsets overlap");
    }

   if (c->npars > 2)
     {     // bank start addr specified (for non std offset) (p[0] bank, p[1] start file offset)
      add = nobank(c->p[2]);
      if (add < PCORG-2 ||  add > PCORG+3 ) return do_error(c,1, "Bank Start too low");
     }

   if (c->npars > 3)
     {     // bank end specified. p[0] bank, p[1] start file offset)
      add = nobank(c->p[3]);
      if (add > 0xffff) return do_error(c,1, "Error -  Bank End too high");

      add -= nobank(c->p[2]);       // size, end-start.
      add++;

      if ((c->p[1] + add) > (int) flhdr.fillen) return do_error(c,1, "Bank end > File Size");

     }


//  if (c->npars > 4)      //fill specified zero otherwise


// then sort fillstart

// NEED MORE CHECKS HERE TO STOP CRASHES and set up data flag markers ..............


// clear valid map entries

   if (!b[3].usrcmd)
      {    //first use of 'bank' command - clear whole map.
  #ifdef XDBGX
     DBGPRT(1,"Bank autodetects CLEARED ***************");
   #endif
   // memset(b,0,
       for (i = 0; i < BMAX; i++)
          {
           b[i].bok = 0;
           b[i].cbnk = 0;
           b[i].bprt = 0;
           b[i].bkmask = 0;
          }
       set_numbanks(-1);                // reset no banks
       b[3].usrcmd = 1;
      }

   b = get_bankmap(bk);
   b->usrcmd = 1;
   b->bok = 1;
   b->bprt = 1;    // print in msg file
   b->filstrt = c->p[1];

   if (c->npars > 2) b->minromadd = nobank(c->p[2]) | bank;
   else  b->minromadd = PCORG | bank;

   if (c->npars > 3) b->maxromadd = nobank(c->p[3]) | bank;
   else  b->maxromadd = 0xffff | bank;

   b->filend =  b->filstrt + (b->maxromadd - b->minromadd);

   if (b->filend > flhdr.fillen)
      {
       b->filend = flhdr.fillen-1;
       b->maxromadd = (b->filend-b->filstrt) + b->minromadd;
      }

   set_numbanks(get_numbanks()+1);      //   numbanks++;

{
BASP *base;
base = get_basepars();
  base->codbnk = 0x90000;                         // code start always bank 8
  base->rambnk = 0;                               // start at zero for RAM locations
  if (get_numbanks()) base->datbnk = 0x20000;           // data bank default to 1 for multibanks
  else   base->datbnk = 0x90000;                  // databank 8 for single
}

   #ifdef XDBGX
     DBGPRT(0,"Bank Set %d %x %x ",bk, b->minromadd,b->maxromadd);
     DBGPRT(1," #(%x - %x  fill %x)",b->filstrt, b->filend, b->maxromadd);
   #endif
   return 0;
 }


uint set_vect (CPS *c)
{                          // assumes vect subrs in same bank as pointers
  uint  ofst, bank;
  int i;
  LBK *blk;
  ADT *a;
  BANK *b;

  if (chk_csize(c))   return 1;

  blk = add_cmd (c->p[0],c->p[1], C_VECT|C_USER);   // by cmd

  if (get_cmdopt(OPTMAN)) return 0;
  if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);

  if (c->adtcnt)                 //levels == 0)
   {
     a = get_adt(c,0);              //nl(&chadcm,0, 1);             // check addn entry
     bank = a->bank << 16;
     b = get_bankmap(a->bank);
     if (!b->bok) return do_error(c,1, "Bank invalid");
     cpy_adt(c,vconvi(blk->start));

   }
  else bank = g_bank(c->p[0]);

  for (i = c->p[0]; i < c->p[1]; i += 2)
    {
      ofst = g_word (i);
      ofst |= g_bank(bank);
      add_scan (ofst, J_SUB,0);      // adds subr

    }
  return 0;
}

uint set_code (CPS *c)
{

  add_cmd (c->p[0], c->p[1], c->fcom|C_USER);   // by cmd
  if (get_lasterr(CHCMD)) return do_ch_error(c,CHCMD);
  return 0;
}

uint set_scan (CPS *c)
{
  add_scan (c->p[0], J_STAT|C_USER,0);
 // if (chscan.lasterr) do_ch_error(c,&chscan);
   if (get_lasterr(CHSCAN)) return do_ch_error(c,CHSCAN);
 return 0;
}


uint set_args(CPS *c)
{
  LBK *blk;

 if (chk_csize(c)) return 1;

  blk = add_aux_cmd (c->p[0], c->p[1], c->fcom|C_USER);  // by cmd

 // if (chaux.lasterr)  return  do_ch_error(c,&chaux);
   if (get_lasterr(CHAUX)) return do_ch_error(c,CHAUX);

  // must clear all fieldwidths for args command

  cpy_adt(c,vconvi(blk->start));
 // set_data_vect(blk);

  blk->size = totsize(vconvi(blk->start));

  #ifdef XDBGX

  if (!blk->size)
      DBGPRT(1,"ZZ SIZE IS ZERO %x", blk->start);

  DBGPRT(1,0);
  #endif
return 0;
}

uint set_cdih (CPS *c)
{
    // xcode commands
  add_aux_cmd(c->p[0],c->p[1],c->fcom|C_USER);
 // if (chaux.lasterr) do_ch_error(c,&chaux);
   if (get_lasterr(CHAUX)) return do_ch_error(c,CHAUX);
  return 0;
}

uint set_psw (CPS *c)
{
    // psw setter p[0] checked, but not p[1]
  //c->p[1] = fix_input_addr(c->p[1]);      // force bank 9 if single bank
  add_psw(c->p[0],c->p[1]);
 // if (chpsw.lasterr) do_ch_error(c,&chpsw);
   if (get_lasterr(CHPSW)) return do_ch_error(c,CHPSW);
  return 0;
}










int chk_cmdaddrs(CPS *c, DIRS* d)
 {
   // check and fixup any address issues (bank etc)
   // can have a pair (start-end) and/or a single start (e.g. rbase has both)

   // check for valid bank and remember to add 1
   // banks set up before here

// can check numbanks < 0 here abaondon if no banks present.

//c->npars is pars read in.........

  int ans;
  int i;

  if (c->npars < d->minpars) return do_error(c,1,"Require at least %d parameters",d->minpars);
  if (c->npars > d->maxpars) return do_error(c,1,"Require %d parameters or fewer",d->maxpars);

  for (i = 0; i < d->maxpars; i++)
   {
      ans = validate_input_addr(c, i, d->ptype[i]);
      if (ans) break;
   }


    // NB. do error returns 1

 return 0;
 }





int do_adnl_item (CPS *c, ADT **fid, uint *ix)
{
   // options by command letter in one 'item'

   //append adt allows 'top' chain to be appended with master fid (pcmnd), subsequent
   //subchains added for fid of master block (an ADT) OR a pcmnd.
   //but still works with 'last adt'



  ADT *a;
  MATHX *x, *y;
  void *n;      // for calcs, name ot term
  DIRS *d;

  int ans, rlen, opt;

  ADT *subfid;     // for subcommands, need a LOCAL fid for next level
  uint subix;

  subix = 1;               // next level down if get a '['

  d = dirs+c->fcom;

   // first open bracket is already read in, so start new item

  if (c->adtcnt > 32) return do_error(c, 1, "Too Many data items");
  c->adtcnt++;                  //number of terms in main level

//---- like this now.....

 a = (ADT*) chimem(CHADNL);           //assemble ADT for insert


// if (!a) error...

  if (a)
   {
     if (*fid)
       {
         a->fid = *fid;
  //       a->fend = (*fid)->fend;           //pass in size
       }
     else a->fid = (ADT*) c;

     a->cnt    = 1;
     a->fend = 7;                               //default ...
     a->sbix = *ix;
     a->dptype = d->defdptype;                  //from command
     a->bank = c->p[0] >> 16;                  // c->p[0] is start address
     a = append_adt(a);            //*fid, *ix);     // ix allows for subfields
   }

  *ix = 0;              //  only first term will ever have ix > 0, and that is a '1' at this time
  *fid = a;

  subfid = a;

 // if (a->fend < dirs[c->fcom].dfend)  a->fend = dirs[c->fcom].dfend;     // check preset for some commands is this right ????

  // now scan down cmd string - use sqcnt to sort out subs....(subix is no good if it's reset.........)

  while (c->cmpos < c->cmend)
    {
      if (c->error) break;

      if (c->sqcnt > 1)  opt = get_optchar (c, dirs[c->fcom].subopts);
      else               opt = get_optchar (c, dirs[c->fcom].mainopts);


    switch(opt)            //toupper done in get_optchar and skipped.
     {


        case '[' :         // new data term inside current one....becomes 1 ?
         do_adnl_item (c,&subfid, &subix);  // nested open - fkey = ADT block 'a' + level via bkts
         if (!c->error) a->sub = 1;         // mark subflags, not used anywhere yet
         break;


    case ']' :         // end of field

    case -2:           //  end of line (safety)

         return 0;


       case '|' :
         //   split printout for large structs, ignore if arg layout set
      //    uint  extcpt   : 2;       // use extended (1) or compact (2) layout
         if (c->extcpt != 1) a->newl = 1;
         c->split = 1;
         break;

      case '=' :

         // formula name or a short calc if in ()
         c->mcnt = 0;
         //read_qname
         readpunc(c);

         if (*c->cmpos != '\"')
           {                                    // not a name. look for calc_type
             ans = get_cmdstr(c, CALCSTR);
             if (ans <= 0)  return do_error(c, 1, "Invalid calc type");
             c->mcalctype = ans;               // set master calctype
             do_math_str(c,a);
             add_link(CHADNL,a,CHMATHX,c->mterm,1);          // mterm <-> adt

           }
         else
          {    // expect a calc name
             get_qstring(c, 5);         //must have name
             if (c->names[1].len)       //name provided
              {
                n = get_mname(0,c->names+1);                             // name only search n is void *
                if (!n)   return do_error(c, 1, "Calc name not found");

                x = get_mterm(n,0);                      // get math calc for name
                add_link(CHADNL,a,CHMATHX,x,1);          // mterm <-> adt
         //       add_link(CHADNL,a,CHMATHN,n,1);          // mname linked to calc. <-> adt
             }
          }
         readpunc(c);
         break;



       case 'B' :                 // Bit for subfields and syms.
         ans = getpd(c,4,2);
         if (!ans) return do_error(c,  1, "Bit Number Required");
         //but this doesn't work with signs etc...........
         if (c->p[4] < 0 || c->p[4] > 31) return do_error(c,  1, "Invalid Start Bit");
         if (c->p[5] < 0 || c->p[5] > 31) return do_error(c,  1, "Invalid End Bit");

         a->fend &= 0x60;         // keep sign and write
         a->fstart = c->p[4];
         a->fend   |= c->p[5];
         // 'W' for write and 'S' sets a->fend already.

         if (ans < 2)
            {
              if (a->fend & C_SIGN) return do_error(c,  1, "Single bit cannot be signed");
              a->fend |= a->fstart;  // single bit
            }

         c->bitfld = 1;             // bit field specified - need to know !!

         break;

// C compact ?

       case 'D' :

         // 1 hex value (address offset with bank) deprecated but should still work....
         ans = getpx(c,4,1);
         if (!ans) return do_error(c,  1, "Address Required");

       x = (MATHX*) chimem(CHMATHX);


         ////  DON'T need name + 2 terms, +x (first,noname), + offset
         //x = 0;
         //n = add_mname(a,0);         //add zero name
         //if (n)
           // {
          //   add_link(CHADNL,a,CHMATHN,n,1);               // mterm -> adt (fwds)
       //      n->mcalctype = DP_DADD;
          //   x = add_mterm (n,0);
          //  }

         if (x)
          {
           x->calctype = DP_DADD;
           x->func = 1;
      //     x->calc = &xplus;
           x->npars = 1;
           x->fpar[0].dptype = DP_REF;
           x->lfirst = 1;

           //2nd term
           y = (MATHX*) chimem(CHMATHX);
        //   x = add_mterm (x,0);              // 2nd -> 1st term
    //       x->calctype = DP_DADD;
    y->fid = x;
           y->func = 1;
     //      x->calc = &xplus;
           y->npars = 1;
           y->fpar[0].ival = c->p[4];    //offset value
           y->fpar[0].dptype = DP_HEX;
          }
         add_mterms(x);

         a->pfw = get_pfwdef(a);  //set_pfwdef(a);  //word sized for offsets
         break;

       case 'E' :     // ENCODED - always size 2 deprecated
         ans = getpx(c,4,2);
         if (ans != 2 )
            {
              return do_error(c,  1, " 2 values Required");
            }

         n = add_encode(CHADNL, a,c->p[4], c->p[5]);
         if (!n) return do_error(c,  1, " encode calc failed!");

  a->fend = 15;            // encoded implies unsigned word
  a->pfw = get_pfwdef(a); //set_pfwdef(a);
  if (!a->cnt) a->cnt   = 1;

         break;

       case 'F' :

         // flags for symbol  (mask ??
         if (c->fcom == C_SYM)
          {             // flags word/byte for SYM.
      //     ans = getpx(6);
        //   if (ans)
      //      {
        //     if (ans != 1 ) return do_error(c, 0, 1, " Value Required");

             c->p[7] |= 8;              // 'F' set
           // }
    //       else cmndp[6] = 0;
         }
        break;



  // G H J

       case 'I' :               // immediate fixup for syms

        if (c->fcom == C_SYM)  c->p[7] |= 4;       //immediate
        break;

       case 'K' :

         // 1 decimal value (bank, 0,1,8,9), but plus 1 for internal
         ans = getpd(c,4,1);
         if (!ans) return do_error(c,  1, "Bank number Required");

        c->p[4]++;
         if (!valid_bank(c->p[4])) return do_error(c,  1, "Bank Number Invalid ");
         a->bank  = c->p[4];
         if (!a->cnt) a->cnt = 1;     // but size stays at zero
         break;

       case 'L' :               // long int (32 bit)

         a->fend &= 0x60;        // keep sign and maybe write
         a->fend |= 31;
       //  a->bsize = 4;
         if (!a->cnt) a->cnt = 1;
         a->pfw = get_pfwdef(a); //set_pfwdef(a);
         break;

 // case 'M':                // mask pair ?, use esize to decide word or byte
 //    cadd->mask = 1;
 //    break;

       case 'N' :

         a->fnam = 1;       // Name lookup in structs
          if (c->fcom == C_SYM) c->p[7] |= 2;
         break;

       case 'O' :                // Cols or count - 1 decimal value

// count may not fit with subfields  [o 5 [b 5 6]] causes 10 items ?? probably not, but DOES work for calcs (e.g. funcs and tabs)

         ans = getpd(c,4,1);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c,  2, " Repeat value Missing, 1 assumed");
           }
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,  1, " Repeat value Invalid (1-32)");

         a->cnt = c->p[4];
         break;

       case 'P' :               // Print fieldwidth (in spaces, not digits) 1 decimal
                                // float handling in 'X'
         ans = getpd(c,4,1);
         if (!ans)
           {
             c->p[4] = 1;
             do_error(c, 2, "Field Width Missing, 1 assumed");
           }
         if (c->p[4] < 1 || c->p[4] > 32) return do_error(c,  1, " Field width Invalid  (1-32)");

         a->pfw = c->p[4];

         break;

       case 'R' :               // indirect pointer to subroutine (= vect)

       //  a->fend &= 0x60;
         a->fend = 15;           // safety, lock to WORD, but not always true for offsets
         a->dptype = DP_DADD;      // address mode

         //and cross check against calcs

     //    a->name  = 1;          // with name by default  NO !!
         if (!a->cnt) a->cnt   = 1;

         a->pfw = get_pfwdef(a);     // set_pfwdef(a);
         break;


       case 'S':              // set Signed
          a->fend |= 0x20;
          break;

       case 'T':                 // bit -> Triple
         a->fend &= 0x60;           // keep sign
         a->fend |= 23;
         if (!a->cnt) a->cnt   = 1;
         a->pfw = get_pfwdef(a);          //set_pfwdef(a);
         break;


       case 'U' :               // unsigned (default so optional)
         a->fend &= 0x5f;       // clear sign, keep others
         break;


// variable size struct ??  how to specify ?? V st field, end field/relative address to row/value ??

       case 'V':                  // diVisor DEPRECATED, now to calc move to VECTOR ??
       {
         float fx;
         // read one FLOAT val
         ans = sscanf(c->cmpos, "%f%n ", &fx, &rlen);       // FLOAT
         if (ans <=0) return do_error(c,  1, "Divisor value Required");
         c->cmpos += rlen;

        //  need name + 2 terms, +x 1st, / divisor 2nd
     //    x = 0;

      //   n = add_mname(a,0);         //add zero name
      //   if (n)
       //    {
        //    add_link(CHADNL,a,CHMATHN,n,1);               // mterm -> adt (fwds)
     //       n->mcalctype = DP_FLT;
         //   x = add_mterm (n,0);
         //  }

         x = (MATHX*) chimem(CHMATHX);

         if (x)
          {
           x->calctype = DP_FLT;
           x->func = 1;
     //      x->calc = &xplus;
           x->npars = 1;
           x->fpar[0].dptype = DP_REF;
           x->lfirst = 1;

           //2nd term

           y = (MATHX*) chimem(CHMATHX);
           y->fid = x;
       //    x = add_mterm (x,0);              // 2nd -> 1st term
           y->calctype = DP_FLT;
           y->func = 4;
    //       x->calc = &xdiv;
           y->npars = 1;
           y->fpar[0].fval = fx;           // divsior
           y->fpar[0].dptype = DP_FLT;
          }
add_mterms(x);

       }
         break;

       case 'W':                  // Word   - or write for syms
            if (c->fcom == C_SYM)    a->fend |= C_WRITE;            // write for syms
            else
            {    // WORD for others
               a->fend &= 0x60;           // keep sign and write
               a->fend |= 15;
               if (!a->cnt) a->cnt   = 1;
               a->pfw = get_pfwdef(a);       //set_pfwdef(a);
            }


          break;

       case 'X':                  // Print radix  ( Hex default)
         ans = getpd(c,4,2);
         if (!ans)
           {
             a->dptype = DP_HEX;
             do_error(c, 2, "Radix value missing, default to HEX");
           }

// change to :x 10.z  for float, where z is num places.
// t = flbuf+cmndposn;       // where we are in cmd string
// if (t = '.')    go float

  // cross check for R which will set pmode at 1
  // print mode & radix.  0 = default (hex) 1 = hex (user), 2 = address (hex+bank) 3 = bin  4 = dec int  5 = dec float (= calc only) (R, X)


// allow last digit as shortcut (0,2,6) ?? so X0.2 for 2 decimal places...

//switch ??

         else if (c->p[4] == 2 ) a->dptype = DP_BIN;
         else if (c->p[4] == 10 || c->p[4] == 0 ) a->dptype = DP_DEC;
         else if (c->p[4] == 16 || c->p[4] == 6 ) a->dptype = DP_HEX;
         else return do_error(c,  1, "Radix value Invalid  (2,10,16)");

         if (ans == 2)
          {       //decimal place
            if (a->dptype != DP_DEC)  return do_error(c,  1, "Float Invalid unless x=10");
                a->dptype = DP_FLT;            // set float print mode.
                a->pfd = c->p[5];        // number of digits field width (after .)
                // check pfd is valid ...

          }
          // reset pfw according to new print type.
                          // CANNOT add pfd to pfw....need the '.' to line up.....



         break;

       case 'Y':                  // bYte
         a->fend &= 0x60;           // keep sign and write, set size to 1
         a->fend |= 7;
         if (!a->cnt) a->cnt   = 1;
         a->pfw = get_pfwdef(a);            //set_pfwdef(a);
         break;

         // Z
                case 'Z':
         c->debug = 1;
         break;

       case -1 :
       default:
    //   c->cmpos++;       //move to failed char
           return do_error(c,  1, "Invalid Option");

      }          // end switch

    }  // end while

 return 0;
    }





int do_adnl_data (CPS *c)
{
   // top level shell for adnl_item
 // int opt;
  ADT *fid;
  uint ix;
 // get next delimiter and then options

  fid = 0;      //&cmnd;           // dummy to start with
  ix = 0;
  while (c->cmpos < c->cmend)
    {
     readpunc(c);

     if (c->cmpos >= c->cmend) break;
     if (*c->cmpos != '[') do_error(c, 1," '[' expected");
     c->sqcnt++;
     c->cmpos++;

     do_adnl_item (c,&fid,&ix);     //this is new item
     if (c->error) break;
     readpunc(c);
     if (c->sqcnt) do_error(c, 1," ']' expected");

     readpunc(c);            //in case trailing spaces at end




   /*  opt = get_optchar(c,0);         // NO !!



     if (opt == '[') do_adnl_item (c,&fid,&ix);     //this is start call, get ADT block here to start ?

     if (!opt) return do_error(c,  1, "'[' expected");      // only 'open' expected here  */

     if (c->error) break;
    }

 // if (c->sqcnt && c->cmpos < c->cmend) return do_error(c, 1," ']' expected");

return 0;
}




















int do_global_opts(CPS *c)
{
   int inglo, ans, opt;
   char *t;

   if (!dirs[c->fcom].glopts) return 0;         // global opts not allowed

   if (!readpunc(c)) return 0;

   t = c->cmpos;

//but this messes up bkt count.....do via case instead ?
   if (*t != '[' || *(t+1) != '$') return 0;       // must be "[$" start

   c->cmpos  += 2;                            // skip the '$'
   c->sqcnt++;

   inglo = 1;

   while (inglo)    //cmpos < cmend
    {
     if (!readpunc(c)) break;

     opt = get_optchar(c, dirs[c->fcom].glopts);

   //  if (opt != '[' ) return do_error(c, 1," '[' expected");

     if (opt < 0) return do_error(c, 1,"Invalid Option");      // illegal option char (4)
  //   else c->cmpos += 1;                                  // Only single char here

     switch (opt)
     {
       case ':'  :
       case ']'  :
         inglo = 0;            // end of global options
     //    c->cmpos--;            // go back to colon
         break;

//case '[' :  ??
//check $ here ??


       case 'Q' :              // terminator byte

         // read optional number of bytes max 3.
         ans = getpd(c, 4,1);
         if (!ans) c->term = 1;
         else
          {
            if (c->p[4] < 1 || c->p[4] > 3) return do_error(c,1, "Size invalid (1-3)");
            c->term = c->p[4];
          }
         break;

//check can't have A and C.....extcpt is zero if not specified.
//  c->extcpt       0 default  use extended (1) or compact (2) layout
       case  'A' :

          if (!c->extcpt) c->extcpt = 1;        // set args layout
          else  return do_error(c,1, "layout already set");
         break;

       case  'C' :

// if (c->argl)
         if (!c->extcpt) c->extcpt = 2;        // set compact layout
                 else  return do_error(c,1, "layout already set");
          break;


       case  'F' :
        // f <str> <reg1> <reg2>    func add/ tab add, cols

         ans = get_cmdstr(c,SPFSTR);
         if (ans <= 0)  return do_error(c,1, "Subroutine type reqd");
         c->spf = ans + 4;                  //do this in cstr.....later
         ans = getpx(c,5,2);

         // read colons in here and cheat ???
// register check ??


//where is '='  ??

         if (c->spf < 13)
          {
           if (ans < 1)   return do_error(c,1, "At least %d values required ",1);
           if (ans > 1)   do_error(c,2,"Extra Values ignored");
          }
         else
          {
           if (ans < 2)   return do_error(c,1, "At least %d values required ",2);
           if (ans > 2)   do_error(c,2, "Extra Values ignored");
          }

         c->adreg = c->p[5];                  // struct address register
         c->szreg = c->p[6];                  // tab column register
         break;



       default:
         do_error(c,1, "Invalid Option");
         break;
     }
    }
   return 0;
}


int do_opt_str (CPS *c)
{
 // same as get_str, but strings for setopt, clropt, but need brackets around them
 // so do delims separately in here, use subr to count brackets

   int ans;
   uchar set;
   DIRS *d;
   CSTR *s;

   d = dirs+c->fcom;

   set = d->ptype[0];         //set or clearopts.

   //c->p[0] = 0;

   while (c->cmpos < c->cmend)
    {
 //     readpunc(c);

      ans = get_delim_char(c);

      if (ans != '[') do_error(c, 1," '[' expected");
      ans = get_cmdstr(c, OPTSTR);

      if (ans < 0)  return do_error(c,  1, "Invalid Option");       // not found

// map a CSTR ! then can dort directly for options
      //do set here if ans >=0

    // or optstrs[ans].par1;
      s = get_cstr(OPTSTR,ans);       //but this more generic
      s->par1 = set;

      if (s->par2)
          {     // read a param and set par1.
            ans = getpd(c,0,1);
            if (!ans)  return do_error(c, 2, "Decimal places missing");
            if (c->p[0] <0 || c->p[0] > 7) return do_error(c, 2, "Decimal places must be 0-7");

            s->par1 = c->p[0];
          }


      ans = get_delim_char(c);

      if (ans  != ']') do_error(c, 1," ']' expected");
      if (c->sqcnt && c->cmpos < c->cmend) return do_error(c, 1," ']' expected");

//      c->p[0] |= optstrs[ans].par1;

      // NO !! won't handle multiple strings !! have to do the set in here !!

  // set_opts, pp_dmy,   0, 0,  0,0,0,0,  0,0,  0,    0,  0, 0,   0, 1,   0,           0, 0 },                  // set options (external strings array)
  // { clr_opts, pp_dmy,   0
/*
  uint get_cmdopt(uint x)
{
  CSTR *s;
  s = get_cstr(OPTSTR,x);
  if (!s) return 0;
  return s->par1;
}

void set_cmdopt(uint x, uint state)
{
 CSTR *s;
  s = get_cstr(OPTSTR,x);
  if (!s) return;
  s->par1 = state;

}
  */
    readpunc(c);            //in case trailing spaces
    }

return 0;

}



void replace_delims(CPS *c)
{

  // convertor for old->new cmd formats
  // need to be able to spot math formulae in = ( ) and not change them............
  // END colon fails !!

 char *s, *d;              //source, dest
 int obkt, skip, name;

 s = flbuf;

 d = nm;          //c->string;            //temp holder

 obkt = 0;
 name = 0;
 skip = 0;

 while (*s)
  {


    if (*s == '\"')
      {
        name ^= 1;                         // in a "name" sequence
        while (*(++s) == '\"');                          // *(s+1) == '\"') s++;        // skip all multiple quotes (as typo)
        *d++ = '\"';
      }

    if (*s == '[')          skip += 1;    // skip any new style format pairs
    if (*s == ']')          skip -= 1;     // for nested setups



 if (!name && !skip)
   {
    if (*s == ':' || *s == '|')    // : is start or end
     {
       if (*s == '|') *d++ = '|';   // put pipe back at end of 'previous' item

       if (obkt) *d++ = ']';
       *d++ = '[';
       obkt = 1;                  // in '[ ]' from ':'

       while (*s == ':' || *s == '|') s++;               // skip the :, |, and any multiples
     //this may be wrong....



     }

//not if new format already ????

    if (*s == '$')
       {                //global options
        if (obkt) *d++ = ']';           //if not at front
        *d++ = '[';
        *d++ = *s++;          // change '$' to '[$'
        obkt = 1;
       }




  if (*s == '#' && obkt)
      {            // if comment add close bkts if necessary
       char *x;
       x = d;
       while (*(--x) == ' ');       //skip backwards for any spaces
       x++;                         //to next char
       *x = ']';
       obkt = 0;
      }

  }           // end !name

    if (*s == '\r' || *s == '\n' )
        {             //if line end add close bkts if colon
          if (obkt)
            {
             *d++ = ']';
      //       obkt = 0;
            }
          break;              //ALWAYS break if line end.
        }

    *d++ = *s++;       //otherwise, just copy
 }

*d = '\0';

strcpy(flbuf,nm); // and copy new one over old one.
}

//probably should be in core....

int new_symname (SYM *xsym, CSTR *fnam)
 {
   // replace symbol name
   // symname cannot be NULL

  int oldsize;

  if (!fnam) return 0;
  if (!fnam->len) return 0;

  if (xsym->name)
    {
      if (xsym->usrcmd) return 0;                           // can't rename if name set by user
      if (!strcmp(xsym->name, fnam->string)) return 0;      // same name
    }

  oldsize = xsym->symsize;                                   // old size
  xsym->symsize = fnam->len;                                 // excludes null

  if (xsym->symsize > SYMSZE) xsym->symsize = SYMSZE;

  xsym->name = (char *) mem(xsym->name,oldsize,xsym->symsize+1);   // get new sym size

  if (xsym->name)
     {
      strncpy (xsym->name, fnam->string, xsym->symsize);           // and replace it
      xsym->name[xsym->symsize] = '\0';                            // safety
     }
  return xsym->symsize;
 }


/*
  j = (get_cmdopt(OPT8065)) ? 0x205f : 0x201f;
     j |= bank;

     add_cmd (0x200a|bank, 0x200f|bank, C_WORD|C_SYS,0);     // from Ford handbook
     add_cmd (0x2010|bank, j, C_VECT|C_SYS,0);               // interrupt vects with cmd

     for (j -= 1; j >= (0x200f|bank); j-=2)      // counting DOWN to 2010
        {
         addr2 = g_word (j);
         addr2 |= bank;
         add_iname(j,addr2);
         add_scan (addr2, J_SUB,0);      // interrupt handling subr

        }

        */

void add_iname(int from, int ofst)
{
  // add interrupt autonames
  uint i;
//  char *z;
  CSTR *n;

  // ignore_int can be at the end of a jump in multibanks....

   if (!get_cmdopt(OPTINTH)) return ;

   from = nobank(from);
   from -= PCORG;

   if (get_cmdopt(OPT8065)) n = get_cstr (INAME65,0);
   else n = get_cstr(INAME61,0);                      // 8061 or 8065

 // z = nm;
  tempstr.len = 0;

  if (get_numbanks())               // sort out bank number
   {
    i = ofst >> 16;             // add bank number after the I
    csprt("B%u_",i-1);
   }
// else
  //  z += sprintf(nmx, "I_");         // no bank number

// if (match_sig(&intign, ofst))       // this is 'ignore int' signature

  i = g_byte(ofst);

  if ( i == 0xf0 || i == 0xf1)       // this is 'ignore int'
    {
      csprt("Ignore");
      add_sym(&tempstr, ofst,0,7 |C_NOBIT | C_SYS, 0,0xfffff);

      return;
    }

//CSTR par2 is start, par3 is end par4 is stringcount, BUT

 while(1)           //for (i = 0; i < NC(inames); i++)
    {
      if (!n->par2) break;            //end of list

    if (from <= n->par3 && from >= n->par2)
       {

       csprt("%s",n->string);
       if ((n->par3 - n->par2) > 1) csprt("%u", (from-n->par2)/2); //add number
       add_sym(&tempstr, ofst,0,7|C_NOBIT|C_SYS,0,0xfffff);
       break;    // stop loop once found for single calls
       }
       n++;
    }
return;
 }



int parse_cmd(CPS *c, char *flbuf)
{
  // parse (and check) cmd return error(s) as bit flags
  // return 1 for next line (even if error) and 0 to stop (end of file)

  int ans;
  char *t;                        // where we are up to
  DIRS* d;
  ADT *a;
  CHAIN *x;
  void *fid;

  memset(c, 0, sizeof(CPS));          // clear entire struct

  //first clear any residual additional data.

  fid = c;
  x = get_chain(CHADNL);

  while ((a = get_adt(fid,0)))        // get first block (dummy fid)
  {
     fid = a;                         // next.....
  //  ans = get_lastix(CHADNL);         // save index of block
     chdelete(x, x->lastix, 1);         // and delete it
   }



  if (!fgets (flbuf, 255, fldat[DIRFILE].fh)) return 0;

  c->cmpos = flbuf;                       // start of string

  flbuf[255] = '\0';                     // safety

  replace_delims(c);                     // replace old style colons etc with new format

  //end string at CR or LF

  t = flbuf;

  while (*t)
   {
     if (*t == '\n' || *t == '\r') *t = 0;
     t++;
   }

  c->cmend = flbuf + strlen(flbuf);        // end of line

  if (c->cmend <= c->cmpos) return 1;      // this is a null line;

  if (!readpunc(c)) return 1;              // null line after punc removed

  t = c->cmpos;
  if (*t == '#')  return 1;                // # comment at beginning (after punctuation), ignore



  ans = get_cmdstr(c,CMDSTR);              // string match against cmds array - zero is valid
  if (ans < 0)  return do_error(c,1, "Invalid Command");             // cmd not found

  d = dirs+ans;
  c->fcom = ans;

  readpunc(c);

  if (d->maxpars)                          // read up to 8 following addresses into p[0-n] (if any)
    {
     c->npars = getpx(c,0, d->maxpars);

     if (c->npars < d->minpars) return do_error(c,1, "More parameters required ");        // params reqd
     if (c->npars > d->maxpars) return do_error(c,2, "Extra parameters ignored"); // extra pars ignored (continue)

     // verify addresses, single and/or start-end pair
     // but not whether valid for data or code (yet)

     ans = chk_cmdaddrs(c, d);

    // ans is not used ???
    // ans id currently 0 or 1
    // (do_error sets c->error to 1 if error. c->error is full int

     if (c->error > 1) return 0;
     if (c->error) return 1;

     // c->error is 0 for warning, 1 for error

     // return 0 if serious bank problem
     // "no banks defined, cannot continue" ..................

    }

  // read name here, if allowed and present - sort out spaces in names ??

 // readpunc(c);  done in qname

  if (d->namex)
    {
      get_qstring(c, 0);                       //  names[0]
      if (d->namex == 2 && !c->names->len)  return do_error(c, 1, "Command must have a Name");
    }
  else
   {
    readpunc(c);
    if (*c->cmpos == '\"')
      {
        c->cmpos++;
        return do_error(c, 1, "Name not allowed here");
      }
   }


   // NOW check for comment, AFTER the name
   t = strchr(c->cmpos, '#');
   if (t) c->cmend = t;              // shorten line as reqd

 //drop spaces at end of string for clean errors etc.

 // xprt(MSGFILE,1, " { %s }",flbuf);            DEBUG
  while (isdelpunc(c->cmend) == 1) c->cmend--;

  *c->cmend = '\0';

  switch(d->stropt)
    {
      default:                  // case 0 (safety)
         do_global_opts(c);
         do_adnl_data(c);
      break;

      case 1:
        do_opt_str(c);         // option strings (setopt)
        break;

      case 2:
       //calcs -  name already read in (c->names[0])
      // '=' to deal with...............
        readpunc(c);
        if (*c->cmpos != '=') return do_error(c, 1,"calc must start with = ");
        c->cmpos++;
        c->mcalctype = get_cmdstr(c, CALCSTR);

        if (c->mcalctype <= 0) return do_error(c, 1, "Invalid Calctype");
        do_math_str(c,0);        // math formula, null fid so must have a name (in c->names)
        break;
    }

  if (c->error) return 1;

  if (!d->maxadt && c->adtcnt) return do_error(c, 1,"Extra [data] items not allowed");
  if (c->adtcnt > d->maxadt)   return do_error(c, 1,"Too many [data] items specified\n");
  if (c->adtcnt < d->minadt)   return do_error(c, 1,"More [data] items required\n");

  // any further error reporting is responsibility of each handler command

  c->adtsize = totsize(c);          //safety.

  if (d->setcmd) d->setcmd (c);                  // do setup procedure if required

  // add sym name for cmds other than SYM itself, but not maths ones

  if (d->namex && c->fcom < C_SYM && c->names->len)
    {
     // default add sym name for cmds other than SYM and CALC
     add_sym(c->names,c->p[0],0, 7|C_NOBIT|C_USER|C_RENAME, 0,0xfffff);
     if (get_lasterr(CHSYM)) return do_ch_error(c,CHSYM);
    }

 if (c->firsterr) xprt(MSGFILE,1,0);    // extra LF after any errors

 // if (!c->error)   xprt(MSGFILE, 1, "  --OK--");          //TEMP !
 show_prog();
 return 1;                   // next line
}

/*************************************
get next comment line from file
move to combined bank+addr (as single hex)
 * could use CPS struct here too.........
**************************************/
//int getpx(CPS *c, int ix, int l);
//int readpunc(CPS *c);
//uint fix_input_addr_bank(CPS *c, int ix);


int get_cmnt (CPS *c)
{
  int ans;
  char *t;

  FDATA *fl;

  fl = get_fldata(CMTFILE);

  if (c->split) {c->split = 0; return 0; }          // a '|' is this ever set for comments ?

  if (c->cmpos < c->cmend) return 1;     // more to do...


  ans = 0;

  c->cmpos = flbuf;

  if (fl->fh == NULL || feof(fl->fh))
   {                                    // end file or no file
    c->p[0] = 0xfffff;                  // max possible address with max bank
    flbuf[0] = '\0';                    // NULL
    return 0;                           // no file, or end of file
   }

  while (ans < 1 && fgets (flbuf, 255, fl->fh))
    {
       flbuf[255] = '\0';                  // safety

       t = strchr(flbuf, '\n');            // check for LF
       if (t) *t = '\0';                   // shorten line

       t = strchr(flbuf, '\r');            // check for CR
       if (t)   *t = '\0';        //c

       if (*flbuf == '#') *flbuf = '\0';        // line commented out (before number)

       ans = strlen(flbuf);
       c->cmend = flbuf + ans;   // end of line
       c->cmpos = flbuf;                   // start of string
     }

  if (ans > 0)
    {
      ans = getpx(c,0,1);                      // input address
      if (fix_input_addr_bank(c,0)) // return 0;    // but then what ??
      {  //fix ofst instead ?
         c->p[0] = 1;  //print rest of comment....
      }


      readpunc(c);

//move to prt loop ?
      c->newline = 0;
      while (*c->cmpos == 0x20) c->cmpos++;     // consume any spaces after number
      t = c->cmpos;
      if (*t == '\\' && *(t+1) == 'n') c->newline = 1;    // newline at front of cmt
      // mark a '|' or '\|' here for split comment ?? c->bits ?? but need it in ptr_cmt as well for multiples.
      //end move
      return 1;
    }

  *c->cmpos = '\0';              // anything else is dead and ignored...
  c->p[0] = 0xffffff;            // max possible address with max bank
  return 0;

}



void do_preset_syms(void)
 {
   uint i;
   CSTR *z;

   if (get_cmdopt(OPT8065))
    {
     for (i = 0; i < NC(d65syms); i++)
     {
       z = d65syms+i;
       add_sym(z, z->par3,z->par1,(z->par2 | C_SYS),0,0xfffff);
     }
    }
   else
    {
     for (i = 0; i < NC(d61syms); i++)
     {
       z = d61syms+i;
       add_sym(z, z->par3,z->par1,(z->par2 | C_SYS),0,0xfffff);
     }
    }

  }


/***************************************************************
 *  read directives file
 ***************************************************************/
void getudir (void)
{
  int addr,addr2, i, j, bank;
  BANK *b;

  // cmdopts will be zero or P8065 here (before dir read)
  // and banks should be sorted (auto detect anyway)

 // cmdopts |= OPTDFLT;            // add default    done in new setup

  if (fldat[DIRFILE].fh == NULL)
    {
     xprt(MSGFILE,2,"# ----- No directive file. Use default options");
    }
  else
    {
      xprt(MSGFILE,2,"# Read commands from directive file at '%s'", fldat[DIRFILE].fn);
      while (parse_cmd(&cmnd, flbuf));
    }




  if (get_cmdopt(OPTPRE)) do_preset_syms();

  if (get_cmdopt(OPTMAN)) return;         // entirely manual

  // now setup each bank's first scans and interrupt subr cmds
// ERROR IF NO BANKS !!


  for (i = 0; i < BMAX; i++)
    {
     b = get_bankmap(i);
     if (!b->bok) continue;

     #ifdef XDBGX
      DBGPRT(1,"--- auto setup for bank %d ---",i);
     #endif

     bank = i << 16;
     addr = b->minromadd;
     addr |= bank;

     add_scan (addr,J_INIT,0);                  // inital scan at PCORG (cmd)

     j = (get_cmdopt(OPT8065)) ? 0x205f : 0x201f;
     j |= bank;

     add_cmd (0x200a|bank, 0x200f|bank, C_WORD|C_SYS);     // from Ford handbook
     add_cmd (0x2010|bank, j, C_VECT|C_SYS);               // interrupt vects with cmd

     for (j -= 1; j >= (0x200f|bank); j-=2)      // counting DOWN to 2010
        {
         addr2 = g_word (j);
         addr2 |= bank;
         add_iname(j,addr2);
         add_scan (addr2, J_SUB,0);      // interrupt handling subr

        }
     }

   #ifdef XDBGX
      DBGPRT(1," END auto setup");
     #endif

// return 0;
 }



char *calcfiles(char *fname)
 {
  // do filenames
  char *t, *x;
  int i;

  t = strrchr(fname, '\n');          // DOS and Linux versions may have newline at end
  if (t) {*t = '\0';}

  x = fname;                           // keep possible pathname

  t = strrchr(fname, PATHCHAR);         // stop at last backslash
  if (t)
   {          // assume a path in filename, replace path.
    *t = '\0';
    fname = t+1;
    sprintf(flhdr.path, "%s%c", x,PATHCHAR);
   }


  x = strrchr(fname, '.');                         // ditch any extension
  if (x) *x = '\0';

  strcpy(flhdr.bare,fname);                        // bare file name

  for (i = 0; i < numfiles; i++)
   {
    if (! *(fldat[i].fn)) strcpy(fldat[i].fn,flhdr.path);   // use default path name

    strcat(fldat[i].fn,flhdr.bare);                 // bare name
    strcat(fldat[i].fn,fsfx[i].suffix);                // plus suffix
   }


 return flhdr.bare;
 }


int get_config(char** pts)
 {
  /* pts [0] = exe path (and default) ,
   * [1] = config file READ path or zero.  (use default path)
   * both from command line
   */

  short i;
  char *t;
  FDATA *inifl;                //sad.ini entry
  HDATA *fh;
  FDATA *fd;

  // PATHCHAR defined at beginning with file option flags

  inifl = get_fldata(INIFILE);
  fh = get_flhdr();
  fd = get_fldata(0);               //base struct

  t = pts[0];
  if (*t == '\"' || *t == '\'') t++;      // skip any opening quote
  strcpy(fh->exepath, t);

 // if (!t) printf("error in cmdline !!");    // never true ??

  t = strrchr(fh->exepath, PATHCHAR);           // stop at last backslash to get path
  if (t) *t = '\0';

  if (pts[1])
  {
     strncpy(inifl->fn,pts[1], 255);           // copy max of 255 chars
     i = strlen(inifl->fn);                    // size to get last char
     t = inifl->fn+i-1;                        // t = last char
     if (*t == PATHCHAR) *t = 0; else t++;

     sprintf(t, "%c%s",PATHCHAR,fsfx[INIFILE].suffix);    // and add sad.ini to it...

     inifl->fh = fopen(inifl->fn, fsfx[INIFILE].fpars);

     if (!inifl->fh)
       {
           printf("\ncan't find config file %s\n", inifl->fn);
           return 1;                // no config file in -c
       }
  }

 else
  {
    // else use bin path to make up file file name of SAD.INI
    sprintf(inifl->fn, "%s%c%s",flhdr.exepath,PATHCHAR,fsfx[INIFILE].suffix);
    inifl->fh = fopen(inifl->fn, fsfx[INIFILE].fpars);         //open it if there
  }


   if (inifl->fh)
     {                                           // file exists, read it
     for (i = 0; i< 5; i++)
      {
       if (fgets (flbuf, 255, inifl->fh))          //change to fn[5] as temp ?? fn is [256].....
        {
         t = strchr(flbuf, ' ');                 // first space
         if (!t) t = strchr(flbuf,'#');          // or comment
         if (!t) t = strchr(flbuf, '\n');        // or newline
         if (t)
            {
             *t = '\0';
             t--;
             if (*t != PATHCHAR) sprintf(t+1, "%c", PATHCHAR);
            }
         flbuf[255] = '\0';                    // safety
         strcpy(fd[i].fn, flbuf);                 // copy path names to relevant files
       }
      }
     fclose(inifl->fh);                 //close leaves handle addr
     inifl->fh = NULL;
     strcpy(fh->path,  fd->fn);         // base path for .bin file
     strcpy(fd[5].fn, fd[2].fn);         //copy wrn path to dbg path

 //    printf("config file OK\n");
    }
  else printf(" - not found, assume local dir\n");

  return 0;
 }


int openfiles(void)
{
  int i, ans;

  ans = 0;


//must find at least the bin file.....
   fldat[0].fh = fopen(fldat[0].fn, fsfx[0].fpars);           // open bin file first
   if (fldat[0].fh == NULL)  return 1;

  for (i = 1; i < numfiles; i++)
   {
    fldat[i].fh = fopen(fldat[i].fn, fsfx[i].fpars);           // open precalc'ed names
    if (fldat[i].fh == NULL)  ans |= (1 <<i);
   }

return ans;
}


void closefiles(void)
{
    // flush and close all files
  int i;
   FDATA *fl;


  for (i=0; i < numfiles; i++)         // include sad.ini
   {
    fl = get_fldata(i);
    if (fl->fh)         // close file
       {
         fflush (fl->fh);
         fclose (fl->fh);
         fl->fh = NULL;
       }
   }
  flhdr.fillen = 0;             //flag marker.

}