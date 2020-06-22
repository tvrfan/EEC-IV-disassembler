
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
extern CHAIN chdata;
extern CHAIN chcode;
extern CHAIN chjpf;
extern CHAIN chjpt;
extern CHAIN chadnl;
extern CHAIN chans;

extern int tscans;
extern int xscans;

extern uchar ibuf[];
extern uint  opbit[];
extern BANK bkmap[];



void prt_glo(SUB *, int (*) (int,const char *, ...));
void prt_adt(CHAIN *, void *, int , int (*) (int,const char *, ...));
void prt_cmds(CHAIN *, int (*) (int,const char *, ...));
SYM* get_sym(int , uint , int , int);





  int DBGmcalls   = 0;            // malloc calls
  int DBGmrels    = 0;            // malloc releases
  int DBGmaxalloc = 0;           //  TEMP for malloc tracing
  int DBGcuralloc = 0;



   const char *jtxt[] = {"retp","init","jif","STAT","call", "else"};   // debug text for jump types
   const char *csub[] = {"", "imd", "inr", "inx"};                     //  0 = register (default) 1=imd, 2 = ind, 3 = inx

// names for chains for debug prtouts, and debug block printers...

   const char *chntxt[] = {"jump from", "jump to", "symbol", "rbase", "sign", "code", "data",
                            "scan", "emulscan", "subscan", "gapscan", "subroutines", "adnl data", "regstat", "spf", "monblk"};

// how to have an array of subroutine pointers.....this works.

// typedef void DBGPB(CHAIN *, int, void*, const char *fmt);  
// DBGPB *chdbgpt [4];
 

int DBGPRT (int nl, const char *fmt, ...)
{  // debug file prt with newlines
  va_list args;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[6], fmt, args);
     va_end (args);
    }
  while (nl--) fprintf(fldata.fl[6], "\n");
  return 1;             // useful for if statements
}


void DBGPBK(int nl, LBK *b, const char *fmt, ...)
{       // debug print for command blk
  va_list args;

  if (!b) return;

  if (fmt)
    {
     va_start (args, fmt);
     vfprintf (fldata.fl[6], fmt, args);
     va_end (args);
    }

  DBGPRT(0," %05x %05x %s ", b->start, b->end, cmds[b->fcom]);
  va_end (args);
  while (nl--) fprintf(fldata.fl[6], "\n");
}



void DBG_stack (FSTK *t)
 {
   int i;

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



void DBG_rgstat(RST *r)
{
    DBGPRT(0,"R%x Z %d", r->reg, r->ssize);
    DBGPRT(0," SREG %x", r->sreg);
    if (r->arg) DBGPRT(0," Arg");
    if (r->popped) DBGPRT(0," Pop");
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
 if (s->cmd)       DBGPRT(0," cmd");
 if (s->nodata)    DBGPRT(0," ND");
 if (s->inv)       DBGPRT(0," INV");
 if (!s->stop)     DBGPRT(0," NOT Scanned");
// if (!s->stopcd)     DBGPRT(0," Call Scanned"); 
 if (s->gapchk)    DBGPRT(0," GAP");

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


void DBG_subs (void)
{
  SUB *s;
  SYM *x;
  uint ix;

  DBGPRT(1,0);
  DBGPRT(1,"# ------------ Subroutine list");
  DBGPRT(1,"# num subs = %d", chsubr.num);

  for (ix = 0; ix < chsubr.num; ix++)
      {
        s = (SUB*) chsubr.ptrs[ix];

        DBGPRT(0,"sub  %x  ", s->start);
        x = get_sym(0, s->start,-1, 0);
        if (x) {DBGPRT(0,"%c%s%c  ", '\"', x->name, '\"');
               //  if (x->xnum > 1)   DBGPRT(0," N%d", x->xnum);
        }
        prt_glo(s, DBGPRT);
        prt_adt(&chadnl,s,0,DBGPRT);
        prt_adt(&chans, s,0,DBGPRT);
        if (s->cmd)   DBGPRT(0,"# cmd");
        DBGPRT(1,0);
      }

   DBGPRT(2,0);
  }

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
      DBGPRT(0," cmp %5x e %5x",j->fromaddr - j->cmpcnt, j->fromaddr + j->size);
      DBGPRT(0," %s",jtxt[j->jtype]);
      if (j->bswp)  DBGPRT (0," bswp");
      if (j->back)  DBGPRT (0," back");
   //   if (j->uci)   DBGPRT (0," uci");
  //    if (j->uco)   DBGPRT (0," uco");
      if (j->retn)  DBGPRT (0," retn");

      if (j->jor)   DBGPRT (0," OR");
      if (j->jelse)   DBGPRT (0," ELSE");




      if (j->cbkt)  DBGPRT (0," }");
      if (j->obkt)  DBGPRT (0," {");
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


void DBG_banks(void)
 {
  uint ix;
  BANK *b;

  DBGPRT(1,0);
  DBGPRT(1,"-------DEBUG BANKS---------");
  DBGPRT(1,"bank filstart, filend, maxpc,  ## minpc, bkend, maxpc, bok,cmd,dbnks, btype ||(->) fbuf, opbt," );

  DBGPRT(1,"ibuf %x, opbt %x",ibuf, opbit);
  for (ix = 0; ix <= BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->bok)
       {
         DBGPRT(0,"%2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
         DBGPRT(0,"  %x %x %x %x   offx  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf-ibuf);
     //       DBGPRT(0,"  %x %x %d%d%d%d %x   offx  %5x", b->bok, b->cmd,b->b0, b->b1,b->b8,b->b9, b->cbnk, b->fbuf-ibuf);
         if (ix < BMAX)   DBGPRT(0," %5x", b->opbt-opbit ); else    DBGPRT(0," %5x", b->opbt );
         DBGPRT(1,0);}
       }
  DBGPRT(1,0);
  DBGPRT(1," Other non bok ---");

  for (ix = 0; ix <= BMAX; ix++)
    {
      b = bkmap + ix;
      if (b->fbuf && !b->bok)
        {
         DBGPRT(0,"[ %2d, %5x %5x %5x", ix, b->filstrt, b->filend, b->maxpc);
         DBGPRT(0," ## %5x %5x", b->minpc, b->maxbk);
          DBGPRT(0,"  %x %x %x %x   offx  %5x", b->bok, b->cmd,b->bkmask, b->cbnk, b->fbuf-ibuf);
    //     DBGPRT(0,"  %x %x %d%d%d%d %x   offx  %5x", b->bok, b->cmd,b->b0, b->b1,b->b8,b->b9, b->cbnk, b->fbuf-ibuf);
         DBGPRT(1,"]");
        }
     }

 DBGPRT(1,0);
}







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
    DBG_banks();
    DBG_rgchain();
    DBG_subs();
    DBG_syms();
    prt_cmds(&chcode,DBGPRT);
    prt_cmds(&chdata, DBGPRT);
    DBG_sigs(&chsig);
 }


#endif

