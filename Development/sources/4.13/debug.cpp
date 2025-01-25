
//#include  <stdio.h>




#include  "shared.h"
#include  "core.h"
#include  "sign.h"

#ifdef XDBGX

extern int anlpass;
extern HDATA fldata;

extern const char *cmds[];
extern CHAIN chrgst;
extern CHAIN chscan;
extern CHAIN chsubr;
extern CHAIN chsym;
extern CHAIN chsig;
extern CHAIN chcmd;
extern CHAIN chaux;
extern CHAIN chjpf;
extern CHAIN chjpt;
extern CHAIN chadnl;
extern CHAIN chans;
extern CHAIN chdtk;
//extern CHAIN chdtko;
//extern CHAIN chdtkr;
extern CHAIN chdtkd;

extern int tscans;
extern int xscans;

//extern uchar ibuf[];
extern uint  opbit[];
extern BANK bkmap[];
extern OPC opctbl[];



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

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[5], fmt, args);
     va_end (args);
    }
  while (nl--) fprintf(fldata.fl[5], "\n");
  return 1;             // useful for if statements
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

  DBGPRT(0," %05x %05x %s ", b->start, b->end, cmds[b->fcom]);
  va_end (args);
  while (nl--) fprintf(fldata.fl[5], "\n");
}



void DBG_stack (SBK *s, FSTK *t)
 {
  int i;

  if (s && (s->scanning || s->emulating))
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

   DBGPRT(1,"# chrgst num = %d", chrgst.num);

   for (ix = 0; ix < chrgst.num; ix++)
    {
     r = (RST*) chrgst.ptrs[ix];
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

   DBGPRT(1,"# dbg scans chnum = %d blk scans = %d chain scans %d", chscan.num, tscans, xscans);

   for (ix = 0; ix < chscan.num; ix++)
    {
    DBG_sbk("Scan",(SBK*) chscan.ptrs[ix]);

    }
 DBGPRT(1,0);
}


void DBG_dtkx (TRK *k)
{
 // uint start;  //,fend;
  DTD *d;
  uint wop, i;

  wop = opctbl[k->opcix].wop;

  DBGPRT(0,"%5x, %5x, %5s ", k->ofst, k->ofst+k->ocnt, opctbl[k->opcix].name);

  for (i = 0; i <= 4; i++)
    {
     if (i && wop == i) DBGPRT(0,"*"); else DBGPRT(0," ");
     DBGPRT(0,"R%4x ", k->rgvl[i]);
    }

 // if (k->numops > 0) {if (wop == 1) DBGPRT(0,"*"); else DBGPRT(0," "); DBGPRT(0,"R%3x ", k->reg1); } else DBGPRT(0,"    ");
 // if (k->numops > 1) {if (wop == 2) DBGPRT(0,"*"); else DBGPRT(0," "); DBGPRT(0,"R%3x ", k->reg2); } else DBGPRT(0,"    ");
 // if (k->numops > 2) {if (wop == 3) DBGPRT(0,"*"); else DBGPRT(0," "); DBGPRT(0,"R%3x ", k->reg3); } else DBGPRT(0,"    ");


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


//if (k->ofst == 0x92233)
//{
//DBGPRT(1,0);
//}





    d = start_dtdk_loop(k->ofst);

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
  uint ix;         //, f, i;

  DBGPRT(1,0);
  DBGPRT(1,"# ------------ data tracking list");
  DBGPRT(1,"# num items = %d", chdtk.num);

  DBGPRT(1,"from, next, addr, size");
  DBGPRT(1,"# by ofst");

  for (ix = 0; ix < chdtk.num; ix++)
      {
        k = (TRK*) chdtk.ptrs[ix];
        DBG_dtkx (k);
    }





//  DBGPRT(1,"# by register");

 // for (ix = 0; ix < chdtkr.num; ix++)
   //   {
     //   k = (DTK*) chdtkr.ptrs[ix];
       // DBG_dtkx (k);
  //  }

  DBGPRT(1,"# by data");

  for (ix = 0; ix < chdtkd.num; ix++)
      {
        d = (DTD*) chdtkd.ptrs[ix];
        DBGPRT(0,"%5x, %5x, ", d->fk, d->stdat);

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


/*
void DBG_syms (void)
{
 CHAIN *x;
 SYM *t;
 uint ix, b;

 DBGPRT(1,"# ---------Symbols num = %d", chsym.num);

 x = &chsym;

 for (ix = 0; ix < x->num; ix++)
   {
   t = (SYM*) x->ptrs[ix];

   if (!t->noprt)     // not printed as subr or struct
     {
     DBGPRT(0,"sym %x ", t->addb >> 4);
     DBGPRT(0," S%x E%x ", t->rstart, t->rend);

     if (t->cmd) DBGPRT(0," CMD");

     b = t->addb & 0xf;
     if (b) DBGPRT(0," biT %d" , (b-1));

     if (t->xnum) DBGPRT(0," xnum =%d", t->xnum);
     if (t->writ) DBGPRT(0," Write");
     if (t->sys) DBGPRT(0," SYS");
     DBGPRT(0," size %d", t->tsize);
     DBGPRT(0," \"%s\"",t->name);

     DBGPRT(0," ab [%x]", t->addb);
     DBGPRT(1,0) ;
     }
   }
  DBGPRT(1,0);

 }
*/

void DBG_jumps (CHAIN *x, const char *z)
{
 JMP *j;
 uint ix;

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
DBG_jumps (&chjpf,"Forwards");
DBG_jumps (&chjpt,"To ");
}

void DBG_sigs(CHAIN *x)
{
 uint i, ix;
 SIG *y;

 DBGPRT(1,0);
 DBGPRT(1,"-----------Signatures---------[%d]--", x->num);
 DBGPRT(1,"d ofst  end    k    name      pars" );

 for (ix = 0; ix < x->num; ix++)
   {
     y = (SIG*) x->ptrs[ix];
     DBGPRT(0,"%d %5x %5x %2d %7s  ", y->done, y->start, y->end, y->skp, y->ptn->name);
     for (i = 0; i < NSGV; i++) DBGPRT(0,"%5x," , y->v[i]);
     DBGPRT(1,0);
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
    prt_rbt (DBGPRT);
    prt_subs(DBGPRT);
    prt_syms(DBGPRT);
    prt_cmds(DBGPRT);
 //   prt_cmds(&chaux, DBGPRT);
    DBG_dtk();
    DBG_sigs(&chsig);
 }


#endif

