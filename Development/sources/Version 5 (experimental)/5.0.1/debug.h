
#ifndef _XDEBUG_H

#define _XDEBUG_H 1


#include  "shared.h"


extern int tscans;
extern int xscans;
extern int DBGalloced;



void prt_chain      (uint,uint (*) (uint,cchar *, ...));
void prt_cmds       (uint (*) (uint,cchar *, ...));
void prt_subs       (uint (*) (uint,cchar *, ...));
void prt_banks      (uint (*) (uint,cchar *, ...));
uint prt_spf        (void *, uint , uint (*) (uint,cchar *, ...));

const OPC* get_opc_entry (uchar);



uint    get_anlpass      (void);
FILE   *get_file_handle  (uint);
MATHX  *get_mterm        (void *, int);
char   *get_mathfname    (uint);
FKL    *get_link         (void *, void *, uint);
uint    get_lastix       (uint);
cchar  *get_cmd_str      (uint);

void   *get_next_item    (uint, uint *);
CHAIN  *get_chain        (uint);
RST    *get_rgstat       (uint);
uchar   get_fsize        (uint);
uint    max_reg          (void);
RPOP   *get_rpop        (uint);

#endif