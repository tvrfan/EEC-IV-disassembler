
//#include  <stdio.h>




#include  "shared.h"
#include  "core.h"
#include  "sign.h"



#ifdef XDBGX

extern uint anlpass;
extern HDATA fldata;

extern CHAIN chopcd;
extern CHAIN chadnl;

extern CSTR cmds[];

extern int tscans;
extern int xscans;

//extern uchar ibuf[];
extern uint  opbit[];
extern BANK bkmap[];
extern OPC opctbl[];


CHAIN *get_chain(uint chn);

void prt_glo(SUB *, uint (*) (uint,const char *, ...));
void prt_adt(CHAIN *, void *, int , uint (*) (uint,const char *, ...));
void prt_cmds(uint (*) (uint,const char *, ...));

void prt_subs (uint (*) (uint,const char *, ...));
void prt_syms (uint (*) (uint,const char *, ...));
void prt_banks(uint (*) (uint,const char *, ...));
void prt_rbt  (uint (*) (uint,const char *, ...));

SYM* get_sym(int , uint , int , int);
//DTKD * get_dtkd(CHAIN*,uint,uint);

DTD *start_dtdk_loop(uint);
DTD *get_next_dtkd(DTD *);



  int DBGmcalls   = 0;            // malloc calls
  int DBGmrels    = 0;            // malloc releases
  int DBGmaxalloc = 0;           //  TEMP for malloc tracing
  int DBGcuralloc = 0;



   const char *jtxt[] = {"retp","init","jif","STAT","call", "else"};   // debug text for jump types
   const char *csub[] = {"", "imd", "inr", "inx"};                     //  0 = register (default) 1=imd, 2 = ind, 3 = inx



// how to have an array of subroutine pointers.....this works.

// typedef void DBGPB(CHAIN *, int, void*, const char *fmt);
// DBGPB *chdbgpt [4];


uint DBGPRT (uint nl, const char *fmt, ...)
{  // debug file prt with newlines
  va_list args;
  uint chars;

  chars = 0;
  if (fmt)
    {
     va_start (args, fmt);
     chars = vfprintf (fldata.fl[5], fmt, args);
     va_end (args);
    }
  while (nl--) fprintf(fldata.fl[5], "\n");
  return chars;             // useful for if statements
}


void DBGPBK(int nl, LBK *b, const char *fmt, ...)
{       // debug print for command blk
  va_list args;

  if (!b) return;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[5], fmt, args);
     va_end (args);
    }

  DBGPRT(0," %05x %05x %s ", b->start, b->end, cmds[b->fcom].str);
  va_end (args);
  while (nl--) fprintf(fldata.fl[5], "\n");
}



void DBG_stack (SBK *s, FSTK *t)
 {
  int i;

  if (s && s->proctype)                         //ng || s->emulating))
   {
  if (anlpass >= ANLPRT) return;

  DBGPRT(0,"STK ");
  for (i = 0; i < STKSZ; i++)
   {
    if (!t->type) break;
    DBGPRT (0," [%d]", i);
    DBGPRT (0," R%x %x (%x) T%d W%x P%d", t->reg, t->newadd, t->origadd, t->type, t->psw, t->popped);
    t++;
   }
  DBGPRT(1,0);
   }
 }



void DBG_rgstat(RST *r)
{
    DBGPRT(0,"R%x (%d) %x", r->reg, r->fend, r->ofst);         //ssize
    DBGPRT(0," SR%x", r->sreg);
    if (r->arg) DBGPRT(0," Arg");
    if (r->popped) DBGPRT(0," Pop");
    if (r->inr) DBGPRT(0," INR");
  //  if (r->used) DBGPRT(0," Used");
    if (r->sarg) DBGPRT(0," Sarg");
    if (r->inc) DBGPRT(0," ++");
    if (r->enc) DBGPRT(0," enc %d %x", r->enc, r->data);
    DBGPRT(1,0);
}

void DBG_rgchain(void)
{

  uint ix;
  RST *r;
  CHAIN *x;

  x = get_chain(CHRGST);

   DBGPRT(1,"# chrgst num = %d", x->num);

   for (ix = 0; ix < x->num; ix++)
    {
     r = (RST*) x->ptrs[ix];
     DBG_rgstat(r);
    }
 DBGPRT(1,0);
}


//flags in order of shift 1,2,4,8,10,20,40, 80

const char *flgdesc[] = {"Spans","Within","Front","Rear",
"Overriden by","Overrides","Merge", "Equal",};



void DBGPRTFGS(int ans,LBK *newb,LBK *t)
{
  // debug prt for olchk (next)
  int i,f;


  DBGPRT(0,"Olchk %x for ", ans);
  DBGPBK(0,newb,"Insert");
  i = 0;
  f = 0;
  while (ans)
    {
     if (ans & 1)
     {
       if (f) DBGPRT(0,"|");
       DBGPRT(0, flgdesc[i]);
       f = 1;
     }
     i++;
     ans >>= 1;
    }
  DBGPBK(0,t,0);
  }




void DBG_sbk(const char *t, SBK *s)
{

 if (t) DBGPRT(0,"%s",t);
 DBGPRT  (0," %05x %05x", s->start, s->nextaddr);         // nextaddr is end, effectively

 DBGPRT(0," scnt %d type %d", s->scnt, s->type);

 if (s->substart)  DBGPRT(0," sub %x", s->substart); else DBGPRT(0," No sub   ");
 if (s->caller)    DBGPRT(0, " caller %x (%x)", s->caller->start, s->caller->nextaddr);
 if (s->emulreqd)  DBGPRT(0," EMURQ");
 if (s->args)      DBGPRT(0," ARGS");
 if (s->php)       DBGPRT(0," PSP");
// if (s->cmd)       DBGPRT(0," cmd");
 if (!s->logdata)  DBGPRT(0," NDAT");
 if (s->inv)       DBGPRT(0," INV");
 //if (!s->stop)     DBGPRT(0," NOT Scanned");
   if (!s->stop)     DBGPRT(0," Scanned");
// if (s->gapchk)    DBGPRT(0," GAP");

 DBGPRT(1,0);
}


void DBG_scans(void)
{
  uint ix;
  CHAIN *x;

  x = get_chain(CHSCAN);

   DBGPRT(1,"# dbg scans chnum = %d blk scans = %d chain scans %d", x->num, tscans, xscans);

   for (ix = 0; ix < x->num; ix++)
    {
    DBG_sbk("Scan",(SBK*) x->ptrs[ix]);

    }
 DBGPRT(1,0);
}


void DBG_opc(void)
{
  uint ix, i;
  CHAIN *x;
  TOPC *c;

  x = &chopcd;

   DBGPRT(1,"# opcodes num = %d", x->num);

   i = 0;
   for (ix = 0; ix < x->num; ix++)
    {
      c = (TOPC*) x->ptrs[ix];
      DBGPRT(0,"%x ",c->ofst);
      i++;
      if (i >=20)
      {
          DBGPRT(1,0);
          i = 0;
      }


    }
 DBGPRT(3,0);
}







//change to per operand


void DBG_dtkx (TRK *k)
{
 // uint start;  //,fend;
  DTD *d;
  uint i;

  DBGPRT(0,"%5x, (%5x), %5s ", k->ofst, k->ofst+k->ocnt, opctbl[k->opcix].name);

  DBGPRT(0,"%-4x ", k->rgvl[0]);
  for (i = 1; i <= 3; i++)
    {





        if (i == 3) DBGPRT(0,"w"); else DBGPRT(0," ");
     DBGPRT(0,"R%-3x ", k->rgvl[i]);
    }
   DBGPRT(0,"S%-3x ", k->rgvl[4]);


  switch(k->opcsub)
    {
      case 0:
        DBGPRT(0, "reg ");
        break;
      case 1:
        DBGPRT(0, "imd ");
        break;
      case 2:
        DBGPRT(0, "inr [R%x]",k->rgvl[1]);
        break;
      case 3:
    //     if (k->rbsx) DBGPRT(0, "rbs "); else
         DBGPRT(0, "inx ");
         if (k->rgvl[0] & 0x8000) DBGPRT(0, "[-%x]", 0x10000 - k->rgvl[0]);
         else DBGPRT(0, "[+%x]", k->rgvl[0]);
        break;
      default:
        break;
    }
    if (k->ainc) DBGPRT(0, " +%x", k->ainc);


    d = (DTD*) chmem(&chdtko);
    if (d) d->ofst = k->ofst;

    while ((d = get_next_dtkd(d)))
      {
        DBGPRT(0," %5x", d->stdat);
    //    if (d->olp) DBGPRT(0,"O");
        if (d->psh) DBGPRT(0," P");
        if (d->gap) DBGPRT(0," G%d",d->gap);
        if (d->olp) DBGPRT(0," O");
        DBGPRT(0,",");
      }
    DBGPRT(1,0);
  }


const char* opct [] = {"reg", "imd", "inr", "inx"};






void DBG_dtk (void)
{

  TRK *k;
  DTD *d;
  uint ix;
  CHAIN *x;

  x = get_chain(CHDTK);

  DBGPRT(1,0);
  DBGPRT(1,"# ------------ data tracking list");
  DBGPRT(1,"# num items = %d", x->num);

  DBGPRT(1,"from, next, addr, size");
  DBGPRT(1,"# by ofst");

  for (ix = 0; ix < x->num; ix++)
      {
        k = (TRK*) x->ptrs[ix];
        DBG_dtkx (k);
    }





//  DBGPRT(1,"# by register");

 // for (ix = 0; ix < chdtkr.num; ix++)
   //   {
     //   k = (DTK*) chdtkr.ptrs[ix];
       // DBG_dtkx (k);
  //  }

  DBGPRT(1,"# by data");

  x = get_chain(CHDTKD);
  for (ix = 0; ix < x->num; ix++)
      {
        d = (DTD*) x->ptrs[ix];
        DBGPRT(0,"%5x, %5x, ", d->ofst, d->stdat);

 /*       f = d->modes;
        i = 0;
  while (f)
    {
       if (f & 1)  DBGPRT(0, "%s ", opct[i]);
       i++;
       f >>= 1;
    }  */

       switch(d->opcsub)
    {
      case 0:
        DBGPRT(0, "reg ");
        break;
      case 1:
        DBGPRT(0, "imd ");
        break;
      case 2:
        DBGPRT(0, "inr ");
        break;
      case 3:
         DBGPRT(0, "inx ");
        break;
      default:
        break;
    }

  DBGPRT(0,"[%d]", d->off);
  if (d->bsze) DBGPRT(0," S%d", d->bsze);
  if (d->gap) DBGPRT(0," G%d", d->gap);
  if (d->inc) DBGPRT(0," I%d", d->inc);
  if (d->psh) DBGPRT(0," PSH", d->gap);
  if (d->hq) DBGPRT(0," Q", d->gap);
  if (d->olp) DBGPRT(0," OLP", d->gap);

DBGPRT(1,0);
      }

   DBGPRT(2,0);
  }



void DBG_adt (void)
{
 CHAIN *x;
 ADT *a;
 uint ix;

 DBGPRT(1,"# ---------Additional num = %d", chadnl.num);

 x = &chadnl;

 for (ix = 0; ix < x->num; ix++)
   {
   a = (ADT*) x->ptrs[ix];

   DBGPRT(0,"fid %lx (%lx) ", a->fid, a);

   DBGPRT(0,"O %d ", a->cnt);
   DBGPRT(0,"D %x ", a->data);
   DBGPRT(0,"E %d ", a->enc);

   /*    else
       if (a->vaddr)
        {
         prt(0,"R ");
        }
       else
       if (a->bank) prt(0,"K %x ", a->bank-1);
       else prt_cellsize(a,prt);

       prt_pfw(a,prt);                         // print fieldwidth if not default
       prt_radix(a,drdx,prt);                  // print radix if not default

       if (a->div) flprt("/ ", a->fldat, prt);
       if (a->fnam) prt(0,"N ");

       #ifdef XDBGX
         if (prt == DBGPRT) prt (0,"[R%x, csz %d] ", a->dreg, cellsize(a));
       #endif
       fid = a;

   DBGPRT(0," S%x E%x ", t->rstart, t->rend);
   //31
uint dreg    : 10 ;    // data destination register for data - for arg processing (0-0x3ff)
uint fstart  : 5 ;     // 0-31 start of sub field (bit number).
uint fend    : 7 ;     // 0-31 end   of sub field (bit number), sign 0x20, write 0x40
uint cnt     : 5 ;     // (O) repeat count, 31 max
uint pfw     : 5 ;     // (P) print min fieldwidth (0-31)

//19
uint bank    : 4 ;     // (K) holds a bank override, zero if none
uint places  : 4 ;     // 0-15 print decimal places (is 7 enough ?) [forces radix 10]
uint enc     : 3 ;     // (E) encoded data type (E) 1-7 (addr & 0xff)
uint prdx    : 2 ;     // (X) print radix 0 = not set, 1 = hex, 2 = dec, 3 bin
uint fnam    : 1 ;     // (N) look for symbol name
uint newl    : 1 ;     // (|) break printout with newline (at start of this level)
uint vaddr   : 1 ;     // (R) value is an addr/pointer to symbol (R) (full address)
uint foff    : 1 ;     // (D) data is an offset address
// uint ans     : 1 ;     // (=) this is ANSWER definition (separate item, for verification) move t spf
uint div     : 1 ;     // (V or /) data is FLOAT, use fldat (union with data)
uint mult    : 1 ;     // (*) data is FLOAT, use fldat (union with data)
uint sbix : 1;         //subchain

// uint write  : 1 ??
} ADT;






     DBGPRT(0," S%x E%x ", t->rstart, t->rend);

     if (t->cmd) DBGPRT(0," CMD");

     b = t->addb & 0xf;
     if (b) DBGPRT(0," biT %d" , (b-1));

     if (t->xnum) DBGPRT(0," xnum =%d", t->xnum);
     if (t->writ) DBGPRT(0," Write");
     if (t->sys) DBGPRT(0," SYS");
     DBGPRT(0," size %d", t->tsize);
     DBGPRT(0," \"%s\"",t->name);

     DBGPRT(0," ab [%x]", t->addb);     */
     DBGPRT(1,0) ;
     }

  DBGPRT(1,0);

 }


void DBG_jumps (uint ch, const char *z)
{
 JMP *j;
 uint ix;
 CHAIN *x;

 x = get_chain(ch);

  DBGPRT(1,"------------ Jump list %s", z);
  DBGPRT(1,"num jumps = %d", x->num);

  for (ix = 0; ix < x->num; ix++)
    {
      j = (JMP*) x->ptrs[ix];
      DBGPRT(0,"%5x->%5x",j->fromaddr, j->toaddr);
      if (j->cmpcnt)  DBGPRT(0," cmp %5x",j->fromaddr - j->cmpcnt); else DBGPRT(0,"          ");
      DBGPRT(0," e %5x", j->fromaddr + j->size);
      DBGPRT(0," %s",jtxt[j->jtype]);
      if (j->bswp)  DBGPRT (0," bswp");
      if (j->back)  DBGPRT (0," back");
   //   if (j->uci)   DBGPRT (0," uci");
  //    if (j->uco)   DBGPRT (0," uco");
      if (j->retn)  DBGPRT (0," retn");

  //    if (j->subloop)   DBGPRT (0," SUB");
  //    if (j->jelse)   DBGPRT (0," ELSE");
//if (j->inloop)   DBGPRT (0," inloop");
//if (j->exloop)   DBGPRT (0," exloop");

      if (j->obkt)  DBGPRT (0," {");
      if (j->cbkt)  DBGPRT (0," }");
      DBGPRT(1,0);
    }
    DBGPRT(1,0);
}


void DBG_jumps ()
{
DBG_jumps (CHJPF,"Forwards");
DBG_jumps (CHJPT,"To ");
}

void DBG_sigs(uint ch)
{
 uint i, ix;
 SIG *y;
 CHAIN *x;

 x = get_chain(ch);

DBGPRT(1,0);
 DBGPRT(1,"-----------Signatures---------[%d]--", x->num);
 DBGPRT(1,"d ofst  end   HN P1 name         pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     DBGPRT(0,"%5x %5x %2d %2d %-10s  ", y->start, y->end, y->hix, y->par1, y->name);

     for (i = 0; i < NSGV; i++)
     {
        if (i == 16)  DBGPRT(0,"\n                              ");
        DBGPRT(0,"%5x," , y->vx[i]);
     }
     DBGPRT(2,0);
    }
 DBGPRT(1,0);
}

/*
void DBG_banks(void)
 {
  uint ix;
  BANK *b;

// bank no , file offset,  (opt) pcstart, (opt)  end file offset, (opt) fillstart


  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, filend, maxpc,  ## minpc, bkend, maxpc, bok,cmd,bkmask, cbnk, fbuf, opbt" );

 // DBGPRT(1,"ibuf %x, opbt %x",ibuf, opbit);
  DBGPRT(1,"opbt %x",opbit);

  for (ix = 0; ix < BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->bok)
       {
         DBGPRT(0,"%2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
         DBGPRT(0,"  %x %x %x %x  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf);
         DBGPRT(0," %5x", b->opbt-opbit );
         DBGPRT(1,0);}
       }
  DBGPRT(1,0);
  DBGPRT(1," Other non bok ---");

  for (ix = 0; ix < BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->fbuf && !b->bok)
        {
         DBGPRT(0,"[ %2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
          DBGPRT(0,"  %x %x %x %x  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf);
         DBGPRT(1,"]");
        }
     }

 DBGPRT(1,0);
}

*/





void DBG_data()
 {

   DBGPRT(1,"max alloc = %d (%dK)", DBGmaxalloc, DBGmaxalloc/1024);
   DBGPRT(1,"cur alloc = %d", DBGcuralloc);
   DBGPRT(1,"mcalls    = %d", DBGmcalls);
   DBGPRT(1,"mrels     = %d", DBGmrels);
   DBGPRT(2,0);
    DBGPRT(1," -- DEBUG INFO --");

    DBG_jumps();

    DBG_scans();
    prt_banks(DBGPRT);
    DBG_rgchain();
    DBG_opc();
    prt_rbt (DBGPRT);
    prt_subs(DBGPRT);
    prt_syms(DBGPRT);
    prt_cmds(DBGPRT);
 //   prt_cmds(&chaux, DBGPRT);
    DBG_dtk();
    DBG_adt();
    DBG_sigs(CHSIG);
 }


#endif

