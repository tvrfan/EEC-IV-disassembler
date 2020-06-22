
#include  "core.h"             // this calls shared.h


extern HDATA fldata;                 // files data holder (in shared.h)
extern DIRS dirs[];

extern uint minwreg, maxwreg;        // min and max ram register address (for checking)
extern uint stkreg1, stkreg2;        // stack registers, 2 stacks for 8065
extern int anlpass;
extern int   cmdopts;
extern uint lowscan;

extern uint casz[];
extern const char *cmds[];

extern INST cinst;

void scan_blk(SBK*, INST*);

int wnprt (int, const char *, ...);
bool val_data_addr(uint);
bool val_code_addr(uint);
bool val_jmp_addr(uint);
uint maxbkadd (uint);
int g_byte (uint addr);
int new_symname (SYM *, const char *);

SYM *new_autosym (uint, int);

int check_sfill(uint addr);

uint find_opcode(int d, uint xofst, OPC **opl);


#ifdef XDBGX

extern const char *chntxt[];
extern  int DBGrecurse;
extern  int DBGnumops;  
   
extern  int DBGmcalls;            // malloc calls
extern  int DBGmrels;            // malloc releases
extern  int DBGmaxalloc;           //  TEMP for malloc tracing
extern  int DBGcuralloc;

extern const char *jtxt[];  
  int DBGPRT (int, const char *, ...);  
  void DBGPRTFGS(int ,LBK *,LBK *);  
  void DBGPBK(int, LBK *, const char *, ...);
  
#endif



int schblk[64];               // search block, used for find - overlayed with whatever struct
                              // declared int so that it can also hold address lists
void *shuffle[16];            // for chain reorganises (and deletes), a block of void pointers



/**********************************************************************************
* CHAIN declarations, used ordered storage with for binary chop searches
*
* CHAIN structures use common find and insert mechanisms with defined subroutines for
* each chain type, Each type has its own structure associated with it.
* searched with 'binary chop' type algorithm.
* CHAIN params = num entries, num ptrs, allocsize, entrysize, maxptrs, num mallocs, ptr array,
* subroutines - free, compare, equals, (debug)
***********************************************************************************/

// Free block

void fsym(void *); void fsub(void *);
void fcmd(void *); void fblk(void *);

// compare, for binary chop searches

int cpjmp  (CHAIN *,uint, void *);   int cpbase (CHAIN *,uint, void *);
int cpcmd  (CHAIN *,uint, void *);   int cpscan (CHAIN *,uint, void *);
int cpsub  (CHAIN *,uint, void *);   int cpsym  (CHAIN *,uint, void *);
int cpsig  (CHAIN *,uint, void *);   int cpsscn (CHAIN *,uint, void *);
int cprgs  (CHAIN *,uint, void *);   int cpadt  (CHAIN *,uint, void *);
int cpspf  (CHAIN *,uint, void *);   int cppsw  (CHAIN *,uint, void *);

// equals for chain inserts and finds where extra logic is required over compare

int eqcmd  (CHAIN *,uint, void *);   int eqbase (CHAIN *,uint, void *);
int eqsym  (CHAIN *,uint, void *);   int eqsig  (CHAIN *,uint, void *);

//void dbglbk (CHAIN *,int, void *, const char *);




CHAIN chjpf   = {0,0,200,sizeof(JMP),   0,0,0,0,0, 0    ,cpjmp  ,cpjmp   };      // jump from
CHAIN chjpt   = {0,0,200,0          ,   0,0,0,0,0, 0    ,cpjmp  ,cpjmp   };      // jump to -  REMOTE (JMP) ptrs (to chjpf)

CHAIN chsym   = {0,0,200,sizeof(SYM),   0,0,0,0,0, fsym ,cpsym  ,eqsym   };      // syms read

CHAIN chbase  = {0,0,200,sizeof(RBT),   0,0,0,0,0, 0    ,cpbase ,eqbase  };      // rbase
CHAIN chsig   = {0,0, 20,sizeof(SIG),   0,0,0,0,0, 0    ,cpsig  ,eqsig   };      // signature

CHAIN chcode   = {0,0,200,sizeof(LBK),  0,0,0,0,0, fcmd ,cpcmd  ,eqcmd   };      // code commands
CHAIN chdata   = {0,0,200,sizeof(LBK),  0,0,0,0,0, fcmd ,cpcmd  ,eqcmd   };      // data commands

CHAIN chscan  = {0,0,200,sizeof(SBK),   0,0,0,0,0, 0    ,cpscan ,cpscan  };      // scan
CHAIN chemul  = {0,0, 20,sizeof(SBK),   0,0,0,0,0, 0    ,cpscan ,cpscan  };      // emu scan
CHAIN chsbcn  = {0,0, 20,0          ,   0,0,0,0,0, 0    ,cpsscn ,cpsscn  };      // sub scan - REMOTE (SBK)
CHAIN chsgap  = {0,0, 20,0          ,   0,0,0,0,0, 0    ,cpsscn ,cpsscn  };      // gap scan - REMOTE (SBK)

CHAIN chsubr  = {0,0, 30,sizeof(SUB),   0,0,0,0,0, fsub ,cpsub  ,cpsub   };      // subr

CHAIN chadnl  = {0,0, 50,sizeof(ADT),   0,0,0,0,0, 0    ,cpadt  ,cpadt   };      // additional data

CHAIN chans   = {0,0, 50,sizeof(ADT),   0,0,0,0,0, 0    ,cpadt  ,cpadt   };      // subroutine answers

CHAIN chrgst  = {0,0, 20,sizeof(RST),   0,0,0,0,0, 0    ,cprgs  ,cprgs   };      // register status (for args)

CHAIN chspf   = {0,0, 10,sizeof(SPF),   0,0,0,0,0, 0    ,cpspf  ,cpspf   };      // special funcs (subroutines)

CHAIN chpsw   = {0,0, 20,sizeof(PSW),   0,0,0,0,0, 0    ,cppsw  ,cppsw   };      // psw setter

CHAIN mblk    = {0,0, 50,sizeof(MBL),   0,0,0,0,0, fblk  ,0      ,0      };      // CHAIN mallocs (for cleanup)


// CHAIN Pointer array for neater admin and external routines.

CHAIN *chindex[] = { &chjpf, &chjpt , &chsym,  &chbase, &chsig,
                    &chcode, &chdata, &chscan, &chemul,  &chsbcn, &chsgap, &chsubr , &chadnl,  &chans, &chrgst, &chspf, &chpsw, &mblk};



void mfree(void *addr, size_t size)
 {
     // free malloc with tracking
  if (!addr) return;     // safety

  #ifdef XDBGX
   DBGcuralloc -= size;    // keep track of mallocs
   DBGmrels++;
  #endif

  free (addr);
//  addr = 0;
 }


void* mem(void *ptr, size_t osize, size_t nsize)
{  // malloc with local tracking
  void *ans;
  if (nsize < 1) wnprt(1,"Invalid malloc size");

  #ifdef XDBGX
  if (ptr)  DBGcuralloc -= osize;
  DBGmcalls++;
  DBGcuralloc += nsize;
  if (DBGcuralloc > DBGmaxalloc) DBGmaxalloc = DBGcuralloc;
  #endif

  ans = realloc(ptr, nsize);

  if (!ans) wnprt(1,"Run out of malloc space");
  return ans;
}





void adpch(CHAIN *);    // declared for logblk

void logblk(void *blk, size_t size)
{
 MBL *b;

 if (!blk) return;

 if ((mblk.pnum - mblk.num) < 2) adpch(&mblk);    // need more pointers

 b = (MBL*) mblk.ptrs[mblk.num];  // no point in using chmem
 b->blk = blk;
 b->size = size;
 mblk.num ++;
 if (mblk.num > mblk.DBGmax) mblk.DBGmax = mblk.num;      // for debug really
}

 //----------------------------------------------------------------
 // add pointers for chains. malloced entries
 // pointer block is realloced, so is continuous...
 // but data blocks are NOT....
 //----------------------------------------------------------------

void adpch(CHAIN *x)
 {   // add pointers for more chain pointers (malloced entries)
     // pointer block is realloced, so is always continuous.
     // data blocks are not.

    int old, nw, msz,i;
    char *z, *n;

    // grow pointer array - always as single block for arrays
    old = x->pnum;
    x->pnum += x->asize;
    x->DBGallocs++;

    nw = x->pnum;            // new size
    x->ptrs = (void**) mem(x->ptrs, old*sizeof(void*), nw*sizeof(void*));

    // allocate new entries if not a remote chain, and save in mbl chain

    n = NULL;
    msz = 0;

    if (x->esize)
     {
      msz = x->esize*x->asize;           // size of new data block to go with ptrs
      n =  (char *) mem(0,0,msz);
      if (!n) wnprt(0,"FATAL - MALLOC ERROR ");
     }

    z = n;
    for (i = old;  i < nw; i++)
     {
      x->ptrs[i] = z;
      z += x->esize;
     }
     
   logblk(n, msz);

 }


void* chmem(CHAIN *x)
 {
    // get a free entry of the chain for search or insert.
    // this is at x->num (next free entry)

  void *ptr;

  if (!x->esize) return NULL;              // can't have memory on a remote chain...

  if ((x->pnum - x->num) < 2) adpch(x);    // need more pointers

  ptr = x->ptrs[x->num];                  // use next free block (at end of list)
  memset(ptr,0, x->esize);                 // and clear it
  return ptr;
 }

void* schmem(void)
 {
    // get a free block for chain searches, from local reserved block.
    // when use of next free chain block is not safe

  memset(schblk,0, 128);           // clear block
  return schblk;
 }


void chinsert(CHAIN *x, int ix, void *blk)
{
  // insert new pointer (= blk) at index ix
  // relies on main pointer block being contiguous and blk being x->num;

 if (!blk) return ;

 if ((x->pnum - x->num) < 2) adpch(x);    // need more pointers

 // move entries up one from ix to make a space.
 memmove(x->ptrs+ix+1,x->ptrs+ix, sizeof(void*) * (x->num-ix));  // fastest for contiguous

 x->ptrs[ix] = blk;                              // insert new block (x->num) at ix
 x->num++;

 #ifdef  XDBGX
 if (x->num > x->DBGmax) x->DBGmax = x->num;     // for debug
 #endif


}


void chdelete(CHAIN *x, uint ix, int num)
{
 // drop num block(s) from list at ix, shuffle down rest of
 // list and restore ptr(s) after x->num.
 // relies on main pointer block being contiguous
 // shuffle array to hold pointers (max 16)
 // extra +1 in shuffle is for x->num block (used by chmem)

 uint i;

 if (ix >= x->num) return;

/* #ifdef XDBGX
 for (i = ix;  i < ix+num; i++)
   {
    if (x->dbgpt)  x->dbgpt(x, ix,0, "delete");
   }
 #endif  */

 if (x->cfree)
   {
    for (i = ix;  i < ix+num; i++) x->cfree(x->ptrs[i]);
   }

 // save pointers about to be 'deleted' (released) in temp array
 memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);

 x->num -= num;

 // move entries down by 'num' from ix+num to overwrite released entries and chmem() pointer.
 memmove(x->ptrs+ix, x->ptrs+ix+num, sizeof(void*) * (x->num-ix+1));

 // put released pointers back after new x->num position so they become unused
 memcpy(x->ptrs + x->num+1, shuffle, sizeof(void*) * num);

}




void fblk(void *x)
{           // malloced blocks tracker (free'd last)
  MBL *k;
  k = (MBL *) x;
  mfree(k->blk, k->size);
}

void clearchain(CHAIN *x)
{
  // clear pointer list contents, but not pointers themselves. Free any attached structures
  //  NB.  count DOWN so that for mblks first entry (which points to itself) is released last.

 int i;

 for (i = x->num-1; i >=0; i--)
    {
     if (x->cfree)  x->cfree(x->ptrs[i]);
    }

 x->num = 0;
}


void freeptrs(CHAIN *x)
{
    // free the master pointer block of chain x (after clearchain does entries)
 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}

void freechain(int ix)
{
  CHAIN *x;

  x = chindex[ix];

  #ifdef XDBGX
    int size;
    size =  x->pnum * sizeof(void *);     // number of void* pointers
    size += x->pnum * x->esize;           // plus structs pointed to
    DBGPRT(1,"max=%5d tot=%5d allocs=%2d esize=%3d size=%6d   %s", x->DBGmax, x->pnum, x->DBGallocs, x->esize, size, chntxt[ix]);
  #endif

  clearchain(x);
  freeptrs(x);
}

void  free_structs ()
{                        // complete tidyup for closing or restart
 uint i;

 for (i = 0; i < NC(chindex); i++) freechain(i);           // this includes mblock

   #ifdef XDBGX
    DBGPRT(1,"max alloc = %d (%dK)", DBGmaxalloc, DBGmaxalloc/1024);
    DBGPRT(1,"cur alloc = %d", DBGcuralloc);
    DBGPRT(1,"mcalls = %d", DBGmcalls);
    DBGPRT(1,"mrels  = %d", DBGmrels);
    DBGPRT(1,"Max recurse = %d", DBGrecurse);
    DBGPRT(1,"Num Opcodes = %d", DBGnumops);
   #endif

   fflush (fldata.fl[1]);            // flush output files (safety)
   fflush (fldata.fl[2]);


}



//---------------------------------------------------------------------------------------/

// CHAIN Compare  procs used by bfindix for binary chop chain searches
// Straight subtract works nicely for chain ordering, even for pointers
// < 0 less than, 0 equals, > 0 greater than

int cpjmp (CHAIN *x, uint ix, void *d)
{
  int ans;
  JMP *j, *t;

  if (ix >= x->num) return -1;

  j = (JMP*) x->ptrs[ix];
  t = (JMP*) d;

  // 'from' is unique, but 'to' is not.

  if (x == &chjpt)
   {    // to chain
     ans = j->toaddr - t->toaddr;
     if (ans) return ans;

     if (t->fromaddr) ans = j->fromaddr - t->fromaddr;
     else
       {    // get to minimum matching 'to' if no 'from' specified
        if (ix > 0)
          {
           j = (JMP*) x->ptrs[ix-1];
           if (j->toaddr == t->toaddr) ans = 1; //go back
          }
       }
   }
  else
   {     // from chain (only ever ONE 'from')
     ans = j->fromaddr - t->fromaddr;
     if (ans) return ans;
   }

  return ans;
}


int cpsym (CHAIN *x, uint ix, void *d)
{
 SYM *s, *t;
 int ans;
 if (ix >= x->num) return -1;

 s = (SYM*) x->ptrs[ix];
 t = (SYM*) d;

 ans = s->addb - t->addb;      // address and bitno combined
 if (ans) return ans;

 ans = s->writ - t->writ;      // and write ??
 if (ans) return ans;

 return s->rend-t->rstart;       // address range check (use end)

}


int cpbase (CHAIN *x, uint ix, void *d)
{
 RBT *b, *t;
 int ans;

 if (ix >= x->num) return -1;
 b = (RBT*) x->ptrs[ix];
 t = (RBT*) d;

  ans = b->reg - t->reg;
  if (ans) return ans;

//use end for address range check
  ans = b->end - t->start;
  return ans;
}


int cpsig (CHAIN *x, uint ix, void *d)
{
 SIG *g, *t;
 int ans;

 if (ix >= x->num) return -1;
 g = (SIG*) x->ptrs[ix];
 t = (SIG*) d;

 ans =  g->end - t->start;
 if (ans) return ans;

 if (t->ptn) ans = g->ptn - t->ptn;
 return ans;
}


int cprgs (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

  RST  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (RST*) x->ptrs[ix];
 t = (RST*) d;

 ans = s->reg - t->reg;

 return ans;

}

int cpspf (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 SPF  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SPF*) x->ptrs[ix];
 t = (SPF*) d;

// ans = ss - tt;

 ans = (long) s->fid - (long) t->fid;

// if (ans) return ans;

// ans = s->seq - t->seq;

 return ans;

}

int cpadt (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 ADT  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (ADT*) x->ptrs[ix];
 t = (ADT*) d;

// ans = ss - tt;

 ans = (long) s->fid - (long) t->fid;

 if (ans) return ans;

 ans = s->seq - t->seq;

 return ans;

}


int cpscan (CHAIN *x, uint ix, void *d)
{
   // d (t) is candidate blk

 SBK  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SBK*) x->ptrs[ix];
 t = (SBK*) d;

 ans = s->start - t->start;
 if (ans) return ans;

 // and subroutine for scans

 if (t->substart) ans = s->substart - t->substart;
 if (ans) return ans;
 
 //? and a caller check for chain scan cockup ???
 
// if (!s->caller && t->caller) return -1;  // ??
 
return ans;
}


int cpsscn (CHAIN *x, uint ix, void *d)
{
   // subscan  - d (t) is candidate blk

   // use subroutine address to ensure
   // all branches are scanned

 SBK  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (SBK*) x->ptrs[ix];
 t = (SBK*) d;

 ans = s->substart - t->substart;

 return ans;

}

int cppsw (CHAIN *x, uint ix, void *d)
{
   // subscan  - d (t) is candidate blk

 PSW  *s, *t;
 int ans;

 if (ix >= x->num) return -1;

 s = (PSW*) x->ptrs[ix];
 t = (PSW*) d;

 ans = s->jstart - t->jstart;

 return ans;

}

int cpsub (CHAIN *x, uint ix, void *d)
{
 // add a check here for pushp subrs ?.
 SUB *s, *t;

 if (ix >= x->num) return -1;

 s = (SUB*) x->ptrs[ix];
 t = (SUB*) d;

 //if (s->psp && t->start == s->start+1)  return 0;     // pushp match

 return s->start - t->start;
}


int cpcmd (CHAIN *x, uint ix, void *d)
{
 // this compare checks END address with start, for address ranges
 // therefore [ix].end >= d->start   ALWAYS.
 // so  [ix].start <= d.start <= [ix].end


 // so just need to check [ix].start < d.start
 // at ix+1 might be the matching block, but
 // ix will span addr

 LBK *b, *t;
 int ans;

 if (ix >= x->num) return -1;
 b = (LBK*) x->ptrs[ix];
 t = (LBK*) d;

 ans = b->end - t->start;

 return ans;

}


int eqbase (CHAIN *x, uint ix, void *d)
{
 RBT *b, *t;
 int ans;

 if (ix >= x->num) return -1;
 b = (RBT*) x->ptrs[ix];
 t = (RBT*) d;

 ans = b->reg - t->reg;
 if (ans) return ans;

 if (t->start >= b->start && t->start <= b->end ) return 0;

 return b->end-t->start;
}


int eqsym (CHAIN *x, uint ix, void *d)
{
 SYM *s, *t;
 int ans;
 if (ix >= x->num) return -1;

 s = (SYM*) x->ptrs[ix];
 t = (SYM*) d;

 ans = s->addb - t->addb;        // includes bitno
 if (ans) return ans;

  ans = s->writ - t->writ;      // and write ??
  if (ans) return ans;

// check address range if specified.
 if (t->rstart >= s->rstart && t->rstart <= s->rend ) return 0;

 return s->rend-t->rstart;



}


int eqsig (CHAIN *x, uint ix, void *d)
{
//cpsig with extra check for skips

// CHANGE THIS to use END addresses, like cmd, to allow for skips at front

 SIG *g, *t;
 int ans;

 if (ix >= x->num) return -1;
 g = (SIG*) x->ptrs[ix];
 t = (SIG*) d;

 ans = 1;

 // allow for skips at front.........
 if (g->start <= t->start && (g->start + g->skp) >= t->start) ans = 0;
 if (!ans && t->ptn) ans = g->ptn - t->ptn;
 return ans;

}



int eqcmd (CHAIN *x, uint ix, void *d)
{
 // compare checks END address, not start
 // so that end is always less than start
 // in previous block (= ix-1)
 // so this block is right candidate to check

// change back, so t->start >= b->start


 LBK *b, *t;

 if (ix >= x->num) return -1;

 b = (LBK*) x->ptrs[ix];
 t = (LBK*) d;

 // t->start will always < b->end ??

 if (t->start >= b->start && t->start <= b->end )
    {
     if (!t->fcom || t->fcom == b->fcom) return 0;
    }
 return 1;
}


uint bfindix(CHAIN *x, void *blk, int (*cmp) (CHAIN *x, uint ix, void *blk))
{
  // generic binary chop.
  // send back an INDEX value to allow access to adjacent blocks.
  // use a compare subr as supplied or default for chain struct.
  // Candidate block is void* and cast to reqd type inside cmp (compare subroutine).
  // answer (min) is always min <= blk, or blk is always >= min (= answer)
  // and min+1 is > blk.   Answer is therefore INSERT point for new block, or FOUND.

  uint mid, min, max;

  if (!cmp) cmp = x->comp;   // use std chain compare subr if not given

  // NB. this loop could work for subsearches too.....

  min = 0;
  max = x->num;

  while (min < max)
    {
      mid = min + ((max - min) / 2);               // safest way to do midpoint (min, max);
      if(cmp(x,mid,blk) < 0) min = mid + 1;        // blk > mid, so mid+1 becomes new minimum
      else  max = mid;                             // blk < mid, so mid is new maximum
    }

  x->lastix = min;                                 // keep last index (even if x->num)
  return min;
}





int olchk(CHAIN *x, uint ix, LBK *newb)
 {

   /***** overlap and overwrite check if - commands can be merged/overwrite etc
   * b is 'candidate' (new) block , t is existing (test) block at ix
   * set answer as bit mask -
   * 1  new block spans  t  (t within newb)
   * 2  new block within t (t spans newb)
   * 4  front overlap or front adjacent
   * 8  rear  overlap or rear  adjacent
   * 10 t overrides newb (newb cannot be inserted)
   * 20 newb overrides t (can overwrite it)
   * 40 merge allowed (flag set and commands match)
   ************/

   LBK *t;
   int ans;

   if (ix > x->num) return 0;

   t = (LBK*) x->ptrs[ix];

   ans = 0;   //clear all flags;

   if (t == newb) return 0;                     // safety check

   // check for equal - already DONE in inschk

   // check for overlaps

   if (newb->start <= t->start && newb->end >= t->end)  ans |= 1; // newb spans t
   else
   if (newb->start >= t->start && newb->end <= t->end)  ans |= 2; // newb is spanned by t
   else
    {      // overlap, front or back
     if (newb->start >= t->start && newb->start <= t->end) ans |= 4;        // overlap front
     if (newb->end   >= t->start && newb->end   <= t->end) ans |= 8;        // overlap rear
    }

   if (ans)
    {       // check overwrite
     if (!t->cmd    && dirs[newb->fcom].ovrsc > dirs[t->fcom].ovrsc) ans |= 0x20;  // newb can overwrite t
     if (!newb->cmd && dirs[newb->fcom].ovrsc < dirs[t->fcom].ovrsc) ans |= 0x10;  // t overwrite newb
    }

   if (dirs[t->fcom].merge && t->fcom == newb->fcom)
    {
     if (ans) ans |= 0x40;                            // overlap set already, merge allowed
     if (newb->end == t->start-1)  ans |= 0x44;       // adjacent to front of test block (merge+front)
     if (newb->start == t->end+1)  ans |= 0x48;       // adjacent to end of test block (merge+rear)
    }

// do the dbg print HERE for both blocks

 #ifdef XDBGX
  if (ans) DBGPRTFGS(ans,newb,t);
 #endif

 return ans;
}







int cprange(CHAIN *x, uint ix, void *d)
{
 // std compare checks END address with start, for address ranges
 // therefore [ix].end >= d->start ALWAYS.
 // this is a special version for checking OVERLAPS.

 // If d.start is specified, find the MINIMUM ix where d.start
 // is overlapped or adjacent, where [ix].end > d.start.

 // If d.end is specified, find the MAXIMUM ix where d.end is
 // is overlapped or adjacent, where [ix].end > d.end. 
 // for LBK structs (data and code).

  LBK *b, *t;
  int ans;
  uint max;

  ans = -1;
  if (ix >= x->num) return -1;

  b = (LBK*) x->ptrs[ix];
  t = (LBK*) d;
  max = x->num-1;

  if (t->end)
     {
      // get to maximum matching 'end', and extra check for adjacent
      ans = b->end - t->end;

      if (!ans && ix < max)
          {      // adjectent check of next block
           b = (LBK*) x->ptrs[ix+1];
           if (b->start <= t->end+1 && b->fcom == t->fcom)
             ans = -1;
          }
      return ans;
     }

  if (t->start)
     {     // extra adjacent check (different logic to END check)
      ans = b->end - t->start;
      if (ans == -1 && ix < max && b->fcom == t->fcom) ans = 0; // adjacent
     }
 return ans;

}


int inschk(CHAIN *x, int ix, int ans, LBK *newb)
{

   /* check if insert is allowed, fix any overlaps later.
   *  newb is insert candidate. check range of possible overlaps.
   * ix is where insert would be (if going recursive....)
   *  probably better to delete blocks because of ADT issues....but causes issue with A9L
   * answer is bit mask from olchck
   * 1  new block spans  t  (t within newb)
   * 2  new block within t  (t spans newb)
   * 4  front overlap or front adjacent
   * 8  rear  overlap or rear  adjacent
   * 10 t overrides newb (newb cannot be inserted)
   * 20 newb overrides t (can overwrite t)
   * 40 merge allowed (flag set and commands match)
   */

  LBK *t;
  uint chkf, min, max;

  // OK, find min and max overlap

  if (x->num == 0) return 1;

  max = bfindix(x,newb,cprange);    // checks end address

  ans = newb->end;
  newb->end = 0;                    // clear end for front check
  min = bfindix(x,newb,cprange);
  newb->end = ans;                  // restore end addr

  if (max >= x->num) max = x->num - 1;

  ans = 1;                            // set 'insert OK'

  while(min <= max && ans)
    {                                 // scan range of possible overlaps
     if (max >= x->num) break;
     if (min >= x->num) break;         // safety for deleted items
    
     chkf = olchk(x, min, newb);      // check chained block for overlap

     if (chkf)
      {
       t = (LBK*) x->ptrs[min];

       if (chkf & 0x20)
        {                    // new block overrides chain block
         if (chkf & 1)
          { //new block spans the existing one (check cmd ?)
            #ifdef XDBGX
            DBGPBK(1,t,"delete1 (%d)", min);
            #endif
            chdelete(x, min, 1);
            //probably better to redo the whole thing ?
            min--;
            max--;                // match is now 1 shorter (but is it ?  min won't be less ??)
          }
         else
         if (chkf & 2)
             { // new block is contained within chain block, but overrides it
               // if front overlaps (4), move front of chain block
               // if end overlaps (8), move end of chain block
            //   NO !!!
        //           ans = 0;          // no insert
       //            return ans;

               if (chkf & 4) t->start = newb->end+1;
               else
               if (chkf & 8) t->end = newb->start-1;
                          // panic here
             }
          else
              { // overlap, can overwrite
               if (chkf & 4)            //F)
                 {    // front of new block overlaps chain block
                  t->end = newb->start-1;     // change end
                 }
              if (chkf & 8)                //R)
                 { // end of new block overlaps start of of chain block
                   t->start = newb->end+1;
                 }
               }
              // more cases ???
          }      // end 0x20
         else
         if (chkf & 0x10)                  //V)
          {
           // chained block overrides new block
             if (chkf & 4) newb->start = t->end+1;
               else
            if (chkf & 8)               //R)
               {
                 newb->end = t->start-1;
               }

           // check start and end ....

            if (chkf & 2 || newb->start < newb->end)
              {      // contains, (or now empty). Reject insert
               ans = 0;          // no insert
       //        return ans;
              }

         }
         else
         if (chkf & 2)                     //C)
           {
            ans = 0;          // no insert
  //          return ans;
           }
         else

       if (chkf & 0x40)         // TEST for span as well ? fewer errors, but....word over byte ??
        {
         // merge allowed, if multiple in this loop,
         // merge new block with chain then stop insert ? drop chain entry

         if (t->start < newb->start) newb->start = t->start;
         if (t->end   > newb->end)   newb->end   = t->end;
        #ifdef XDBGX
         DBGPRT(0,"(%d) ", min);
         DBGPBK(1,t,"delete2");
        #endif
         chdelete(x, min, 1);
         min--;
         max--;                // match is now 1 shorter
        }
       else

// but if TWO merges (i.e. new block spans gap between two chains,
// MUST drop one......



// where is s ?
        {
          // default catch
          #ifdef XDBGX
           DBGPRT(0,"(%d) ",min);
   //        DBGPRTFGS(chkf);
           DBGPBK(1,newb, "DFLT Reject");
           #endif
          ans = 0;           // temp stop insert
        }
  //       #ifdef XDBGX
  //      DBGPRT(1,0);
  //       #endif
      }          // end if chk set
     min++;
    }         // end loop

 return ans;                    // insert if not zero
}




LBK* inscmd(CHAIN *x, LBK *blk)
 {
   /* need extra checks the for address ranges (start->end)

    * at this call 'blk' is always x->ptrs[x->num] ....
    * do a simple check,   THEN do more complex merging as necessary

   this works but does not merge anything yet.  THis assumes you never get an END overlap ?
   probably true for current scan setup, but not safe.....

   eqcmd only checks that start falls between start&end of chained block....

   NB.   Probably don't care about overlaps right now, can do later, but must capture
   any extended END markers.

   so have to check for exact match, and non_allowed overlaps/overrides ?

    */

  int ans;
  uint ix;
  LBK *l;

  ans = 1;

  ix = bfindix(x, blk,0);                 // where to insert (from cpcmd)

  l = (LBK*) x->ptrs[ix];

  // can't use x->equal here, it's too slack - check exact match
  if (ix < x->num && l->start == blk->start && l->end == blk->end && l->fcom == blk->fcom)
    {
      if (!l->sys) x->lasterr = E_DUPL;
      return 0;
    }
    
  ans = inschk(x,ix,ans,blk);              // found - is insert allowed

  if (ans)
      {
       ix = bfindix(x, blk,0);       // find again in case stuff deleted
       chinsert(x, ix, blk);         // do insert at ix
       x->lasterr = 0;                  //lasterr
      }
  else
     {
      blk = (LBK*) x->ptrs[ix];       // overlap. (duplicate above)
      x->lasterr =  E_OVLP;           //what about overlaps ??
     }
 return blk;

}

// match for signature blocks

SIG* inssig(SIG *blk)
 {

  int ans;
  uint ix;
  SIG *s;
 // ans = 1;                  // insert by default redundant

  ix = bfindix(&chsig, blk,0);
  ans = chsig.equal(&chsig,ix,blk);

 // if (!ans) return 0;             // straight duplicate

// check for duplicates with different skips at front...
// and check for overlap with next sig - but sign sig may be OK ?
// may need better check here for overlaps

  if (ix < chsig.num)
   {       // new sig overlaps ix (which is next block in chain)
    s = (SIG*) chsig.ptrs[ix];

   if (s->ptn == blk->ptn && s->start+s->skp == blk->start+blk->skp)
     {      // duplicate, only skips differ...
       ans = 0;
     }
   }

  if (!ans)
     {
      return 0;
     }

  chinsert(&chsig, ix, blk);      // do insert
  return blk;

}



LBK* add_data_cmd(uint start, uint end, uint com, uint from)
 {

   LBK *b;
   uint tcom;

   chdata.lasterr = E_INVA;           // set "invalid address"
   tcom = com & 0x1f;  // drop any extra flags
   
   if (nobank(start) < PCORG) return NULL;         // TEMP for TESTING

   if (end < start) end = start;             // safety

   // special for checksum loop at front
   if (nobank(start) > PCORG && nobank(start) < (PCORG+4)) return NULL;

   if (!val_data_addr(start)) return NULL;
   
   if  (tcom == C_DFLT)
    {  // special for fill 
     if (end > maxbkadd (end)) return NULL;
    }
   else  
   if (!val_data_addr(end))   return NULL;

   if (g_bank(start))
     {
      if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

     // can get requests for 0xffff with bank, which will break boundary....

      if (!bankeq(end, start))
       {
        chdata.lasterr = E_BKNM;           // set "banks don't match"
        return NULL;
       }
     }


   if ((tcom == C_WORD || tcom == C_LONG || tcom == C_VECT) && (start & 1))
    {  
      // must start on even address
      chdata.lasterr = 4; 
      #ifdef XDBGX
         DBGPRT(0,"FAIL add data");
         DBGPRT(0, " %x ODD Boundary from %x", start, from);
   DBGPRT(1,0);
   #endif

              
      return NULL;  
    }

   b = (LBK *) chmem(&chdata);
   b->start = start;
   b->end = end;
   b->from = from;
   b->fcom  = tcom;             // max 31 as set
   if (com & C_CMD) b->cmd = 1;
   if (com & C_SYS) b->sys = 1;

   b = inscmd(&chdata,b);

   #ifdef XDBGX
   if (!chdata.lasterr) DBGPBK (0,b,"add data");
   else   DBGPBK (0,b,"FAIL add data");
   if (from) DBGPRT(0, "from %x", from);
   DBGPRT(1,0);
   #endif

   return b;
}



// general chain finder for all chains

void *find(CHAIN *x, void *b)
{
 // find item with optional extra pars in b

 int ix;
 ix = bfindix(x, b, 0);
 if (x->equal(x,ix,b)) return NULL;    // no match
 return x->ptrs[ix];
}

PSW *add_psw(uint jstart, uint pswaddr)
{
  PSW *x;
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;
  if (!val_code_addr(jstart))  return 0;
  if (!val_data_addr(pswaddr)) return 0;

  x = (PSW *) chmem(&chpsw);
  x->jstart = jstart;
  x->pswop  = pswaddr;

  ix = bfindix(&chpsw, x,0);
  ans = chpsw.equal(&chpsw,ix, x);

  if (ans)
   {
    chinsert(&chpsw, ix, x);
    chpsw.lasterr = 0;
   }

   #ifdef XDBGX
      if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
      DBGPRT(0," pswset %x = %x", jstart, pswaddr);
      DBGPRT(1,0);
     #endif


if (ans) return x;

chpsw.lasterr = E_DUPL;       //duplicate
return 0;
}

PSW* get_psw(int addr)
{
  PSW *r, *x;

  r = (PSW*) schmem();
  r->jstart = addr;
  x = (PSW*) find(&chpsw, r);
  return x;
}

RBT *add_rbase(uint reg, uint addr, uint start, uint end)
{
  RBT *x, *z;     // val includes bank
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;          // set "invalid addr + 2"
  if (reg & 1)        return 0;    // can't have odd ones
  if (reg <= stkreg1) return 0;    // can't have special regs or stack
  if (reg <= stkreg2) return 0;    // 8065 has stack2
  if (reg > maxwreg)  return 0;    // must be register

  //if (addr <= maxwreg) {return 0;}   // can't have register to register adds
  // jsa does this for CARD

  if (addr && !val_data_addr(addr)) return 0;         //allow zero for rbases

  // check range address too----

  if (start && !val_data_addr(start)) return 0;

  x = (RBT *) chmem(&chbase);
  x->reg   = reg;
  x->val   = addr;
  x->start = start;
  x->end   = end;

  ix = bfindix(&chbase, x,0);
  ans = chbase.equal(&chbase,ix, x);

  // eqbase checks for MATCHing range, but not overlap
  // check for overlap here
   // must check x->end does not overlap next block....
   // start MUST be > than end of previous block...

  if (ix < chbase.num)
    {
     z = (RBT *) chbase.ptrs[ix];         // chain block found
     if (x->reg == z->reg && x->end > z->start)
      {
       chbase.lasterr = E_OVRG;          //overlap ranges
       return 0;
      }
     }

  if (ans)
   {
    chinsert(&chbase, ix, x);
    chbase.lasterr = 0;
   }

     #ifdef XDBGX
      if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail");
      DBGPRT(0," rbase R%x = %05x", reg, addr);
      if (start) DBGPRT(0,"(%05x - %05x)", start, end);
      DBGPRT(1,0);
     #endif

if (ans) return x;

chbase.lasterr = E_DUPL;       //duplicate
return 0;


}


RBT* get_rbt(int reg, int pc)
{
  RBT *r, *x;

  r = (RBT*) schmem();
  r->reg = reg;
  r->start = pc;
  x = (RBT*) find(&chbase, r);

// if not found, look for default (addr = 0).....

  if (!x && pc)
    {
     r->start = 0;
     x = (RBT*) find(&chbase, r);
    }

  if (x && x->inv) x = 0;

return x;

}

RST* find_rgstat(uint reg)
{
  uint ix;
  RST *b, *t;

 t = (RST *) chmem(&chrgst);
 t->reg  = reg;

 ix = bfindix(&chrgst, t, 0);
 b = (RST*) chrgst.ptrs[ix];

 if (ix < chrgst.num)
   {       // exact match reqd
    if (b->reg != t->reg) return NULL;
 //   if (t->addr && b->addr != t->addr) return NULL;
    return b;    // must match to get here
   }

 return NULL;

}


void set_rgsize(uint ssize, uint ix)
{
    // need ix anyway so may as wel use it
 RST *r;
 int reg;
// uint ix;

// ix = chrgst.lastix;
   if (ix >= chrgst.num) return;    // safety
   
   r = (RST*) chrgst.ptrs[ix];
   if ((r->ssize & 3) < (ssize & 3))
    {
     r->ssize = ssize;                         // just do size
     #ifdef XDBGX
     DBGPRT(1,"reg %x size = %d", r->reg, r->ssize);
#endif
     // drop upper register if part of reg
     if ((r->ssize & 2) && ix < chrgst.num)
      {
       reg = r->reg;
       ix++;
       r = (RST*) chrgst.ptrs[ix];

       if (r->reg == (reg+1))
         {
                  #ifdef XDBGX
         DBGPRT(1,"del reg %x size = %d", r->reg, r->ssize);
         #endif
         chdelete(&chrgst,ix,1);
         }
      }
    }
}


RST* add_rgstat(uint reg, uint ssize)
{

 RST *x;
 int ix, ans;

 x = (RST *) chmem(&chrgst);
 x->reg  = reg;
// x->addr = addr;

 ix  = bfindix(&chrgst, x,0);

 ans = chrgst.equal(&chrgst,ix, x);

 if (ans)
   {
    chinsert(&chrgst, ix, x);
   }
 else
   {
    x = (RST*) chrgst.ptrs[ix];
    memset(x,0, sizeof(RST));                 // and clear it
    x->reg = reg;
   }

 set_rgsize(ssize,ix);

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT  (0," regstat %x", x->reg);
    DBGPRT(1,0);
   #endif

 return x;

}

SPF* add_spf (void *fid)         //, int seq)
  {
   // add special func block
   // only one at the moment, otherwise need seq

  SPF *a;     //, *x;
  int ix, ans;

  a = (SPF *) chmem(&chspf);

 // a->cnt = 1;
 // a->ssize = 1;                            // single byte unsigned is default
  a->fid = fid;

 // if (seq < 0) seq = 0;                    // safety

 // if (seq > 31) a->seq = 31; else a->seq = seq;

  // check if it exists already

  ix = bfindix(&chspf,a,0);

 /* if (seq > 31)
    {          // check if fid exists before adding
      a->seq = 0;                // default safety
      x = (ADT*) chadnl.ptrs[ix];          //if a = x ...

     if (ix > 0)
       {
        x = (ADT*) chadnl.ptrs[ix-1];
        if (x->fid == fid && x->seq < 31) a->seq = x->seq+1;
       }
    }  */

   ans = chadnl.equal(&chspf,ix, a);

  if (ans)
   {
    chinsert(&chspf, ix, a);
   }
  else
  {
   a = (SPF*) chspf.ptrs[ix];
  }

  chspf.lastix = chspf.num;        //must reset any queries
  return a;
}

SPF *get_spf(void *fid)         //, int seq)
{
 int ix;
 SPF *s;

 s = (SPF*) schmem();
 s->fid = fid;
 //s->seq = seq;

 ix = bfindix(&chspf, s, 0);

 if (!chspf.equal(&chspf,ix,s))
 {
   return (SPF*) chspf.ptrs[ix];
 }

chspf.lastix = chspf.num;
return NULL;
}

ADT *set_adnl(ADT* a)
{
 memset(a,0,sizeof(ADT));                // clear block

 a->cnt = 1;
 a->ssize = 1;                            // single byte unsigned is default
 return a;
}


ADT* add_adt (CHAIN *x, void *fid, int seq)
  {
   // add additional data block to fid
   // if seq >= 128 then look for end of chain, if less then
   // find actual sequence number


  ADT *a;
  int ix, ans;

  a = (ADT *) chmem(x);

  a->cnt = 1;
  a->ssize = 1;                            // single byte unsigned is default
  a->fid = fid;

  if (seq < 0) seq = 0;                    // safety

  // set highest, or requested, sequence
  if (seq > 127) a->seq = 127; else a->seq = seq;

  // check if it exists already

  ix = bfindix(x,a,0);

  if (seq > 127)
    {   // add new block at end with next seq
      ADT *z;      
      a->seq = 0;                          // default safety
      z = (ADT*) x->ptrs[ix];

     if (ix > 0)
       {  // get next seq number
        z = (ADT*) x->ptrs[ix-1];
        if (z->fid == fid && z->seq < 127) a->seq = z->seq+1;
       }
    }

   ans = chadnl.equal(x,ix, a);

  if (ans)
   {
    chinsert(x, ix, a);
   }
  else
  {
   a = (ADT*) x->ptrs[ix];
  }

  x->lastix = x->num;        //must reset any queries
  return a;
}


LBK *find_data_cmd (int start, int fcom)
{
  // returns match if ofst within range of a data block (start-end)
  LBK *blk;
  int ix;

  blk = (LBK*) chmem(&chdata);
  blk->start = start;
  blk->fcom =  fcom;

  ix = bfindix(&chdata, blk, 0);

  if (chdata.equal(&chdata,ix,blk)) return NULL;    // no match

  return (LBK *) chdata.ptrs[ix];
}


RST * find_lowest_rgarg(uint reg)
{
  uint i;
  RST *r, *ans;

  ans = 0;
  for (i = 0; i < chrgst.num; i++)
  {
    r = (RST*) chrgst.ptrs[i];
    if (r->arg && r->sreg == reg)
    {
        ans = r;
        break;
    }
  }
        return ans;
}




SBK *find_scan(uint addr)
{
 SBK *x, *ans;
 int ix;

  x = (SBK*) schmem();              // dummy block for search
  x->start = addr;                  // symbol addr

  ans = NULL;
  ix = bfindix(&chscan, x, 0);
  if (!chscan.equal(&chscan,ix,x))
 ans = (SBK *) chscan.ptrs[ix];

 return ans;
}




SUB *get_subr(int addr)
{
  SUB *s;

  if (!addr) return 0;         // shortcut..
  s = (SUB*) schmem();
  s->start = addr;

  s = (SUB*) find(&chsubr, s);

return s;
}


SBK *get_scan(uint addr, uint subaddr)
{
 int ix;
 SBK *s;

 s = (SBK*) schmem();
 s->start = addr;
 s->substart = subaddr;

 ix = bfindix(&chscan, s, 0);

 if (!chscan.equal(&chscan,ix,s))
    return (SBK*) chscan.ptrs[ix];

return NULL;
}


ADT *get_adnl(CHAIN *x, void *fid, int seq)
{
 int ix;
 ADT *s;

 s = (ADT*) schmem();
 s->fid = fid;
 s->seq = seq;

 ix = bfindix(x, s, 0);

 if (!x->equal(x,ix,s))
 {
   return (ADT*) x->ptrs[ix];
 }

x->lastix = x->num;
return NULL;
}


ADT *get_next_adnl(CHAIN *x, ADT *b)
{
 ADT *ans;
 uint ix;

 ans = NULL;

 if (b->fid && b->seq < 0)
  {    //  a new query
   ans = get_adnl(x,b->fid, 0);
   return ans;
  }

 ix = x->lastix + 1;
 
 while (ix < x->num)
   {
    ans = (ADT*) x->ptrs[ix];
    
    if (ans->seq == b->seq+1)
      {
       x->lastix = ix;         // last valid 
       break;                      // found
      }

    if (ans->fid != b->fid) 
      {
       x->lastix = x->num;   //no last valid
       ans = 0;
       ix = x->num;
      }
    
    ix++;
  }

 if (ix >= x->num) return 0;
 return ans;
}


void free_adnl(CHAIN *x, void *fid)
 {

  ADT *a;
  uint start, end;

  get_adnl(x,fid,0);
  start = x->lastix;
  end = start;

  if (start >= x->num) return;

  while (1)
   {
    end++;
    if (end >= x->num) break;
    a = (ADT*) x->ptrs[end];
    if (a->fid != fid)  break;
    a->fid = 0;                          //temp lashup !!!
   }

  //chdelete(&chadnl,start,end-start);      //but this is done in tidyup anyway


 return;
 }


void fsym(void *x)
{            // free symbol
 SYM *s;

 s = (SYM*) x;
 if (s->tsize)  mfree (s->name, s->tsize);              // free symbol name
 s->name = NULL;
 s->tsize = 0;
}

void fsub(void *x)
 {
  free_adnl (&chadnl,x);         //&(s->adnl));
 }

void fcmd(void *x)
{       // free cmd
  free_adnl (&chadnl,x);         //&(k->adnl));
}

  
int adtchnsize(void *fid, int seq, int nl)
{
 // continue from seq to next newline if nl
 // but must save lastix in case it hits end

 ADT *a;
 int sz, lastix;
 sz = 0;

 a = (ADT*) chmem(&chadnl);
 a->fid = fid;
 a->seq = seq;

 lastix = chadnl.lastix;

 while ((a = get_next_adnl(&chadnl,a)))
   {
    sz +=  (a->cnt * casz[a->ssize]);
    if (nl && a->newl) break;
   }

  chadnl.lastix = lastix;
 return sz;
}



void set_retn(JMP *x)
 {
 // set retn flag for anything which jumps to addr
 // bfindix on to chain will always get FIRST jump
 // called recursively

  uint ix;
  JMP *t;

  if (x->jtype != J_STAT) return;

  t = (JMP*) schmem();
  t->toaddr = x->fromaddr;               // jumps to addr
  ix = bfindix(&chjpt, t, 0);     // TO chain, finds first match

  while (ix < chjpt.num)
   {
    t = (JMP *) chjpt.ptrs[ix];     // match
    if (t->toaddr != x->fromaddr) break;
    // only if 't' (from) is static
    if (t->jtype == J_STAT)
       {
        t->retn = 1;
        set_retn(t);   //->fromaddr);      // check jumps to here
       }
    ix++;
   }
  }



int fwdadds(JMP *j, uint *from, uint *to)
 {
    // return valid to and from, always forwards

  if (!j->jtype) return 1;
  if (j->bswp)   return 1;
  if (j->jtype == J_SUB) return 1;      // ignore subr and return jumps
  
  if (j->retn) return 1;             // TEST !!

  if (j->back)
     {  // backwards jump
      *from = j->toaddr;
      *to   = j->fromaddr;
     }
  else
     {
      *from = j->fromaddr;
      *to   = j->toaddr;
     }
 return 0;
 }

/*
uint faddr(JMP *j)
 {
    // return valid to and from, always forwards

  if (j->back) return j->toaddr;
  return j->fromaddr;
 }

int taddr(JMP *j)
 {
    // return valid to and from, always forwards

  if (j->back) return j->fromaddr;
  return j->toaddr;
}

*/
/*
void outside (JMP* b, JMP *t)
 {
     // jump overlap tester.  b is base, t is test
      // could flag b directly

   uint bfrom, bto;
   uint tfrom, tto;       // need these in case jumps are backwards

   if (b == t)        return;     // don't compare same jump...
   if (fwdadds(b, &bfrom, &bto)) return;
   if (fwdadds(t, &tfrom, &tto)) return;

// outside jumps in

   if (tto > bfrom && tto < bto)
     {
      if (tfrom < bfrom)  b->uci = 1;
      if (tfrom > bto)    b->uci = 1;
     }

/ inside jumps out - allow this as experiment

  if (tfrom > bfrom && tfrom < bto)
     {
      if (!t->jelse) {
      if (tto < bfrom)  b->uco = 1;
      if (tto > bto)    b->uco = 1;
} //  else t->cbkt = 0;
     }


 }

   if (f->jtype == J_COND && !f->retn)
zx = ix+1;                                  // next jump
          t = (JMP*) chjpf.ptrs[zx];
          while (t->fromaddr <= f->toaddr && zx < chjpf.num)
            {
             outside(f, t);
             zx++;
             t = (JMP*) chjpf.ptrs[zx];
            }

void check_jump_span(JMP *j)
 {
  // check if j is within a cond jump (and possibly other cases too)
  // j is candidate for insert
  // get minimum overlap candidate and go forwards

  JMP *x;
  uint ix, oix;

  if (j->jtype != J_STAT) return;

  x = (JMP*) schmem();
  x->toaddr = j->fromaddr;                 // to get to first of overlaps (to >= from)

  oix = bfindix(&chjpt, x, 0);      // TO chain

  if (oix >= chjpt.num)  return;  // not found

  ix = oix;

  // ix is minimum possible match (as  to >= from)

  while (ix < chjpt.num)
   {
      x = (JMP*) chjpt.ptrs[ix];

      if (x != j)
       {
         if (x->jtype == J_COND || x->jtype == J_ELSE)
          {
             // ignore backwards jumps for now
           if (!x->back)
            {
              if (x->fromaddr > j->fromaddr) break;      // no more overlaps
              if (x->fromaddr < j->fromaddr && x->toaddr > j->fromaddr)     // && !x->back
                {          // static is inside a conditional
                   // BUT not if cond only jumps a single goto............... (xdt2)
                 // if (x->to == j->from+j->size) then only jumps over another jump,
                 //  so j remains a STAT

 //                 if (x->to == j->from+j->size && x->from+x->size == j->from )
 //DBGPRT(0,"Ignore ");
//else
                  j->jtype = J_ELSE;
                      #ifdef XDBGX
                  DBGPRT(1,"CJUMP %x-%x spanned by %x-%x", j->fromaddr, j->toaddr, x->fromaddr, x->toaddr);
                  #endif
                  break;
                }
            }
          }
       }
      ix++;
   }
 }
*/

void do_jumps (void)
{

/******* jumps checklist ***************
 set conditions flags for various things
 whether jumps overlap, multiple end points etc etc.
*****************************************/

  uint ix, zx;
//  uint tstart, tend, fstart, fend;    // have to use indexes here as interleaved scan
  uint tend;
  JMP *f, *t;

  // first, check for jumps to return opcodes
  // true ret has jtype 0

  for (ix = 0; ix < chjpf.num; ix++)
    {         // down list of forward jumps
     f = (JMP*) chjpf.ptrs[ix];

     if (f->jtype == J_COND)
      {
       f->obkt = 1;         //assume OK at first
       f->cbkt = 1;
       f->done = 0;
      }

     if (f->jtype)            // == J_STAT)
      {  // static jumps only
       tend = g_byte(f->toaddr);
       if (tend == 0xf0 || tend == 0xf1)
         {
          f->retn = 1;
          set_retn(f);               // recursive...
         }
      }
    }

// check for overlaps in jumps

  for (ix = 0; ix < chjpf.num-1; ix++)
    {
     f = (JMP*) chjpf.ptrs[ix];                // down full list of forward jumps


   //  fend   = f->fromaddr + f->size;           // end of jump
   //  fstart = f->fromaddr - f->cmpcnt;         // start, from cmp if present

      if (f->back)
         {
            f->jdo = 1;
            f->obkt = 0;         // later !!
            f->cbkt = 0;
            f->done = 1;
         } 

     if (f->jtype == J_COND && !f->done)
        { // only for conditional jumps (as base)
          // try to sort out heirarchy for whether brackets can be used
          // fromaddr is unique, but toaddr is not
          // in order of FROM...
          
           zx = ix+1;                                  // next jump
           t = (JMP*) chjpf.ptrs[zx];

           while (zx < chjpf.num)
             {      // inside -> outside (same chain)
              if (t->fromaddr < f->toaddr)
                {
                 if (t->toaddr > f->toaddr) t->obkt = 0;
                 if (t->toaddr < f->fromaddr) t->obkt = 0;
                }




    /*   skip these for now...  
              //  check for 'else' and 'or' etc.
              tend   = t->fromaddr + t->size;           // end of jump
              tstart = t->fromaddr - t->cmpcnt;

              if (t->jtype == J_STAT && tend == f->toaddr && tstart > fend && !t->back)
               {
                t->jelse = 1;          //type = J_ELSE;  
               t->cbkt = 1;
               f->cbkt = 0;
         //    if (

               }

              if (tstart == fend && f->toaddr == tend)   //   && tstart > fend)
               {
               DBGPRT(1,0);
               if (t->jtype == J_COND) t->jor = 1;
               }
//*/



      
             zx++;
             t = (JMP*) chjpf.ptrs[zx];

             if (t->fromaddr > f->toaddr)  break;

            }  //end while (inside->outside)
        







//          if (f->jtype == J_COND && !f->back && !f->bswp && !f->uci && !f->uco) f->cbkt = 1;      // conditional, forwards, clean.

        }  
    }




}







LBK *find_code_cmd (int start, int fcom)
{
  LBK *blk;

  blk = (LBK*) chmem(&chcode);
  blk->start = start;
  blk->fcom =  fcom;
  blk = (LBK *) find(&chcode,blk);
  return blk;
}


LBK* add_code_cmd (uint start, uint end, uint com)
 {          // this for user commands only, no called params
  LBK *n;

  if (anlpass >= ANLPRT)    return NULL;

  //  verify start and end addresses

  if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

  // can get requests for 0xffff with bank, which will break boundary....

  if (!bankeq(end, start))
    {
      #ifdef XDBGX
     // if (nobank(start) < 0xffff) to drop warning
      chcode.lasterr = E_BKNM;
      DBGPRT(1,"Invalid - diff banks %05x - %05x", start, end);
      #endif
      return NULL;
    }

  if (end < start) end = start;

  // check start and end within bounds

  chcode.lasterr = E_INVA;       // invalid address

  if (!val_code_addr(start)) return NULL;

  if  ((com &0x1f) == C_XCODE)
   {  // special for xcode
    if (end > maxbkadd (end)) return NULL;
   }
  else
  if (!val_code_addr(end))   return NULL;

  n = (LBK *) chmem(&chcode);
  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
  if (com & C_CMD) n->cmd = 1;
  if (com & C_SYS) n->sys = 1;
  
  n = inscmd(&chcode,n);

  if (chcode.lasterr) return 0;                        // fail

  #ifdef XDBGX
  DBGPBK(1,n, "Add cmd");
  #endif

  return n;   // successful
 }

SUB * add_subr (int addr)
{
  // type is 1 add name, 0 no name
  // returns valid pointer even for duplicate

  int ans, ix;
  SUB *x;

  if (anlpass >= ANLPRT) return NULL;
  if (!val_code_addr(addr)) return NULL;

  if (find_code_cmd(addr, C_XCODE))
     {
       #ifdef XDBGX
       DBGPRT(1,"XCODE bans add_subr %x", addr);
       #endif
      return NULL;
     }


// insert into subroutine chain

  x = (SUB *) chmem(&chsubr);
  x->start = addr;

  ix = bfindix(&chsubr, x, 0);
  ans = cpsub (&chsubr,ix,x);

  if (ans)
   {
    chinsert(&chsubr, ix, x);       // do insert
    chsubr.lasterr = 0;
   }
  else
  {
   // match - do not insert. Duplicate.
   x = (SUB*) chsubr.ptrs[ix];
   chsubr.lasterr = E_DUPL;
  }

  #ifdef XDBGX
    if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Dup");
    DBGPRT(0," sub %x",  addr);
    DBGPRT(1,0);
  #endif
  return x;
  }
 
 SBK *add_scan (uint add, int type, SBK *caller)
{

 /* add code scan.  Will autocreate a subr for type "SUBR"
  * updates type etc even for duplicates to fit 'rescan' type model
  * C_CMD (+32) = by user command (can't be changed)
  * caller is only set in calls from do-sjsubr and taks lists (vect)
  */

   SBK *s;

   int ans;
   uint ix;

   if (anlpass >= ANLPRT)   return 0;

   // TEMP add a safety barrier ...
   if (chscan.num > 20000) return 0;

   if (!val_code_addr(add))
    {
     #ifdef XDBGX
     if (val_jmp_addr(add)) DBGPRT (0,"Ignore scan %x", add);
     else        DBGPRT (0,"Invalid scan %x", add);
     DBGPRT(1,0);
     #endif
     chscan.lasterr = E_INVA;
     return 0;
    }

   if (find_code_cmd (add, C_XCODE))
     {
      #ifdef XDBGX
      DBGPRT(0,"XCODE bans scan %x", add);
      DBGPRT(1,0);
      #endif
      chscan.lasterr = E_XCOD;      
      return 0;
     }

   if (PMANL)
      {
       #ifdef XDBGX
       DBGPRT(1,"no scan %x, manual mode", add);
       #endif
       return 0;
      }



   s = (SBK *) chmem(&chscan);             // new block (zeroed)
   s->start    = add;
   s->curaddr  = add;                      // important !!
   s->nextaddr = add;
   s->scanning     = 1;                    // scan processing on

   if (type & C_CMD) s->cmd = 1;           // added by user command

   s->type     =  type & 0x1f;

   if (caller)
      {
       s->substart = caller->substart;
       s->caller   = caller->caller;         // default to same level
       s->nodata   = caller->nodata;         // keep nodata flag
       s->gapchk   = caller->gapchk;         // keep gap check flag too
      }

   if (s->type == J_SUB)
    {
      if (caller) s->caller = caller;     // new level
      add_subr(s->start);
      s->substart =  s->start;            // new subr
      new_autosym(1,s->start);            // add name (autonumber subr) but not subr
    }

   if (s->caller && s->start == s->caller->start)  return 0;          // safety check

   ix = bfindix(&chscan, s, 0);
   ans = cpscan(&chscan,ix,s);                     // checks start address ONLY, not range

   if (!ans)
    {                                     // match - duplicate
 //    SBK *t;
  //   t = (SBK*) chscan.ptrs[ix];          // block which matches
    
     s = (SBK*) chscan.ptrs[ix];          // block which matches
    chscan.lasterr = E_DUPL;

/* M0M2 is a static jump - also need to know it's a chain scan.....

     
     if (s->args && s->type == J_SUB)           // try 1 - restore scan works for CARD, not for MOM2
     { 
       s->start    = add;
   s->curaddr  = add;                      // important !!
   s->nextaddr = add;
   s->scanning     = 1;                    // scan processing on
   s->stop = 0;
   if (caller) s->caller = caller;     // new level
      s->substart =  s->start;            // new subr
    #ifdef XDBGX
       DBGPRT  (0,"Reset Dup scan %05x ", s->start);
    #endif
     }   */
    }
   else
    {        // no match - do insert
     chinsert(&chscan, ix, s);
     if (ix < lowscan)  lowscan = ix;       // reset lowest scan index

     if (s->substart)                       // scan all blks within subr
       {
        ix = bfindix(&chsbcn, s, 0);           // and insert in subscan chain
        chinsert(&chsbcn, ix, s);
       }

     if (s->gapchk)
       {
        ix = bfindix(&chsgap, s, 0);           // and insert in subscan chain
        chinsert(&chsgap, ix, s);
       }
     chscan.lasterr = 0;
    }


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT  (0," scan %05x ", s->start);
    if (s->substart)  DBGPRT (0," sub %x", s->substart);
    if (s->caller) DBGPRT(0, " caller %x", s->caller->start);
    
    if (!ans)
    {      //dupl
    if (!s->stop) DBGPRT(0," Not");
    DBGPRT(0, " Scanned");
    if (s->args) DBGPRT(0," ARGS!");
    if (s->chscan) DBGPRT(0," CHSC");
    } 
    DBGPRT(1,0);
   #endif
 return s;
}



SBK *add_escan (uint add, SBK *caller)
{

 /* add emu scan.
  if addr is zero, COPY caller, else work as 'standard' scan
  */

   SBK *s;

   int ans, ix;

   if (anlpass >= ANLPRT)   return 0;

   s = (SBK *) chmem(&chemul);             // new block (zeroed)
   if (add) s->start = add;                // standard add emu
   else s->start    = caller->start;       // copy of SBK to new emu

   ix = bfindix(&chemul, s, 0);
   ans = cpscan(&chemul,ix,s);             // checks start address ONLY

   if (!ans)
    {                                     // match - duplicate
     s = (SBK*) chemul.ptrs[ix];          // block which matches
    }
   else
    {        // no match - do insert (s is valid pointer)
     chinsert(&chemul, ix, s);
    }

   if (!add)
    {
      // copy caller block
      *s = *caller;                          // copy whole block and pars
      s->scanning = 0;                       // clear scanning flag
    }
   else
    {
     // this add will ALWAYS be the a subr call in EMU, caller will always be set
      s->caller = caller;
      s->substart =  s->start;               // new subr
    }

   s->curaddr = s->start;
   s->nextaddr = s->start;
   s->emulating = 1;
   s->stop = 0;
   s->inv = 0;                             // and always set for emu rescan


 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," EMUscan %x", s->start);
    if (add) DBGPRT(0," New");  else DBGPRT(0," Copy");
    if (add) DBGPRT(0," from %x" , caller->curaddr);
    DBGPRT(1,0);
   #endif
 return s;
}

void fixbitno(uint *add, int *bitno)
{
 int i;

// if add is odd, should not allow > 7 ???
//bitno can be -ve !

 if (*bitno > 7)
    {
      i = (*bitno)/8;     // this works for doubles too....
      *add += i;          // add extra bytes
      i *= 8;             // bits to subtract
      (*bitno) -= i;
    }
 (*bitno) +=1;
}


SYM *add_sym (int rw, const char *fnam, uint add, int bitno, uint start, uint end)
 {
   // chain for symbols bitno = -1 for no bit

   SYM *s, *t;
   int ans;
   uint ix;

   if (anlpass >= ANLPRT) return 0;
   if (!fnam || !*fnam) return 0;          // must have a symname.

   // if (bitno > byte size, map into correct address (byte)

   fixbitno(&add,&bitno);  // make into byte
 
 //  chsym.lasterr = E_INVA;  
 //  if (add > PCORG && add > maxbkadd(add)) return 0; //syms to invalid addresses are OK
 
   s = (SYM *) chmem(&chsym);
   s->addb = add << 4;
   s->addb |= bitno;     // bitno + 1 in fixbitno (zero = none)

   if (rw & 0xf) s->writ = 1;                     // this is a WRITE sym
 
   if (start)
    {
      // only for cmd, already validated  
     s->rstart  = start;
     s->rend    = end;
    }

   ix = bfindix(&chsym, s, 0);
   ans = chsym.equal(&chsym, ix, s);              // zero if matches
   
   // eqsym checks for EXACT match, not for overlaps.   
   // check for overlap here
   // must check x->end does not overlap next block....
   // start MUST be > than end of previous block...

  if (ix < chbase.num && start)
    {
     t = (SYM *) chsym.ptrs[ix];         // chain block found
     if (s->addb == t->addb && s->rend > t->rstart)
      {
       chbase.lasterr = E_OVRG;          // overlap ranges
       return 0;
      }
     }

   // still allow a new symname if it already exists

   if (ans)
   {
     new_symname (s, fnam);
     if (rw & C_CMD) s->cmd = 1;                  // by user command
     if (rw & C_SYS)
        {
          s->sys = 1;                  // autogenerated flag  
          s->xnum = 1;                 //stop overwrites by autoname
        }  

     chinsert(&chsym, ix, s);
     chsym.lasterr = 0;

     #ifdef XDBGX
      DBGPRT(0,"add sym %x",add);
      if (fnam) DBGPRT(0," %c%s%c " ,'"',fnam, '"');
      if (s->rstart)      DBGPRT (0," %05x - %05x" , s->rstart, s->rend);
      if (bitno) DBGPRT (0," + BIT %d", (s->addb & 0xf)-1);
      if (s->flags) DBGPRT(0," + flags");
      if (s->cmd)  DBGPRT(0," + CMD");
      DBGPRT(1,0);
     #endif
   }   // do insert
 else
 {
   s = (SYM*) chsym.ptrs[ix];
   chsym.lasterr = E_DUPL;
 }
 
 return s;

}


SYM* get_sym(int rw, uint add, int bitno, int pc)
{
  // bit symbols only
// auto check of read if no write found

  SYM *s, *t;
  int a;

  a = nobank(add);
  if (a < PCORG)  add = a;     // fiddle for register and RAM syms

  if (!val_data_addr(add)) return 0;          // not valid address

  fixbitno(&add,&bitno);

  s = (SYM*) schmem();        // block for search
  s->addb = add << 4;         // symbol addr + bit
  if (bitno >= 0) s->addb |= bitno;
  s->rstart = pc;            // current addr for range check

 if (rw) s->writ = 1;

  t = (SYM*) find(&chsym,s);

  if (!t && s->rstart)
   {         // try default (write)
    s->rstart = 0;
    t = (SYM*) find(&chsym,s);
   }

   if (!t && rw)
    {         // try read symbols
     s->rstart = pc;
     s->writ = 0;          // drop write bit;
     t = (SYM*) find(&chsym,s);

     if (!t && s->rstart)
      {         // try default read symbol
       s->rstart = 0;
       t = (SYM*) find(&chsym,s);
      }
    }

// try no bit ????



  return t;
}




void scan_dc_olaps(void)
{
  // check for data commands inside code
  // could be an indexed offset ldx etc.
  // needs more tidyups in here, but works

  uint ix,i;
  int j, k;
  LBK *d;
  SBK *s, *p;

  if (!chscan.num) return;

  for (i = 0; i < chdata.num; i++)
   {
     d = (LBK *) chdata.ptrs[i];
     if (d->fcom > C_DFLT && d->fcom < C_ARGS)
      {
        s = (SBK*) schmem();              // dummy block for search
        s->start = d->start;              // symbol addr
        ix = bfindix(&chscan, s, 0);
        s = (SBK*) chscan.ptrs[ix];

        if (s->start <= d->start && s->nextaddr >= d->start )
          {
              #ifdef XDBGX
              LBK *x;
            DBGPRT(1,"DATA OVERLAP 1 %x %x %s-> %x %x %s (from %x, %d)", s->start, s->nextaddr, "SCAN", d->start, d->end, cmds[d->fcom],d->from, d->opcsub);
            x = (LBK *) chdata.ptrs[i];
            DBGPBK(1,x,"delete (%d)", i);
            #endif
            chdelete(&chdata,i,1);
            i--;
            ix = 0;                      //stop next section
          }

       if (ix > 0)
         {
          p = (SBK*) chscan.ptrs[ix-1];  // previous scan

          if (p->start <= d->start && p->nextaddr >= d->start )
           {
               #ifdef XDBGX
            DBGPRT(1,"DATA OVERLAP 2 %x %x %s-> %x %x %s (from %x, %d)", p->start, p->nextaddr, "SCAN", d->start, d->end, cmds[d->fcom], d->from, d->opcsub);
            #endif
            if (d->opcsub == 3)
             {  // indexed - check for standard (2-vects ,4,6,8 - cyls)
              j = (p->nextaddr+1) - d->start;
              if (s->start <= p->nextaddr && ix < (chscan.num-1))
              s = (SBK*) chscan.ptrs[ix+1];         // skip forwards if scans overlap
              k = s->start - d->start;             // just a test for now
              // may need to check END gap as well ???
              if( j >= 0 && k <= 16)
               {
                d->start += j;
                d->end += j;
                
                if (d->fcom > 1 && (d->start & 1))
                  {     // can't have odd for bigger than byte...............
                   d->start++;
                   d->end++;
                  } 
                
                   #ifdef XDBGX
                DBGPRT(1,"move data to %x-%x", d->start, d->end);
                  #endif
               }
              else
               {
                #ifdef XDBGX
                  LBK *x;
                  x = (LBK *) chdata.ptrs[i];
                  DBGPBK(1,x,"delete (%d)", i);
                #endif
                chdelete(&chdata,i,1);
                i--;
               }
             }
            else
             {
               #ifdef XDBGX
                 LBK *x;
                 x = (LBK *) chdata.ptrs[i];
                 DBGPBK(1,x,"delete (%d)", i);
               #endif
              chdelete(&chdata,i,1);
              i--;
             }
           }
        }
     }
  }
}


void scan_sgap(uint addr)
 {
  // add a scan for the detected gap, and add_scan keeps
  // scans also in extra chain for tracking

  SBK *s, *x;
  uint ix, num;

  // check not a 'fill' gap first

  if (check_sfill(addr))
  {
    #ifdef XDBGX
     DBGPRT(1,"Ignore gscan for %x, fails rpt check", addr);
    #endif
    return;
  }

  x = add_scan(addr, J_STAT,0);
  if (!x) return;

  x->gapchk = 1;
  x->topgap = 1;            // flag for comment
  scan_blk(x, &cinst);

  if (!x->inv)
  {
  // this may have added new scans, will be in gap chain

//  wnprt(1,"# scan gap at %x as code",cmdaddr(addr));
  ix = 0;

  num = chsgap.num;

  while (ix < chsgap.num)
   {
    s = (SBK*) chsgap.ptrs[ix];
    if (!s->stop && !s->inv && s->scnt < 10)
      {
        #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(0,"Sgap Scan %d ", ix);
        #endif
          scan_blk(s, &cinst);
          if (chsgap.num != num)
            {
             #ifdef XDBGX
             DBGPRT(1,"NEWCH %d %d", chsbcn.num, num);
             #endif
             ix = -1;   // rescan if changed
             num = chsgap.num;
            }
         }
    ix++;
   }

  }   // x->inv
  clearchain(&chsgap);       // clear all temp scan blocks

 }














JMP * add_jump (SBK *s, int to, int type)
{
  JMP *j;
  OPC *x;
  int ix, ans;

  if (anlpass >= ANLPRT) return NULL;

  if (!val_jmp_addr(to))
    {
     #ifdef XDBGX
     DBGPRT(1,"Invalid jump to %x from %x", to, s->curaddr);
     #endif
     return NULL;
    }

  j = (JMP *) chmem(&chjpf);
  j->toaddr     = to;
  j->fromaddr   = s->curaddr;
  j->jtype  = type;
  j->size   = s->nextaddr-s->curaddr;   // excludes compare

  // is this preceeded by a compare ?
  // get size of cmp as well

  ix = find_opcode(0, s->curaddr, &x);
  if (x && x->sigix == 10)  j->cmpcnt = s->curaddr - ix;  // count back


  if (!bankeq(j->toaddr, j->fromaddr))  j->bswp = 1;        // bank change
  else
  if (j->jtype && j->fromaddr >= j->toaddr) j->back = 1;   

  // insert into the two jump chains

  ix = bfindix(&chjpf, j, 0);      // FROM chain
  ans = chjpf.equal(&chjpf, ix, j);

  if (ans)
    {
     chinsert(&chjpf, ix, j);                    // do insert in FROM chain
     ix = bfindix(&chjpt, j, 0);                 // and in TO chain
     chinsert(&chjpt, ix, j);
    }
  else  j =  (JMP *) chjpf.ptrs[ix];             // match - duplicate

//  check_jump_span(j);

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," jump %x-%x %s", j->fromaddr, j->toaddr, jtxt[j->jtype]);
    if (j->retn)  DBGPRT(0," ret");
    DBGPRT(1,0);
   #endif

  if (!ans) return 0;             // for detection of duplicate (sjsubr)
  return j;
}

uint find_tjix(uint ofst)
{
  // find to-jump - as this is multiple,
  // bfindix (via cpjump) will find first one

  JMP * j;
  uint ix;

  if (!chjpt.num) return 1;

  j = (JMP*) chmem(&chjpf);
  j->toaddr = ofst;
  ix = bfindix(&chjpt, j, 0);

  // bfindix WILL find first to jump (code in cpjmp)

  j = (JMP*) chjpt.ptrs[ix];

  if (ix >= chjpt.num) return ix;

  if (j->toaddr != ofst) return chjpt.num;

  return ix;
}


LBK *find_prt_cmd(LBK *dflt, BANK *b, uint ofst)
  {
    // find next command for printout from either code or data chain
    // if none found use a default (fill) block
    // code always overrides data

    LBK *c, *d;
    int ixc, ixd;

    dflt->start = ofst;
    dflt->end = 0;

    ixc = bfindix(&chcode,dflt, 0);             // search for code entry first
    c = (LBK*) chcode.ptrs[ixc];

    if (!chcode.equal(&chcode,ixc,dflt) && c->fcom != C_XCODE) return c;    // match

    ixd = bfindix(&chdata,dflt, 0);             // search for data entry
    d = (LBK*) chdata.ptrs[ixd];

    //check code block in case found data block overlaps it
    if (c->start < d->end && c->start >= d->start && c->fcom == C_CODE)
        d->end = c->start-1;

    if (!chdata.equal(&chdata,ixd,dflt)) return d;    // match

    // not found either one, look for next blocks to get an end address

    dflt->end = b->maxbk;

    if (c->fcom != C_XCODE && dflt->end > c->start && ofst < c->start)  dflt->end = (c->start-1);  // c is valid code block

    if (d->start < dflt->end && ofst < d->start)  dflt->end = (d->start-1);

    dflt->fcom = C_DFLT;

    if (dflt->end < dflt->start && dflt->end < b->maxbk)
     {
       #ifdef XDBGX
        DBGPRT(0,"PANIC for %x  %x!!! ", dflt->start, dflt->end);
        DBGPBK(0,c,"CD");
        DBGPBK(1,d,"DAT");
       #endif
       dflt->end = dflt->start+1;
     }

return dflt;

}


void scan_code_gaps(void)
{
  uint ix, dx, ofst;   //, nscans;
  SBK *s, *n;
  LBK *d;

  // use scans, nextaddr is end provided scanned is set.........
  // still experimental !!

//  ditch invalid scans first.

  ix = 0;
  while (ix < chscan.num)
  {
    s = (SBK*) chscan.ptrs[ix];
    if (s->inv) chdelete(&chscan,ix,1);
    else ix++;
  }

  ix = 0;
  while (ix < chscan.num-1)
   {
    s = (SBK*) chscan.ptrs[ix];
    n = (SBK*) chscan.ptrs[ix+1]; // next scan block

    if (nobank(s->start) <= 0x201e) { ix++; continue;}

    ofst = s->nextaddr+1;      // ofst after end of THIS scan

    if (ofst < n->start)
     {       // GAP, could be code or data in here..............
      if ((ofst & 1) && g_byte(ofst) == 0xff) ofst++;   // filler

      d = (LBK*) chmem(&chdata);
      d->start = ofst;

      dx = bfindix(&chdata, d, 0);

      while (dx < chdata.num)
        {
         d = (LBK *) chdata.ptrs[dx];
         if (d->opcsub == 1) dx++;      //skip any immediates
         else break;
        }
      if (dx < chdata.num && d->start > n->start)
            {
             // code gap with nothing else in it (except perhaps for immediates).
             #ifdef XDBGX
             DBGPRT(1,"******************************************* Code GAP %x-%x", s->nextaddr+1, n->start-1); // not if found something before it.
             #endif
             scan_sgap(s->nextaddr+1);
            }

         //}
  //     else
   //    {
   //        DBGPRT(1,"z");
  //     }

   //  what if d not found ???


     }
     ix++;
   }

         #ifdef XDBGX
           DBGPRT(1,0);
           DBGPRT(1,"- - -END Code gap Scan");
        #endif

 } // end func