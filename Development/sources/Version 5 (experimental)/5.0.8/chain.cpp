

/******************************************************
 *  8065 advanced disassembler for EEC-IV
 *
 * This program is for private use only.
 * No liability of any kind accepted or offered.
 *
 * Version number and notes in shared.h
 ******************************************************/

#include  "chain.h"               // this calls shared.h

/**********************************************************************************
* CHAIN declarations, used ordered storage with for binary chop searches
*
* CHAIN structures use common find and insert mechanisms with defined subroutines for
* each chain type, Each type has its own structure associated with it.
* searched with 'binary chop' type algorithm.
* CHAIN params = num entries, num ptrs, allocsize, entrysize, maxptrs, num mallocs, ptr array,
* subroutines - free, compare, equals, (debug)
***********************************************************************************/

// Free block, for a few chains

void fsym(void *); void fsub(void *);
void fcmd(void *); void fblk(void *);
void fscan(void *);

// compares, by chain for binary chop searches

int cpjmpf (CHAIN *,uint, void *);   int cpjmpt (CHAIN *,uint, void *);
int cpbase (CHAIN *,uint, void *);   int cpmath (CHAIN *,uint, void *);
int cpcmd  (CHAIN *,uint, void *);   int cpscan (CHAIN *,uint, void *);
int cpsub  (CHAIN *,uint, void *);   int cpsym  (CHAIN *,uint, void *);
int cpsig  (CHAIN *,uint, void *);
int cpspf  (CHAIN *,uint, void *);   int cppsw  (CHAIN *,uint, void *);
int cpdtk  (CHAIN *,uint, void *);   int cpdtka (CHAIN *,uint, void *);
int cplnk  (CHAIN *,uint, void *);   int cpname (CHAIN *,uint, void *);
int cpbnk  (CHAIN *,uint, void *);   int cpadt  (CHAIN *,uint, void *);

int eqcmd (CHAIN *x, uint ix, void *newb);
int eqlnk (CHAIN *x, uint ix, void *newb);
int eqscan (CHAIN *x, uint ix, void *newb);


// print item - a few chains - for generic print of a CHAIN item

int pl_cmd(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_psw(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_sym(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_sub(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_rbt(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_adl(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_bnk(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_spf(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));
int pl_rst(CHAIN *,uint, uint, uint (*prt) (uint,cchar *, ...));

// CHAIN params = num entries, num ptrs, allocsize, entrysize,
// nextid, last search index, last error, lowest insert index, new blk,  ptr array,
// subroutines (if set) - free, compare, print (msg and debug files)

// chains for holding data


CHAIN chsym   = {&chsym,   0,0,200,sizeof(SYM),   0,0,0,0, 0, fsym  ,cpsym  ,cpsym  ,pl_sym  };     // symbols

CHAIN chbase  = {&chbase,  0,0,200,sizeof(RBT),   0,0,0,0, 0, 0     ,cpbase ,cpbase ,pl_rbt  };     // rbases
CHAIN chsig   = {&chsig,   0,0, 20,sizeof(SIG),   0,0,0,0, 0, 0     ,cpsig  ,cpsig  ,0  };          // signatures

CHAIN chcmd   = {&chcmd,   0,0,200,sizeof(LBK),   0,0,0,0, 0, fcmd  ,cpcmd  ,eqcmd  ,pl_cmd  };      // main code and data commands
CHAIN chaux   = {&chaux,   0,0,100,sizeof(LBK),   0,0,0,0, 0, fcmd  ,cpcmd  ,eqcmd  ,pl_cmd  };      // auxilliary commands (arg, xcode, ...)

CHAIN chscan  = {&chscan,  0,0,200,sizeof(SBK),   0,0,0,0, 0, fscan ,cpscan ,eqscan, 0  };           // code scans

CHAIN chstst  = {&chstst,  0,0,200,sizeof(SBK),   0,0,0,0, 0, fscan ,cpscan ,eqscan, 0  };           // test scans - gap, vect, emul, etc

CHAIN chsubr  = {&chsubr,  0,0, 30,sizeof(SUB),   0,0,0,0, 0, fsub  ,cpsub  ,cpsub, pl_sub  };       // subroutines

CHAIN chadnl  = {&chadnl,  0,0, 50,sizeof(ADT),   0,0,0,0, 0, 0     ,cpadt  ,cpadt, pl_adl  };       // additional data, linked by FK

CHAIN chspf   = {&chspf,   0,0, 10,sizeof(SPF),   0,0,0,0, 0, 0     ,cpspf  ,cpspf, pl_spf  };       // special funcs (subroutines, tablu, funclu etc.) and answer

CHAIN chpsw   = {&chpsw,   0,0, 20,sizeof(PSW),   0,0,0,0, 0, 0     ,cppsw  ,cppsw, pl_psw  };       // psw setters

CHAIN chdtk   = {&chdtk,   0,0,500,sizeof(TRK),   0,0,0,0, 0, 0     ,cpdtk  ,cpdtk, 0  };            // opcode data tracking.  index by opcode ofst

CHAIN chdtd   = {&chdtd,   0,0,500,sizeof(DTD),   0,0,0,0, 0, 0     ,cpdtka ,cpdtka ,0 };            // data address tracking.  index by data start

CHAIN chmathn = {&chmathn, 0,0, 50,sizeof(MATHN), 0,0,0,0, 0, 0     ,cpname ,cpname ,0 };            // math func names.  index by name
CHAIN chmathx = {&chmathx, 0,0, 50,sizeof(MATHX), 0,0,0,0, 0, 0     ,cpmath, cpmath   ,0 };          // math func definitions - first term links to name

CHAIN chlink  = {&chlink,  0,0,500,sizeof(FKL),   0,0,0,0, 0, 0     ,cplnk   ,eqlnk    ,0  };        // linker many->many, forwards   keyin ->  keyout

CHAIN chjpf   = {&chjpf,   0,0,500,sizeof(JMP),   0,0,0,0, 0, 0     ,cpjmpf  ,cpjmpf, 0  };          // jump, indexed by 'from'
CHAIN chjpt   = {&chjpf,   0,0,500,0          ,   0,0,0,0, 0, 0     ,cpjmpt  ,cpjmpt, 0  };          // jump, indexed by 'to' [REMOTE]

CHAIN chbkf   = {&chbkf,   0,0, 50,sizeof(BNKF),  0,0,0,0, 0, 0     ,cpbnk   ,cpbnk, pl_bnk  };      // bank finds


/* Notes
* chlink - link chain, No reverse as add_link does both fwd and back in a pair.
*
* Keys
* Search keys are unique for binary chop, exceptions are -
* Links   (chlink).  No unique single key - need all three (sce, dst, type) to be unique.
* Jump to (cpjpt).  'Jump from' is unique, but jump to is not.
* Extra code in compares and equals to cater for this, extra backwards check as necessary
*/

// CHAIN Pointer index array for neater access across all modules
//need a dummy if using optional key values.............

CHAIN *chindex[20] = { 0, &chsym,  &chbase,  &chsig,  &chcmd,   &chaux,
                       &chscan, &chstst,  &chsubr, &chadnl,  &chspf,
                       &chpsw,  &chdtk,   &chdtd,  &chmathn, &chmathx,
                       &chlink, &chjpf,   &chjpt , &chbkf
 };

#ifdef XDBGX

uint DBGmax      [20];       // max number of pointers
uint DBGallocs   [20];       // no of (re)allocs
uint DBGalloced ;            // total alloced


#endif

void *shuffle[32];                // for chain reorganises (and deletes), a block of void pointers


// for data struct discovery etc
 uint datadds    [34];
 uint datszes    [34];
 uint mtype      [34];                      // type match score
 uint valb       [34];                      // unique values for col match
 uint scores     [1024];                    // 32 cols by 32 rows, best score, also sym lists
 char tempname   [96];

 INST  tinst;                        // search instance (for conditionals, params, etc)
 SBK   tscan;                        // local scan block for searches

// ***********************************************************************************




void mfree(void *addr, size_t size)
 {
     // free malloc with tracking
  if (!addr)
  {
   #ifdef XDBGX
     DBGPRT(1,"zero pointer release!");
     DBGalloced -= size;
   #endif
   return;     // safety
  }

  free (addr);
 }


void* mem(void *ptr, size_t osize, size_t nsize)
{  // malloc with local tracking
  void *ans;
  if (nsize < 1) wnprt(1,"Invalid malloc size");

  #ifdef XDBGX
   DBGalloced -= osize;
  #endif

  ans = realloc(ptr, nsize);

  #ifdef XDBGX
  DBGalloced += nsize;
  #endif

  if (!ans) wnprt(1,"Run out of malloc space");
  return ans;
}


void* cmem(void *ptr, size_t osize, size_t nsize)
{  // malloc with clear
  void *ans;
  ans = mem(ptr,osize,nsize);
  memset(ans,0,nsize);
  return ans;
}


void adpch(uint ch)
 {   // add pointers for more chain pointers (malloced entries)
     // pointer block is realloced, so is always contiguous.
     // data blocks are not.

    size_t nw, msz;
    uint i;
    char *z, *n;
    CHAIN *x;

    x = chindex[ch];

    #ifdef XDBGX
    DBGallocs[ch] ++;
    #endif

    nw = x->pnum+x->asize;

    x->ptrs = (void**) mem(x->ptrs, x->pnum*sizeof(void*), nw*sizeof(void*));

    // allocate new data entries if not a remote chain.

    n = NULL;

    if (x->esize)
     {
      msz = (x->esize*x->asize);         // size of new data block to go with ptrs + extra pointer
      n =  (char *) mem(0,0,msz);
      if (!n) wnprt(0,"FATAL - MALLOC ERROR ");
     }

    z = n;                   //skip 'base' pointer

    for (i = x->pnum;  i < nw; i++)            // pnum + 1;  with nw already incremented ??
     {
      x->ptrs[i] = z;
      z += x->esize;
     }
   x->pnum += x->asize;

 }


CHAIN *get_chain(uint chn)
{
    if (chn >= NC(chindex) ) return NULL;
    return chindex[chn];
}


uint get_lastix(uint ch)
{
  CHAIN *x;
  if (ch >= NC(chindex) ) return 0;
  x = chindex[ch];
  return x->lastix;
}

/*
void *get_last_chitem(uint chn)
{
  CHAIN *x;
  if (chn >= NC(chindex) ) return NULL;
  x = chindex[chn];
  if (x->lastix >= x->num) return NULL;
  return x->ptrs[x->lastix];
}
*/

uint get_lasterr(uint ch)
{
  CHAIN *x;
    if (ch >= NC(chindex) ) return 0;
  x = chindex[ch];
 return x->lasterr;
}

/*
void clear_lasterr(uint ch)
{
  CHAIN *x;
  if (ch >= NC(chindex) ) return;
  x = chindex[ch];
  x->lasterr = 0;
}*/


void* chmem(uint ch, uint ix)
 {
   // get a free entry of the chain for search or insert.
   // this is from x->num onwards (next free entry) + ix for nested use
   // and from 'sce' for handling remote chains.

  void *ptr;
  uint num;
  CHAIN *x;

  x = chindex[ch];

  num = x->num + ix;                      // allow more remote block

  x = x->sce;                             // may be a different chain.

  if (!x->esize) return NULL;             // safety, should never happen.

  if (x->pnum < (num+2)) adpch(ch);    // need more pointers (unsigned check)

  ptr = x->ptrs[num];                     // use next free block (at end of list)
  memset(ptr,0, x->esize);                // and clear it
  x->newins = num;                        // remember
  return ptr;
 }



void chinsert(uint ch, uint ix, void *blk)
 {
  // insert new pointer (to blk) at index ix
  // relies on main pointer block being contiguous;

  CHAIN *x;

  if (!blk) return ;

  x = chindex[ch];

  if ((x->pnum - x->num) < 2) adpch(ch);    // need more pointers

  // move 'gap' entries up one from ix to make a space.
  memmove(x->ptrs+ix+1,x->ptrs+ix, sizeof(void*) * (x->num-ix));

  x->ptrs[ix] = blk;                              // insert new block (x->num) at ix
  if (ix < x->lowins) x->lowins = ix;             // set lowest insert index (for loops)
  x->num++;

  x->lastix = x->num;                              // invalidate any get_next
  x->lasterr = 0;                                  // and clear any errors
  x->newins = x->pnum+1;                           // new blk now invalid
  #ifdef  XDBGX
  if (x->num > DBGmax[ch]) DBGmax[ch] = x->num;     // for debug
  #endif

 }



void chdelete(uint ch, uint ix, uint num)
{
  // drop num block(s) from list from ix, shuffle down rest of
  // list and restore ptr(s) after x->num.
  // relies on main pointer block being contiguous
  // shuffle array to hold pointers (max 32)
  // extra +1 in shuffle is to preserve ch[x->num] block (used by chmem)

  uint i;
  CHAIN *x;

  x = chindex[ch];

  x = x->sce;                        // real chains, not remotes

  if (ix >= x->num) return;          // invalid index, ignore it

  if(num > NC(shuffle)) num = 32;    // safety, max 32 block delete




  if (x->chfree)
    {     //free any additional items
     for (i = ix;  i < ix+num; i++) x->chfree(x->ptrs[i]);
    }

  // save pointers about to be 'deleted' (released) in temp array
  memcpy(shuffle, x->ptrs+ix, sizeof(void*) * num);

  x->num -= num;   //drop no of defined entries

  // move 'gap' entries down by 'num+1' from ix+num to overwrite released entries, with extra chmem(0) pointer.
  memmove(x->ptrs+ix, x->ptrs+ix+num, sizeof(void*) * (x->num-ix+1));

  // put released pointers back after new x->num position so they become unused
  memcpy(x->ptrs + x->num+1, shuffle, sizeof(void*) * num);
  x->lastix = x->num;                              // invalidate any get_next

 }


void *get_next_item(uint chn, uint *ix)
{
    CHAIN *x;
    x = chindex[chn];
    if (*ix >= x->num) return 0;
    return x->ptrs[(*ix)++];
}



void clearchain(uint ix)
{
  // clear pointer list contents (virtually, by setting ptr num to zero)
  // do not clear pointers themselves. Free any attached structures
  //  NB. count DOWN so that for mblks first entry (which points to itself) is released last.

 int i;
 CHAIN *x;

 x = chindex[ix];

 //clear any dependent items (e.g strings from symbols)
if (x->chfree)
 {
  for (i = x->num-1; i >=0; i--) x->chfree(x->ptrs[i]);
 }


 x->num = 0;
}


void freeptrs(uint ix)
{
    CHAIN *x;
     x = chindex[ix];
    // free the master pointer block of chain x (after clearchain does entries)
 if (x->pnum)
   {
    mfree(x->ptrs, x->pnum * sizeof(void*));
    x->ptrs = NULL;
    x->pnum = 0;
   }
}


void freechain(uint ch)
{


  #ifdef XDBGX
  CHAIN *x;

    x = chindex[ch];
    int size;
    size =  x->pnum * sizeof(void *);     // number of void* pointers
    size += x->pnum * x->esize;           // plus structs pointed to
    DBGPRT(1,"Num=%5d  esize=%3d totsize=%6d  max=%5d allocs=%2d  %s",
            x->pnum,  x->esize, size, DBGmax[ch], DBGallocs[ch], get_chn_name(ch));
  #endif

  clearchain(ch);
  freeptrs(ch);
}

void free_structs ()
{                        // complete tidyup for closing or restart
 uint  i;
 BANK  *b;

 for (i = 0; i < BMAX; i++)
  {
   b = get_bkdata(i);

   if (b->bok)
    {
      mfree(b->fbuf, 0x10000); // free bank
    }
  }


 for (i = 1; i < NC(chindex); i++) freechain(i);

}


void *vconv(uint addr)
{
   // allows first term of any chain to link either via a uint (e.g. bin address) and by
   // void* pointer within each chain of terms. Converts uint to void*

   // convert a uint of 32 bits into void* (64 bits) and bypass compiler warnings.
   // It doesn't matter how compiler aligns this as long as it's consistent within the program.


  union
   {
    void *fid;
    uint address;
   } cv;

  cv.fid = 0;            // clear whole field
  cv.address = addr;     // 'add' address (in effect)

  return cv.fid;
}


//---------------------------------------------------------------------------------------/

// CHAIN Compare subrs used by findix for binary chop chain searches
// Straight subtract works nicely for chain ordering, even for pointers
// void* keys converted to char* to do subtract.
// answer < 0 less than, 0 equals, > 0 greater than

int cpjmpf (CHAIN *x, uint ix, void *newb)
{
  // from chain. fromaddr is unique
  int ans;
  JMP *ch, *nw;

  if (ix >= x->num) return -1;

  ch = (JMP*) x->ptrs[ix];
  nw = (JMP*) newb;


   ans = ch->fromaddr - nw->fromaddr;

  return ans;
}

int cpjmpt (CHAIN *x, uint ix, void *newb)
{
  // 'jump to' chain
  // 'fromaddr' is unique, but 'toaddr' is not.

  int ans;
  JMP *ch, *nw;

  if (ix >= x->num) return -1;

  ch = (JMP*) x->ptrs[ix];
  nw = (JMP*) newb;

  ans = ch->toaddr - nw->toaddr;
  if (ans) return ans;

  // extra check if not at first item of a multiple key
  // when 2nd key is not set.
 //  Test previous entry to see if 1st key matches

  if (!nw->fromaddr && ix)
     {
       JMP *z;
       z = (JMP*) x->ptrs[ix-1];
       if (z->toaddr == nw->toaddr) return 1;
     }

  ans = ch->fromaddr - nw->fromaddr;
  return ans;
}


int cpsym (CHAIN *x, uint ix, void *newb)
{
 // symbols - multiple items in effect make the key

 int ans;
 SYM *ch, *nw;

  if (ix >= x->num) return -1;

  ch = (SYM*) x->ptrs[ix];
  nw = (SYM*) newb;

 ans = ch->addr - nw->addr;
 if (ans) return ans;

 ans = ch->fstart - nw->fstart;    // lowest bit first
 if (ans) return ans;

 ans = nw->fend - ch->fend;        //  reverse order, largest field first - nw->fend;     //includes WRITE (reverse order ?)
 if (ans) return ans;

 return nw->rstart  -  ch->rstart;    // by range start, in REVERSE order

}


int cpbase (CHAIN *x, uint ix, void *newb)
{
 RBT *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (RBT*) x->ptrs[ix];
 nw = (RBT*) newb;

 ans = ch->reg - nw->reg;
 if (ans) return ans;

 return nw->rstart - ch->rstart;  // by start, in reverse order

}


int cpsig (CHAIN *x, uint ix, void *newb)
{
  SIG *ch, *nw;
  int ans;

  if (ix >= x->num) return -1;

  ch = (SIG*) x->ptrs[ix];
  nw = (SIG*) newb;

  ans = ch->start - nw->start;

  return ans;

}


int cpspf (CHAIN *x, uint ix, void *newb)
{
  // convert void* to char* for pointer arithmetic

 SPF  *ch, *nw;
 int ans;


 if (ix >= x->num) return -1;

 ch = (SPF*) x->ptrs[ix];
 nw = (SPF*) newb;

 ans = (char *) ch->fid - (char *) nw->fid;

 return ans;

}


int cpadt (CHAIN *x, uint ix, void *newb)
{

 ADT  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (ADT*) x->ptrs[ix];
 nw = (ADT*) newb;

 ans =  (char*) ch->fid - (char*) nw->fid;

 if (ans) return ans;

 ans = ch->sbix - nw->sbix;         //and index

 return ans;

}


int cpscan (CHAIN *x, uint ix, void *newb)
{

 SBK  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (SBK*) x->ptrs[ix];
 nw = (SBK*) newb;

 ans = ch->start - nw->start;

 return ans;

}



int cppsw (CHAIN *x, uint ix, void *newb)
{

 PSW  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (PSW*) x->ptrs[ix];
 nw = (PSW*) newb;

 ans = ch->jstart - nw->jstart;

 return ans;

}


int cplnk  (CHAIN *x, uint ix, void *newb)
{
   // key to key generic multiple linker
   // forward chain, scekey -> destkey
   // many to many, with extra type
   // no key is unique, and only unique with all 3 values set

 FKL *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (FKL*) x->ptrs[ix];
 nw = (FKL*) newb;

  ans = (char *) ch->keysce - (char *) nw->keysce;
  if (ans) return ans;

  // extra check if not at first key of a multiple when 2nd key
  // is not set. Test previous entry to see if 1st key matches

  if (!nw->keydst && ix)
     {
       FKL *z;
       z = (FKL*) x->ptrs[ix-1];
       if ((char *) z->keysce == (char *) nw->keysce) return 1;
     }

  if (nw->keydst) ans =  (char *) ch->keydst - (char *) nw->keydst;
  if (ans) return ans;


  // same trick required for type ???
  // both keys must match to get here

  if (!nw->type && ix)
     {
       FKL *z;
       z = (FKL*) x->ptrs[ix-1];
       if ((char *) z->keysce == (char *) nw->keysce &&
           (char *) z->keydst == (char *) nw->keydst) return 1;
     }

  if (nw->type) ans = ch->type - nw->type;
  return ans;

 }


int eqlnk  (CHAIN *x, uint ix, void *newb)
{
   // key to key generic multiple linker
   // scekey <-> destkey both ways.
   // many to many, so need both keys

 FKL *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (FKL*) x->ptrs[ix];
 nw = (FKL*) newb;

 // no key is unique, only the combination of the three....

  ans = (char *) ch->keysce - (char *) nw->keysce;
  if (ans) return ans;

  if (nw->keydst) ans =  (char *) ch->keydst - (char *) nw->keydst;
  return ans;

  if (nw->type) ans =  ch->type - nw->type;
  return ans;

 }


int cpbnk  (CHAIN *x, uint ix, void *newb)
{
   // temp banks from find_bank....

 BNKF *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (BNKF*) x->ptrs[ix];
 nw = (BNKF*) newb;

   ans = ch->filestart - nw->filestart;                 // file start address
   if (ans) return ans;
   ans = ch->pcstart - nw->pcstart;                     // program start

   return ans;
 }



int cpdtk  (CHAIN *x, uint ix, void *newb)
{
    //main opcode tracking items to data

 TRK *ch, *nw;

 if (ix >= x->num) return -1;

 ch = (TRK*) x->ptrs[ix];
 nw = (TRK*) newb;

 return ch->ofst - nw->ofst;

}



int cpdtka  (CHAIN *x, uint ix, void *newb)
{
// data tracking, 'start' addresses list

 DTD  *ch, *nw;

 if (ix >= x->num) return -1;

 ch = (DTD*) x->ptrs[ix];
 nw = (DTD*) newb;

 return ch->stdat - nw->stdat;
}


int cpname  (CHAIN *x, uint ix, void *newb)
{
// math terms, name. use strcmp for name ordering

 MATHN  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (MATHN*) x->ptrs[ix];
 nw = (MATHN*) newb;

 if (nw->fid)
   {
     ans = (char *) ch->fid - (char *) nw->fid;
     if (ans) return ans;
   }


if (nw->nsize)
  {      //check by name
     if (ch->nsize) ans = strcmp(ch->mathname, nw->mathname);
     else ans = 1;
  }
  return ans;
}


int cpmath  (CHAIN *x, uint ix, void *newb)
{
// math terms chained to each other or to name


 MATHX  *ch, *nw;
 int ans;

 if (ix >= x->num) return -1;

 ch = (MATHX*) x->ptrs[ix];
 nw = (MATHX*) newb;

 ans = (char *) ch->fid - (char *) nw->fid;

 if (ans) return ans;

 ans = ch->pix - nw->pix;      //parameter index

 return ans;

}



int cpsub  (CHAIN *x, uint ix, void *newb)
{
 //subroutine

 SUB *ch, *nw;

 if (ix >= x->num) return -1;

 ch = (SUB*) x->ptrs[ix];
 nw = (SUB*) newb;

 return ch->start - nw->start;
}



int cpcmd  (CHAIN *x, uint ix, void *newb)
{
 // commands - as they are a RANGE of addresses
 // compare chain END to new START
 // so that new.start > chain.end is correct insert point

 // but need separate eq routine.

  LBK *ch, *nw;


 if (ix >= x->num) return -1;

 ch = (LBK*) x->ptrs[ix];
 nw = (LBK*) newb;

 return ch->end - nw->start;

}


int eqcmd (CHAIN *x, uint ix, void *newb)
{
 // make sure candidate block falls within
 // indexed block start and end.

 LBK *ch, *nw;

 if (ix >= x->num) return -1;

 ch = (LBK*) x->ptrs[ix];
 nw = (LBK*) newb;

 if (nw->start >= ch->start && nw->start <= ch->end)
    {
     if (!nw->fcom || nw->fcom == ch->fcom) return 0;         //match
    }
 return 1;
}


int eqscan (CHAIN *x, uint ix, void *newb)
{
 // make sure candidate block falls within
 // indexed block start and end.

 SBK *ch, *nw;

 if (ix >= x->num) return -1;

 ch = (SBK*) x->ptrs[ix];
 nw = (SBK*) newb;

 if (nw->start >= ch->start && nw->start <= ch->nextaddr) return 0;

 return 1;
}



uint bfindix(CHAIN *x, void *newblk)
{
  // generic binary chop.
  // returns an index value to allow easy access to adjacent blocks.
  // use the compare subr defined for each CHAIN.
  // Candidate block is void* and cast to reqd type inside compare subroutine (x->chcmp).
  // answer (min) is always [min] < newblk, therefore newblk is always >= [min]
  // and [min+1] is > blk.   Answer is therefore INSERT point or FOUND for newblk.

  uint tst, min, max;

  min = 0;
  max = x->num;

  while (min < max)
    {
      tst = min + ((max - min) / 2);                // safe way to do midpoint of (min, max);

      if (x->chcmp(x, tst, newblk) < 0) min = tst + 1;  // tst <  blk, so new minimum is (tst+1)
      else  max = tst;                                  // tst >= blk, so new maximum is tst
    }

  x->lastix = min;                                  // keep last index (even if = x->num)
  return min;
}



void chupdate(uint ch, uint ix)
{
  // block at index 'ix' has been updated - rechain in correct place
  // but need extra check here if ix = x->num

  CHAIN *x;
  void *blk;
  uint newix;

  x = chindex[ch];
  blk = x->ptrs[ix];                                                // save blk pointer

  newix = bfindix(x,blk);                                           // find new insert point

  if (newix == ix) return;                                          // nothing to do

  if (newix > ix)
   {   // move ptrs down one, over ix. Reinsert block at newix.
      newix--;    // don't move newix entry.

     if (newix >= x->num) newix = x->num-1;                           // safety, should never happen ??
     memmove(x->ptrs+ix, x->ptrs+ix+1, sizeof(void*) * (newix-ix));    // move ptrs in gap down
     x->ptrs[newix] = blk;
   }

  if (newix < ix)
   {
      // move ptrs up one, over ix, reinsert block at newix.
      memmove(x->ptrs+newix+1, x->ptrs+newix, sizeof(void*) * (ix-newix));    // move ptrs in gap UP
      x->ptrs[newix] = blk;
   }

  if (newix < x->lowins) x->lowins = newix;             // reset lowest insert index (for loops)

  x->lastix = x->num;                                   // invalidate any get_next

}



ADT *get_adt(void *fid, uint bix)
{
    // bix is for subchains on same fid
 uint ix;
 ADT *s;
 CHAIN *x;
 x = &chadnl;

 s = (ADT*) chmem(CHADNL,1);
 s->fid = fid;
 s->sbix = bix;

 ix = bfindix(x, s);

 if (!x->cheq(x, ix,s)) return (ADT*) x->ptrs[ix];

 x->lastix = x->num;   //invalidate if not found
 return NULL;
}


void free_adt(void *fid)
 {

  ADT *a;
  uint ix;

 while ((a = get_adt(fid,0)))
  {
    //but what about subfields ??? copy from print ??
    ix = chadnl.lastix;        //of a
    chdelete(CHADNL,ix,1);         // a->cnt = 0;
    fid  = a;
  }


 }



SPF *get_spf(void *fid)
{
 int ix;
 SPF *s;

 s = (SPF*) chmem(CHSPF,0);
 s->fid = fid;

 ix = bfindix(&chspf, s);

 if (!chspf.cheq(&chspf,ix,s)) return (SPF*) chspf.ptrs[ix];  //found

 chspf.lastix = chspf.num;   // invalid lastix
 return NULL;
}




void free_spf(void *fid)
 {

  SPF *a;
  uint ix;

 return;          //dummy for now...

 while ((a = get_spf(fid)))
  {
    //but what about subfields ??? copy from print ??
    ix = chspf.lastix;        //of a
    chdelete(CHSPF,ix,1);         // a->cnt = 0;
    fid  = a;
  }


 }



int olbchk(CHAIN *x, uint inx, uint blx)
 {

   /***** overlap check for new insert command block.
    * really there are only three end cases - reject, override, merge/trim
    *
   * newb is 'candidate' (new) block , ch is existing (chain) block at inx
   * set answer as bit mask -
   * 0   no overlap
   * 1   new block spans  ch  (ch falls within newb)
   * 2   new block within ch  (ch spans newb)
   * 4   new block same as ch (addresses, not commands)
   * 8   newb end   overlaps ch   (possible reduce end)
   * 10   newb start overlaps ch   (possible inrease start)
   * 20  newb end   adjacent to ch start (for merge)
   * 40  newb start adjacent to ch end   (for merge)
   * 80  commands match
   * 100  merge allowed       (commands match and permitted)
   * 200 newb overrides ch   (newb can overwrite ch)
   * 400 ch overrides newb   (newb cannot override ch)    default case anyway ???

   ************/

   LBK *ch;
   LBK *newb;
   int ans;
   DIRS *chcm, *newcmd;

   if (inx > x->num) return 0;

   if (inx == blx) return 0;                     // safety checks

   ch = (LBK*) x->ptrs[inx];
   newb = (LBK*) x->ptrs[blx];

   chcm   = get_cmd_def(ch->fcom);
   newcmd  = get_cmd_def(newb->fcom);             //for merge and overrides

   if (!chcm || !newcmd) return 0;              // pointer safety

   ans = 0;   //clear all flags;

   // NB.  check for equal already done in inscmd

   if (newb->start == ch->start && newb->end == ch->end)  ans |= 4; // addresses match
   else
   if (newb->start <= ch->start && newb->end >= ch->end)  ans |= 1; // newb spans ch
   else
   if (newb->start >= ch->start && newb->end <= ch->end)  ans |= 2; // newb is spanned by ch

   if (!ans)
    {      // check overlap, front or back
     if (newb->start < ch->start && newb->end   >= ch->start && newb->end   <= ch->end) ans |= 8;        // newb end overlap ch
     if (newb->end   > ch->end   && newb->start >= ch->start && newb->start <= ch->end) ans |= 0x10;        // newb start overlap ch
    }

   if (ch->fcom == newb->fcom)
    {
      if (ans & 0x1f) ans |= 0x80;                                              //commands match

      if (chcm->merge)
       {
           //BWAK Merge !!!!

         if (!newb->adt && !ch->adt)                              // no merge if adnl blocks attached may become matcher subr...
           {                                                      // merge matching commands if overlap set above.
             if (ans & 0x1f) ans |= 0x100;                        // overlap set already above, merge allowed
             if (newb->end == ch->start-1)  ans |= 0x120;         // 100+20 newb end   adjacent to front of ch (merge+front)
             if (newb->start == ch->end+1)  ans |= 0x140;         // 100+40 newb start adjacent to end   of ch (merge+rear)
            // for cmd chain, having ADT will stop any merge.
            // for ARGS cmd (AUX chain), if called from same address assume merge is OK (e.g. 3654) args always has ADT attached ?
           }
       }
    }
   else
    {
     // different commands, check for overrides if any olaps found.
     if (ans)
      {        // user set
         if (ch->usrcmd   && !newb->usrcmd)  ans |= 0x400;     // ch overrides by user
         if (newb->usrcmd && !ch->usrcmd  )  ans |= 0x200;     // newb  overrides


         if (!(ans & 0x600))
          {         // user cmd not set
            if (ch->sys && !newb->sys)  ans |= 0x400;             // ch    overrides
            if (newb->sys && !ch->sys)  ans |= 0x200;             // newb  overrides
          }

         if (!(ans & 0x600))
          {      // if no system or user flags override, check commands
            if (newcmd->ovrmsk & (1 << ch->fcom))    ans |= 0x200;  // newb overwrites ch
            if (chcm->ovrmsk  & (1 << newb->fcom))  ans |= 0x400;  // ch overwrites newb
          }
       }        //end of commands differ
    }


// do the dbg print HERE for both blocks

 #ifdef XDBGX
  if (ans) DBGPRTFGS(ans,newb,ch);
 #endif

 return ans;
}


uint find_max_olap(CHAIN *x, uint inx, uint blx)
{

 // get mx overlap index for blx (->newblock),
 // start at insert point in chain (inx = bfindix)

   LBK *ch, *newb;           // ch is chain block, newb is new block
   uint ans;

   ans = inx;

  if (inx >= x->num) return inx;

  newb = (LBK*) x->ptrs[blx];

  // newb end always >= chain->start

  while (ans < x->num)
    {
     ch = (LBK*) x->ptrs[ans];
     if (newb->end < (ch->start -1)) break;       //not overlap or adj
     ans++;
    }
  if (ans >= x->num) ans = x->num - 1;
  return ans;

}

uint chk_modblk(LBK* blk)
{
   //check modded blk is still OK after bound adjust
  uint ans;
  ans = 0;
  DIRS *c;

  c = get_cmd_def(blk->fcom);
  if (blk->end < blk->start) ans = 1;

  if ((blk->start + c->dsize) > blk->end) ans = 1;

  #ifdef XDBGX
    if (ans)  DBGLBK(0,blk," Bounds FAIL ");
  #endif

  return ans;
}







int inschk(uint ch, uint inx, uint blx, uint *hs)
{

  // check if newblk (at inx) overlaps anything
  // overlap results in mtype array. no of matches in ans
  // highest score index in *hs

 // HOW TO DO triples and longs which have two independent addresses ?????

  CHAIN *x;
  uint min, max, i;
  int ans;

 // LBK *chn, *nb;                  //debug

  x = chindex[ch];

  ans = 0;

  min = inx - 1;   // always cover block before insert
  if (min >= x->num) min = 0;        //assume wrap...

  max = find_max_olap(x, inx, blx);              //x->newins);

  i = 0;           // = min;
  *hs = 0;         // highest score index

  while (i+min <= max)
    {
      mtype[i] = olbchk(x, i+min,  x->newins);            // check if chain block (i) overlaps
 //     nb  = (LBK*) x->ptrs[x->newins];
  //    chn = (LBK*) x->ptrs[i+min];
      if (mtype[i])
        {
         if (mtype[*hs] < mtype[i]) *hs = i;
         ans++;
        }
      i++;
    }


return ans;
}



int fix_overlaps (uint chn, uint inx, uint blx)
{
    /***** overlap check for new insert command block.
    * really there are only three end cases - reject, override, merge/trim
    *
   * newb is 'candidate' (new) block , ch is existing (chain) block at inx
   * set answer as bit mask -
   * 0   no overlap
   * 1   new block spans  ch  (ch falls within newb)
   * 2   new block within ch  (ch spans newb)
   * 4   new block same as ch (addresses, not commands)
   * 8   newb end   overlaps ch   (possible reduce end)
   * 10   newb start overlaps ch   (possible inrease start)
   * 20  newb end   adjacent to ch start (for merge)
   * 40  newb start adjacent to ch end   (for merge)
   * 80  commands match
   * 100  merge allowed       (commands match and permitted)
   * 200 newb overrides ch   (newb can overwrite ch)
   * 400 ch overrides newb   (newb cannot override ch)    default case anyway ???

   ************/



//chdelete safe to call if ix outside x->num.    For indexes outside pnum is invalid

  LBK *ch;
  CHAIN *x;
  LBK *newb;
  uint min, ix, chk, hs;          //starts at min
  int ans;

  uint chks;         //dummy
  chks = 0;
  // find highest 'score', which is override, if any. mtype[i]
  // could always olbchk(x, i+min,  blx); again

  // or even int inschk(uint ch, uint inx) for entire loop do over...

//number of matches in mtype[0]

  x = chindex[chn];

  if (x->num == 0) return 1;        //ok to insert if chain empty

 ans = inschk(chn, inx, blx, &hs);

 if (!ans) return 1;                    //no matches...


 while (ans)
{
  min = inx-1;    //1 block below insert point
  if (min >= x->num) min = 0;        //assume wrap...

  ix = hs+min;       // always do highest scorer first

  ch    = (LBK*) x->ptrs[ix];           // chain block
  newb  = (LBK*) x->ptrs[blx];          // new or check block

  chk = mtype[hs];




  if (chk & 0x100)
        {
         // merge allowed, may be multiple in this loop, most common case so put first
         // merge new block with chain then drop chain entry and recalc
         // cases 1,2 4,8,10,20


        #ifdef XDBGX
           DBGLBK(0,ch,"Merge");
         #endif


if (chk & 1)
{
    // new block spans chain.

 ch->start = newb->start;
 ch->end   = newb->end;

  chk = 0;
}





         if (chk & 0x28)          // 20 and 8,  end adjacent or overlap, move ch->start down
           {
             ch->start = newb->start;
           }

         if (chk & 0x50)          // 40 and 10, start adjacent or overlap, move ch->end up
           {
               ch->end   = newb->end;
           }

         if (chk & 6)           // cases 4 and 2 spanned or equal
          {                     // Ignore/delete newb
            if (ix < x->num) chdelete(chn, blx ,1);
            return 0;          //no others affected return 'no insert'
          }

// 1?
         ans = inschk(chn, inx, ix, &hs);     //do overlaps against inserted block
         chk = 0;
        }


  if (chk & 0x400)
    {                  // chain block is king.
    //cases 1,2,4,8
    /* 1   new block spans  ch             (ch falls within newb)
     * 4   newb end   overlaps ch          (may change either to fit)
     * 8   newb start overlaps ch          (may change either to fit)
     */

     if (chk & 89)
       {
           if (ix < x->num) chdelete(chn, blx ,1);
         return 0;
       }

     if (chk & 16)
       {                       // newb spanned or matched. Ignore/delete newb
         if (ix < x->num) chdelete(chn, blx ,1);
         return 0;          //no others affected return 'no insert'
       }
     #ifdef XDBGX
 DBGPRT(0,"X%X%X 2");
#endif




    }


  if (chk & 0x200)
    {             // newb is king.  it can override anything

    //cases 1,2,4,8
        if (chk & 1)
         {
            // new block spans chain block - write new block
            // what if stuff is attached ???        ADT gets lost ??
            *ch = *newb;          //copy block over the top, recalculate overlaps
            ans = inschk(chn, inx, ix, &hs);     //do overlaps against inserted block
         }
       if (chk & 2)
       {
           //new fits within a chain block
          ch->end = newb->start-1;
          ch->start = newb->end+1;
//recalc
       }
// 4
// 8
 // * 10   newb start overlaps ch   (possible inrease start)
 //  * 20  newb end   adjacent to ch start (for merge)


      if (chk & 0x10)
       {
           //new start overlaps chain block end
          ch->end = newb->start-1;
          return 1;
       }





     #ifdef XDBGX
 DBGPRT(0,"X%X%X 3");
 #endif
    }


//and anything else should never happen.............1,2,4,8 by themselves........


   if (chks & 1)
        {         // new block spans the existing chain one - split newb into two ???
          #ifdef XDBGX
          DBGLBK(0,ch,"spanned (%d)", inx);
          #endif
          if (chks & 0x80)
            {                    // new block overrides chain block(s)
              #ifdef XDBGX
              DBGLBK(0,ch,"delete %d", inx);
              #endif
              chdelete(chn, inx, 1);           //  void chupdate(uint ch, uint ix) on original ?? ****
              return 2;                   // recalc after delete
            }
          else
           {
            //split block if necessary and insert two parts ?? or just reject it ?
            #ifdef XDBGX
             DBGLBK(0,newb, "No Insert");
             #endif
            return 0;                     //reject for now
           }
        }

   if (chk & 0x86)
       {                   // newb spanned or matched, same command. Ignore/delete newb
             //consider doing merge here !!
         if (ix < x->num) chdelete(chn, blx ,1);
         return 0;          //no others affected return 'no insert'
       }


 /*  if (chk & 2)
        { // new block is contained within chain block - only do something if 0x80 set
           #ifdef XDBGX
           DBGLBK(0,ch,"spans %d", inx);
           #endif
   //        if (chks & 0x80)
     //       {                    // new block overrides chain block
       //     #ifdef XDBGX
         //     DBGLBK(0,newb, "No Insert");
           //   #endif
             // return 0;                     // reject always, nothing to do....
                          // panic here
 //            }
   //       else
     //       {
              return 0;                  // stop insert
          //  }
        }

// must recheck for odd bounds !! unless do it in add....





 //        chdelete(ch, inx, 1);                               // why delete ?? chupdate(ch, ix);         //update instead.... o
 // ************************but inserting 'newb' block fucks up check next time around !!!
//      return 2;                // recalc to check surrounding blocks

  //      } */

   if (chks & 4)
         {    //  newb overlaps, front of t (chain), and not merge
           #ifdef XDBGX
           DBGLBK(0,ch," overlap F %d", inx);
           #endif
           if (chks & 0x80)
             {                            // new block overrides chain block
               ch->start = newb->end+1;    // change chain->end to fit
               if (chk_modblk(ch)) chdelete(chn, inx, 1);  //chain block not valid ***********************
               return 1;                  // test - OK to insert
             }
           else
           if (chks & 0x100)
             {                            // chain block overrides new block
               newb->end = ch->start-1;    // change new block to fit
               if (chk_modblk(newb)) return 0;  //new block not valid
               return 1;                  // test - OK to insert
             }
              else
            {
#ifdef XDBGX
               DBGLBK(0,newb, "No Insert");
               #endif
              return 0;
            }
         }

     if (chks & 8)
       { // newb overlaps end of t (chain) and not merge

       // !! not if a byte or fill !!!! The delete hack seems to work ...
#ifdef XDBGX
              DBGLBK(0,ch," overlap R %d", inx);
              #endif
           if (chks & 0x200)
             {                            // new block overrides chain block
               ch->end = newb->start-1;     // change chain.end to fit
               if (chk_modblk(ch)) chdelete(chn, inx, 1);  //chain block not valid       ______________________
               return 1;                   //OK to insert
             }
           else
           if (chks & 0x400)
             {                            // chain block overrides new block
               newb->start = ch->end+1;    // change new block to fit
               if (chk_modblk(newb)) return 0;
               return 1;
             }
              else
            {
                #ifdef XDBGX
               DBGLBK(0,newb, "No Insert");
               #endif
              return 0;
            }
       }

//if it gets to here and chks is set, default reject


  //        #ifdef XDBGX
    // DBGLBK(1,newb,  "DFLT catch %x no insert",chks);
     //           #endif
     //     return 0;           // temp stop insert



// nevber gets here ???

if (chks > 0)             // && !(chk & 380))
{
        #ifdef XDBGX
        DBGPRT(1,"PANIC reject !");
         #endif

              if (ix < x->num) chdelete(chn, blx ,1);
         return 0;          //no others affected return 'no insert'
     //    if (ans == 1)  probably OK if commdns match ??
}
}
   return 1;
}







LBK* inscmd(uint ch)
 {
   // insert need extra checks the for address ranges (start->end)
   // done in inschk

  int ans;
  uint ix;
  LBK *cblk, *newb;
  CHAIN *x;

  x = chindex[ch];

  newb = (LBK*) x->ptrs[x->newins];                 //the last chmem

  ix = bfindix(x, newb);                 // where to insert (from cpcmd)

  cblk = (LBK*) x->ptrs[ix];

  // check for exact match
  if (ix < x->num && cblk->start == newb->start && cblk->end == newb->end && cblk->fcom == newb->fcom)
    {
      if (!cblk->sys) x->lasterr = E_DUPL; else x->lasterr = 0;

       return 0;       //no insert
    }


 // inschk(ch,ix, x->newins);            // check overlaps etc - is insert allowed ?

  ans = fix_overlaps(ch, ix, x->newins);

  if (!ans && ix < x->num)
   {
      newb = (LBK*) x->ptrs[ix];       // overlap (duplicate is above)
  //    x->lasterr =  E_OVLP;           // what about overlaps ?? done in inschk
   }
  else
   {
    ix = bfindix(x, newb);           //  find again in case stuff deleted or changed
    chinsert(ch, ix, newb);          //  do insert at ix
  //  x->lasterr = 0;                  // clear any errror  should be done already
   }

x->lasterr = 0;         //stops user command errors - may not be right....
 return newb;

}

// match for signature blocks

SIG* add_sig(SIG *blk)
 {

  int ans;
  uint ix;
  SIG *s;

  ix = bfindix(&chsig, blk);

  ans = -1;

  if (ix < chsig.num)
   {       // new sig overlaps ix (which is next block in chain)
    s = (SIG*) chsig.ptrs[ix];

    if (s->start == blk->start) ans = 0;       // duplicate
    if (ix > 0)
     {
      s = (SIG*) chsig.ptrs[ix-1];             // previous entry
      if (s->novlp && s->start <= blk->start && s->end >= blk->start) ans = 0;  // no overlaps set
     }

    if (!ans)
     {
      chsig.lasterr = E_DUPL;
      return 0;
     }
   }

  chinsert(CHSIG, ix, blk);      // do insert
  return blk;

}

LBK *get_cmd (uint chn, uint start, uint fcom)
{
  // returns match if ofst within range of a data block (start-end)

  LBK *blk;
  int ix;
  CHAIN *x;

  x = chindex[chn];

  blk = (LBK*) chmem(chn,0);
  blk->start = start;
  blk->end  = start;
  blk->fcom =  fcom;

  ix = bfindix(x, blk);

  if (x->cheq(x,ix,blk)) return NULL;    // no match

  return (LBK *) x->ptrs[ix];
}


LBK *chk_aux_olap (uint start, uint end)
{
  // returns match if start within range of an XCODE
  LBK *blk, *k;
  uint ix, min, max,chkf;

  blk = (LBK*) chmem(CHAUX,0);
  blk->start = start;
  blk->end   = end;

  ix = bfindix(&chaux,blk);

  min = ix-1;                   //find_maxolap(&chaux, 0, ix, chaux.newins);
  max = find_max_olap(&chaux, ix, chaux.newins);

  chaux.lastix = chaux.num;

  while (min <= max)
    {                                                 // scan range of possible overlaps
     k = (LBK *) chaux.ptrs[min];
     chkf = olbchk(&chaux, min, chaux.newins);      // check chained block for overlap

     if (chkf && k->fcom == C_XCODE)
      {
       chaux.lastix = min;
       break;
      }
     min++;
    }

  if (chaux.lastix < chaux.num) return k;
  return NULL;
 }



LBK* add_aux_cmd (uint start, uint end, uint com, uint from)
 {
  LBK *n;

  if (get_anlpass() >= ANLPRT)    return NULL;

  //  verify start and end addresses

  if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

  // can get requests for 0xffff with bank, which will break boundary....

  if (!bankeq(end, start))
    {
      #ifdef XDBGX
     // if (nobank(start) < 0xffff) to drop warning
      chaux.lasterr = E_BKNM;
      DBGPRT(1,"Invalid - diff banks %05x - %05x", start, end);
      #endif
      return NULL;
    }

  if (end < start) end = start;

  // check start and end within bounds

  chaux.lasterr = E_INVA;       // invalid address

  if (!val_rom_addr(start)) return NULL;

   chaux.lasterr = E_INVA;

  if (!val_rom_addr(end))   return NULL;

  n = (LBK *) chmem(CHAUX,0);
  n->start = start;
  n->end   = end;
  n->fcom  = com & 0x1f;    // max 31 as set
 // n->from = from;            // where added, for args data rather than xcodes
  if (com & C_USR) n->usrcmd = 1;
  if (com & C_SYS) n->sys = 1;


  n = inscmd(CHAUX);        //,n);            // is this reqd with only aux cmds ? (xcode and args)

  if (chaux.lasterr) return 0;                        // fail

  #ifdef XDBGX
  DBGLBK(1,n, "Add aux");
  #endif

  return n;   // successful
 }



LBK* add_cmd(uint start, uint end, uint com, uint from)
 {

   LBK *b, *x;
   uint tcom;
   DIRS *cmd;

   chcmd.lasterr = E_INVA;           // set "invalid start address"
   tcom = com & 0x1f;                 // drop any extra flags

   cmd = get_cmd_def(tcom);

   if (end < start) end = start;      // safety

 //only code at 0x2000 in each bank
   if (nobank(start) == 0x2000 && tcom != C_CODE) return NULL;

   if (start < minromadd(start) || start > maxromadd (start)) return NULL;

   if (end > maxromadd (end))
     {
      chcmd.lasterr = E_INVA;        // invalid end address
      return NULL;
     }

    // can get requests for 0xffff with bank, which will break boundary....

   if (!g_bank(end)) end |= g_bank(start);     // use bank from start addr

   if (!bankeq(end, start))
     {
      chcmd.lasterr = E_BKNM;         // banks don't match
      return NULL;
     }

   if (cmd->dsize > 1 && (start & 1))       ///  longs must be on long (4)??? move to command ???
    {
      // must start on even address
      chcmd.lasterr = E_ODDB;
      #ifdef XDBGX
         DBGPRT(0,"FAIL add cmd");
         DBGPRT(0, " %x ODD Boundary from %x", start, from);
   DBGPRT(1,0);
   #endif

      return NULL;
    }

//need an overlap checker BETWEEN CHAINS - cmd and aux


   b = (LBK *) chmem(CHCMD,0);
   b->start = start;
   b->end = end;
   b->fcom  = tcom;             // max 31 as set
 //  b->from = from;
   if (com & C_USR) b->usrcmd = 1;
   if (com & C_SYS) b->sys = 1;
   if (get_cmdopt(OPTARGC))       b->cptl = 1;

//this could cause odd addresses for words............

 if (tcom == C_CODE)
     {
       if (chk_aux_olap(b->start, b->end))
         {
          #ifdef XDBGX
            DBGPRT(1,"XCODE bans code %x-%x", start, end);
            #endif
          chcmd.lasterr = E_XCOD;
          return NULL;
         }
     }

   x = inscmd(CHCMD);

   #ifdef XDBGX
   if (!chcmd.lasterr) DBGLBK (0,b,"add cmd");
   else   DBGLBK (0,b,"FAIL add cmd (%d)", chcmd.lasterr);
   if (from) DBGPRT(0, "from %x", from);
   DBGPRT(1,0);
   #endif

   return x;
}



PSW *add_psw(uint jstart, uint pswaddr)
{
  PSW *x;
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;
  if (!val_rom_addr(jstart))  return 0;
  if (!val_rom_addr(pswaddr)) return 0;

  x = (PSW *) chmem(CHPSW,0);
  x->jstart = jstart;
  x->pswop  = pswaddr;

  ix = bfindix(&chpsw, x);
  ans = chpsw.cheq(&chpsw, ix, x);                   //&chpsw,ix, x);

  if (!ans && ix < chpsw.num)
   {
    chpsw.lasterr = E_DUPL;       //duplicate

     #ifdef XDBGX
      DBGPRT(0,"Fail");
      DBGPRT(1," pswset %x = %x", jstart, pswaddr);
     #endif

    return 0;
   }

   chinsert(CHPSW, ix, x);

   #ifdef XDBGX
      DBGPRT(0,"Add");
      DBGPRT(1," pswset %x = %x", jstart, pswaddr);
     #endif

 return x;


}


PSW *get_psw(uint addr)
{
  PSW *r;
  uint ix;

  r = (PSW*) chmem(CHPSW, 1);
  r->jstart = addr;

  ix = bfindix(&chpsw, r);
  if (chpsw.cheq(&chpsw, ix,r)) return NULL;    // &chpsw,ix,r)) return NULL;  no match
  return (PSW*) chpsw.ptrs[ix];
}


RBT *add_rbase(uint reg, uint addr, uint rstart, uint rend)
{
  RBT *x, *z;
  uint ix;
  int ans;

  chbase.lasterr = E_INVA;                  // set "invalid addr"

  if (reg  & 1)              return 0;          // can't have odd registers
  if (valid_reg(reg) != 1) return 0;          // not any special regs
  if (addr > maxromadd(addr)) return 0;       // anywhere in address range

  x = (RBT *) chmem(CHBASE,0);
  x->reg    = reg;
  x->val    = addr;                           //  assumed to include bank.
  x->rstart = rstart;
  x->rend   = rend;

  ix = bfindix(&chbase, x);
  ans = chbase.cheq(&chbase, ix, x);

  // check for overlap here

  z = (RBT *) chbase.ptrs[ix];         // chain block found, nearest below

  // check for range overlaps. Range can be within an existing range,
  // or outside as a new one.  Default range is 0 - 0xfffff.

  if (ix < chbase.num)
    {
     int err;
     err = 0;

     if (x->rstart < z->rstart && x->rend   >= z->rstart && x->rend   <= z->rend) err = 1;   //overlaps rstart
     if (x->rend   > z->rend   && x->rstart >= z->rstart && x->rstart <= z->rend) err = 1;   // overlap rear

     if (err)
      {
       chbase.lasterr = E_OVRG;          // overlap ranges

       #ifdef XDBGX
        if (ans) DBGPRT(0,"Add"); else DBGPRT(0,"Fail OLAP");
        DBGPRT(0," rbase R%x = %05x", reg, addr);
        if (rstart) DBGPRT(1,"(%05x - %05x)", rstart, rend);
       #endif
       return 0;
      }
    }

     if (!ans)
      {
          if (z->inv) return 0;           //already marked as invalid.
        chbase.lasterr = E_DUPL;       //duplicate

        #ifdef XDBGX
          DBGPRT(0,"DUPL");
          DBGPRT(0," rbase R%x = %05x", reg, addr);
          if (rstart) DBGPRT(0,"(%05x - %05x)", rstart, rend);
          DBGPRT(1,0);
        #endif

        return 0;
      }


    chinsert(CHBASE, ix, x);
  //  add_rgstat(reg);

    #ifdef XDBGX
      DBGPRT(0,"Add");
      DBGPRT(0," rbase R%x = %05x", reg, addr);
      if (rstart) DBGPRT(0,"(%05x - %05x)", rstart, rend);
      DBGPRT(1,0);
    #endif

 return x;

}


RBT* get_rbase(uint reg, uint pc)
{
  RBT *r, *x;
  uint ix;
  RST *g;

  if (valid_reg(reg) != 1) return NULL;

  g = get_rgstat(reg);

  if (g->invrbt) return NULL;

  r = (RBT*) chmem(CHBASE,1);
  r->reg    = reg;
  r->rstart = pc;
  r->rend   = pc;

  ix = bfindix(&chbase, r);

  if (ix <= chbase.num)
    {
     x = (RBT*) chbase.ptrs[ix];
     if (x->inv) return NULL;
     if (x->reg == r->reg && r->rstart >= x->rstart && r->rstart <= x->rend) return x;          //   match
    }

return NULL;

}



SPF *find_spf(void *fid, uint spf, uint level)
{

 SPF *s;
 uint m ;

  while ((s = get_spf(fid)))
    {
     m = 0;
     if (!spf || s->spf == spf) m++; // i.e. zero is 'any'
     if (!level || s->level == level) m++;
   //  ix = chspf->lastix;           //keep last valid ix
     if (m >= 2) return s;
     fid = s;
    }

 return NULL;
}


SPF* add_spf (void *fid, uint spf, uint lev)
  {
   // add special func block
   // subrs can have multiple special functions.
   // adt may be attached to an spf for remote arg getters

  SPF *a, *s;
  uint ix;
  int ans;

  if (!spf) return NULL;            //safety
  if (!lev) return NULL;            //safety


//look for last item in chain, and at same time check for duplicates

  s = get_spf(fid);

  while (s)
  {
    if (s->spf == spf && s->level == lev)
      {
       chspf.lasterr = E_DUPL;
            #ifdef XDBGX
       DBGPRT(0, " DUPL SPF ");
       #endif
       return s;
       }

    a = get_spf(s);         // get next item
    if (!a) break;          // a is end of chain
    s = a;                  // last valid item
  }

  a = (SPF *) chmem(CHSPF,0);

//last valid id of chain

  if (!s) a->fid = fid;
  else
    {
        a->fid = s;
    }

  a->spf = spf;
  a->level = lev;

  ix = bfindix(&chspf,a);
  ans = chspf.cheq(&chspf,ix, a);

  if (!ans)
   {
            #ifdef XDBGX
     DBGPRT(0, "PANIC!!");
     #endif
   }
  else
  {
    chinsert(CHSPF, ix, a);
  }

  return a;
}



ADT* add_adt (void *fid, uint fix)
  {
   // add additional data block to fid as key
   // need extra lev for subterms ??

  ADT *a;
  uint ix;
  int ans;
  CHAIN *x;

  x = &chadnl;

  a = (ADT *) chmem(CHADNL,0);

  a->cnt    = 1;
  a->fend   = 7;               // single byte default

  a->fid = fid;
  a->sbix = fix;              // fid and index

  ix = bfindix(x,a);

  ans = x->cheq(x, ix, a);

  if (!ans)

  { // exists already.  Copy new over original
   ADT *s;
   s = a;
   a = (ADT*) x->ptrs[ix];
   *a = *s;
  }
else
  {
    chinsert(CHADNL, ix, a);
   }

  x->lastix = x->num;        //must reset any queries
  return a;
}



SBK *get_scan(uint ch, uint addr)
{
 uint ix;
 SBK *s;
 CHAIN *x;

 //scan must match start address

 x = get_chain(ch);

 s = (SBK*) chmem(ch,1);
 s->start = addr;

 ix = bfindix(x, s);

 if (x->cheq(x, ix ,s)) return NULL;         //&chscan,ix,s)) return NULL;

 return (SBK*) x->ptrs[ix];

}



uint get_scan_range (uint ch, uint addr)
{
 uint ix;
 SBK *s, *z;
 CHAIN *x;

 //scan can span start address

 x = get_chain(ch);

 s = (SBK*) chmem(ch,1);
 s->start = addr;

 ix = bfindix(x, s);

 if (x->lastix >= x->num) return 0;

 // minimum overlap ix

 while (ix < x->num)
   {
    z = (SBK*) x->ptrs[ix];
    if (z->nextaddr < addr) break;
    ix--;
    x->lastix--;
   }

  ix++;
  x->lastix++;


 // maximum overlap ix
 while (ix < x->num)
   {
    z = (SBK*) x->ptrs[ix];
    if (z->start > addr) break;
    ix++;
 }

 return ix;


}




SUB *get_subr(uint addr)
{
  SUB *s;
  uint ix;

  if (!addr) return 0;         // shortcut..
  s = (SUB*) chmem(CHSUBR,1);
  s->start = addr;

 ix = bfindix(&chsubr, s);
 if (chsubr.cheq(&chsubr, ix ,s)) return NULL;    // no match       &chsubr,ix,s)) return NULL;
 return (SUB*) chsubr.ptrs[ix];
}



DTD *get_dtd(uint start)
{
 uint ix;
 DTD *s;

 s = (DTD*) chmem(CHDTD,1);
 s->stdat = start;

 ix = bfindix(&chdtd, s);

if (ix < chdtd.num)
 {
  s = (DTD*) chdtd.ptrs[ix];
  if (start == s->stdat) return s;
 }

return NULL;

}



FKL *add_slink(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)
{
   // add a single link between chain elements.
   // for a 'many to many' type links
   // need source chain and item, dest chain and item.
   // may need a 'type' too ??

  CHAIN *x;
  FKL *a;
  uint ix;
  int ans;

  if (!sce)   return NULL;
  if (!dst)   return NULL;
  if (!chsce) return NULL;
  if (!chdst) return NULL;

  x = chindex[CHLINK];

  a = (FKL*) chmem(CHLINK,0);
  a->keysce = sce;
  a->keydst = dst;
  a->chsce  = chsce;
  a->chdst  = chdst;
  a->type   = type;

  ix = bfindix(x,a);

  ans = x->cheq(x, ix,a);

  if (!ans)

    {   // exists already.
     return NULL;
    }

  chinsert(CHLINK,ix,a);

  return a;
  }


FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)
{
    // add two way link with a type.
    // could do 'back' chain like jumps, but by the time a CHAIN is used
    // savings are only 1 pointer per entry ...

  FKL *a;

  a = add_slink(chsce,sce,chdst,dst,type);

  if (!a) return NULL;

  add_slink(chdst,dst,chsce,sce,type);                  // add reverse link

  return a;
  }



FKL *get_link (void *sce, void *dst, uint type)
{
 FKL *a;
 uint ix;
 CHAIN *x;

  // set up loop for generic linker, and find first item
  //don't need chain for sce, but do need for dest to get right cast

  x = chindex[CHLINK];

  a = (FKL*) chmem(CHLINK,1);              //&chlkdop,0);
  if (!a) return  NULL;

  a->keysce = sce;
  a->keydst = dst;
  a->type   = type;

  ix = bfindix(x,a);

  if (x->cheq(&chlink, ix ,a)) return NULL;    // no match

  return (FKL*) x->ptrs[ix];

}



BNKF *add_bnkf(uint add, uint pc)
{
  CHAIN *x;
  BNKF *b;
  uint ix;
  int ans;

  x = chindex[CHBNK];

  b = (BNKF *) chmem(CHBNK,0);           // both ways now covered (&chlkdop,0);
  b->filestart = add;
  b->pcstart   = pc;

  ix = bfindix(x,b);         //&chlkdop,a);

  ans = x->cheq(x, ix,b);         //chlkdop.comp(&chlkdop,ix, a);

  if (!ans)
    { // exists already.
     return NULL;
    }
  else
   {
    chinsert(CHBNK,ix,b);
   }

return b;
  }



DTD* add_dtd (INST *c, int start)
  {
   // add data block

  DTD *a;
  uint ix;
  int ans;
   OPS *o;

  o = c->opr+1;

  a = (DTD *) chmem(CHDTD,0);

  // start is only key
  a->stdat = start;

  ix = bfindix(&chdtd,a);

  ans = chdtd.cheq(&chdtd, ix, a);

  if (!ans)
   { // exists already.  update with new opcode details
     a = (DTD*) chdtd.ptrs[ix];
     chdtd.lasterr = E_DUPL;
   }
  else
   {
    chinsert(CHDTD, ix, a);
   }


// update blk AFTER block found/inserted (for multiple opcodes)


 if (c->sigix == 25)
    {            //incb or incw
      a->inc = bytes(o->fend);
    }

  if (o->inc) a->inc = bytes(o->fend);        //inc size


  if (o->rbs) a->rbs = 1;

  a->bsze  |= bytes(o->fend);

  a->modes |= (1 << c->opcsub);

  if (c->sigix == 14) a->psh = 1;
  if (c->sigix == 15) a->pop = 1;

  if (get_cmd(CHCMD,start,C_CODE)) a->olp = 1;

  return a;
  }


TRK* add_dtk(SBK *s,INST *c)
 {
   //add opcode tracking, with data links

  TRK *x;        //opcode
  DTD *d;        // data
  int ans;
  uint ix, start;
  OPS *o;

  // c instance should have all info required, BUT NOT FOR ARGS

  if (get_anlpass() >= ANLPRT)  return 0;
  if (!s->proctype) return 0;

  // would need to sort out qualifying ops (LDX, AD2B, imd etc) here
  // normally, source would be [1] which is only mutliple mode, except for stx

  // source is now ALWAYS [1]

  //  ([2] = [1]), but not always........

  // base block setup

  x = (TRK *) chmem(CHDTK,0);      // next free block
  x->ofst = c->ofst;
  x->ocnt = s->nextaddr-c->ofst;

  x->opcix = c->opcix;

  o = c->opr+1;

  for (ix = 0; ix < 5; ix++) x->rgvl[ix] = c->opr[ix].addr;

  x->opcsub = c->opcsub;
  x->numops = c->numops;

//  ans = 0;
  start = 0;

  switch (c->opcsub)
    {

      case 0:               // all register op

         if (c->sigix == 25)
           {            //incb or incw  temp removed
        //    start = databank(o->val,c);
     //       x->ainc = 1;
        //    ans = 1;

           }

         if (c->sigix == 14)
           {
            // pushw        //not always code bank ?
            start = codebank(o->val,c);
      //      ans = 1;
           }

        break;

      case 1:    // immediate op

        //need to keep immediate ldx to register as an rbase..............

         if (c->sigix == 14)
           {
            // pushw
            start = codebank(o->addr,c);

           }

         if (c->sigix == 12)       // || c->sigix == 6)
           {   // ldx
            start = o->addr;

           }

         if (c->sigix == 6)
           {
            // add - check if answer is valid address.
        // need subtract too !!!!

            start = o->addr;

           }


         if (c->sigix == 10)
           {    //cmp
            start = o->addr;

           }


//stw stb only way to store value outside of regs
// sigix = 13, opcix 48,49

        break;

      case 2:        //indirect - beware of stx ???

        x->rgvl[1] = c->opr[4].addr;
        start = o->addr;

        x->base   = o->addr;

        break;

     case 3:
        // if R0 or rbase treat as indirect

        x->rgvl[1] = c->opr[4].addr;
        x->rgvl[4] = c->opr[4].val;

        start = o->addr;  // combined address

        start = databank(start,c);

    x->offset = c->opr[0].addr;
    x->base   = c->opr[0].addr;

      if (x->offset < 0xff && c->opr[4].val > 0x2000)
          x->base = c->opr[4].val |= g_bank(o->addr);

      if (!valid_reg(x->offset) && c->opr[4].val < 0xff)
          x->offset = c->opr[4].val;

         break;

    }




if (valid_reg(start)) return 0;             //ignore if valid register

//if (!c->opcsub && !ans) return 0;

//if it gets to here, address (start) is valid

  ix = bfindix(&chdtk, x);
  ans = chdtk.cheq(&chdtk, ix, x);

  if (!ans)
    {
      x = (TRK*) chdtk.ptrs[ix];
    }
  else
    {          // no match, insert
     chinsert(CHDTK, ix, x);
    }


  // link data item even if duplicate
  d = add_dtd(c,start);          // start address

 if (!ans) chdtk.lasterr = E_DUPL;

 add_link(CHDTK, x,  CHDTD, d, 1);                // opcode <-> data

 return x;
}


void fsym(void *x)
{            // free symbol

 SYM *s;

 s = (SYM*) x;
 mfree (s->name, s->symsize);              // free symbol name
 s->name = NULL;
 s->symsize = 0;
}

void fsub(void *x)
 {
  free_adt (x);
  free_spf (x);
 }

void fcmd(void *x)
{       // free cmd
  free_adt (x);
}


void fscan(void *x)
 {
  free_adt (x);
  free_spf (x);
 }




int cellsize(ADT *a)
{
  // byte size of single ADT entry
 if (!a) return 0;
 return a->cnt * (bytes(a->fend));
}

int adtchnsize(void *fid, uint nl)
{
 // continue from fid to next newline
 // save lastix in case it hits end

 ADT *a;
 int sz;
 sz = 0;

 while ((a = get_adt(fid,0)))
   {
    sz +=  cellsize(a);                             // size in bytes
    if (nl && a->newl) break;
    fid = a;
   }

 return sz;
}


int adn_listsize(void *fid)
{
 // list size to next newline (last par).
 // may continue (via fid) from last newline
 // use chain.lastix in case it hits end

 return adtchnsize(fid, 1);

}

int adn_totsize(void *fid)
{
 // total size of aditional data chain
 // ignore newlines
 return adtchnsize(fid, 0);
}





void set_retn(JMP *x)
 {
 // set retn flag for anything which jumps to addr
 // bfindix on to chain will always get FIRST jump
 // called recursively

  uint ix;
  JMP *t;

  if (x->jtype != J_STAT) return;

  t = (JMP*) chmem(CHJPF,1);     //use forward chain
  t->toaddr = x->fromaddr;
  ix = bfindix(&chjpt, t);     // TO chain, finds first match

  while (ix < chjpt.num)
   {
    t = (JMP *) chjpt.ptrs[ix];     // match
    if (t->toaddr != x->fromaddr) break;
    // only if 't' (from) is static
    if (t->jtype == J_STAT)
       {
        t->retn = 1;
        set_retn(t);      // check jumps to here
       }
    ix++;
   }
  }



void do_jumps (void)
{

/******* jumps checklist ***************
 set conditions flags for various things
 whether jumps overlap, multiple end points etc etc.
*****************************************/


  uint ix, zx;
 // uint initofst;             // where initialise is.
//  uint tstart, tend, fstart, fend;    // have to use indexes here as interleaved scan

//  uint excnt;
  uint tend;
  JMP *f, *t;

  // first, check for jumps to return opcodes
  // true ret has jtype 0

  for (ix = 0; ix < chjpf.num; ix++)
    {         // down list of forward jumps
     f = (JMP*) chjpf.ptrs[ix];

     //assume brackets OK for all conds except backwards jumps

   if (f->jtype == J_COND)
      {

       if (!f->back)     //if back, probably a loop
       {
        f->obkt = 1;         //assume bkts OK at first
        f->cbkt = 1;
  //      f->swl =  1;         // swop logic if brackets
       }
       // else//
      }


     if (f->jtype)
      {     // static and cond jumps
       tend = g_byte(f->toaddr);
       if (tend == 0xf0 || tend == 0xf1)
         {
          f->retn = 1;
          set_retn(f);    // recursive for STAT jumps only
         }
      }
    }          //end of first loop




// check for overlaps in jumps

  for (ix = 0; ix < chjpf.num; ix++)          //unsigned !!
    {
     f = (JMP*) chjpf.ptrs[ix];                // down full list of forward jumps


     if (f->jtype == J_COND && !f->back) // && !f->done)
        { // only for conditional jumps (as base)
          // try to sort out heirarchy for whether brackets can be used
          // fromaddr is unique, but toaddr is not
          // in order of FROM...

//findix instead ??



           zx = ix+1;                                  // next jump

           while (zx < chjpf.num)
             {      // inside -> outside (same chain) i.e t->from < f->to

              t = (JMP*) chjpf.ptrs[zx];
              if (t->fromaddr >= f->toaddr)  break;

              if (t->toaddr > f->toaddr) {t->obkt = 0; }   //t->swl = 0;}
              if (t->toaddr < f->fromaddr) {t->obkt = 0; } //t->swl = 0;}
              zx++;
           }

        }
    }
}

SUB *add_subr (uint addr)
{
  // type is 1 add name, 0 no name
  // returns valid pointer even for duplicate

  int ans;
  uint ix;
  SUB *x;

  if (get_anlpass() >= ANLPRT) return NULL;
  if (!val_rom_addr(addr)) return NULL;

  if (get_cmd(CHAUX,addr, C_XCODE))
     {
       #ifdef XDBGX
       DBGPRT(1,"XCODE bans add_subr %x", addr);
       #endif
      return NULL;
     }


// insert into subroutine chain

  x = (SUB *) chmem(CHSUBR,0);
  x->start = addr;

  ix = bfindix(&chsubr, x);
  ans = chsubr.cheq (&chsubr, ix,x);

  if (!ans)
   {
    // match - do not insert. Duplicate.
    x = (SUB*) chsubr.ptrs[ix];
    chsubr.lasterr = E_DUPL;
         #ifdef XDBGX
    DBGPRT(0,"Dup");
    #endif
   }
  else
   {
    chinsert(CHSUBR, ix, x);       // do insert
    chsubr.lasterr = 0;
 //   if (get_cmdopt(OPTARGC)) x->cptl = 1;         // set compact layout
      #ifdef XDBGX
    DBGPRT(0,"Add");
    #endif
   }

  #ifdef XDBGX
    DBGPRT(1," sub %x",  addr);
  #endif

  return x;
  }


STACK *get_stack(SBK *s, uint ix)
{
  STACK *k;

  ix += s->stkptr;
  ix &= 7;          // safety
  k = s->stack+ix;

  return k;
}



void put_stack(SBK *s, uint type, uint start, uint next)
{
  STACK *k;

  // type = 1   standard CALL
  // type = 2   immediate address push
  // type = 3   pushp  (PSW)  (start = psw)
  // type = 4   dummy local ???

 // ptr &= 7;  //safety

  k = s->stack+s->stkptr;

  k->type = type;
  k->curaddr = s->curaddr;
  k->start   = start;
  k->nextaddr = next;
  k->popreg = 0;

}



void fake_push(SBK *s, uint type, SBK *newblk)
 {
  // officially stkptr -=2; [stkptr] = [Rx];
  // this autowraps pointer
  // NOT for pushp

   STACK *k;

   if (!newblk)
     {         //restore of popped entry, or push register
   //   s->stkptr --;   // still need to push stack down ??
      return;
     }

// check_dupl_push( ); ??
   k = s->stack+s->stkptr;           // set previous entry for check (may need 2 ?? PSHP+std)

   if (k->curaddr == s->curaddr)       // && k->sc == newblk)
    {
             #ifdef XDBGX
     DBGPRT(1,"%x duplicate push %x - ignored", s->curaddr, newblk->start);
     #endif
     return;
    }
   // *** check dupl...



    s->stkptr--;           // next entry
    put_stack(s,type, newblk->start, newblk->nextaddr);

  s->stkptr--;
    put_stack(s,4, s->start, s->nextaddr);              //test
  s->stkptr++;

   // put s at stkptr -1 again ??

 }




void fake_pushp(SBK *s)

 {
    // push flags (i.e. psw)
    // push is stkptr -=2; [stkptr] = PSW;

    // hardware manual - if first intruction (ie. curaddr == start) then get CALLERS bank.
    // type = 3   pushp  (PSW)  (start = psw)

    STACK *k;
    uint pp;

    s->stkptr++;
    k = s->stack+s->stkptr;       //previous entry for caller
    s->stkptr--;

    if (s->curaddr == k->curaddr)  //wrong k ??
      {
          #ifdef XDBGX
       DBGPRT(1,"%x duplicate pushp - ignored", s->curaddr);
       #endif
       return;
      }



//push HW correct format

 if (s->curaddr == s->start && k->type)     // < 3)  ??
   {
     pp = (k->curaddr >> 6) & 0x3c00;        //  get caller's bank
   }
 else
     pp = (s->codbnk << 10);    // -> 0x3c00

 pp |= s->psw;                // byte
 pp |= (s->rambnk << 8);     // -> 0x300

 s->stkptr--;
 put_stack(s, 3, pp, 0);

         #ifdef XDBGX
      DBGPRT(1,"%x PUSHP %x = %x,%x,%x",  s->curaddr, pp, s->psw, pp >> 10, (pp >> 8) & 3);

      #endif
 s->stkptr--;
    put_stack(s,4, s->start, s->nextaddr);              //test
  s->stkptr++;
 }



void set_scan_banks(SBK *s, SBK* caller)
 {
   // set default banks, carry over from caller if there is one

  s->codbnk = (s->start >> 16) & 0xf;          // code bank from start addr

  if (caller)
    {
     s->rambnk = caller->rambnk;
     s->datbnk = caller->datbnk;
    }
  else
    {
      s->rambnk = 0;                               // zero for Register banks
      if (get_numbanks()) s->datbnk = 2;           // data bank 1 for multibanks
      else   s->datbnk = 9;                        // 8 for single
    }
 }


void set_rescan(SBK *s)
 {
   s->curaddr  = s->start;
   s->nextaddr = s->start;
   s->stop     = 0;
   s->inv      = 0;             //allow rescan and restart
        #ifdef XDBGX
   DBGPRT(1, "SET RESCAN %x U%d S%d", s->start, s->ssjp, s->ssub);
   #endif
 }


 SBK *add_scan (uint ch, uint start, uint end, int type, SBK *caller)
 {

 /* add code scan.  Will autocreate a subr for type "SUBR"
  * updates type etc even for duplicates to fit 'rescan' type model
  * C_CMD (+32) = by user command (can't be changed)
  * caller is only set in calls from do-sjsubr and taks lists (vect)
  * allow dummy blocks with type = 0 ??
  */

   CHAIN *x;
   SBK *s;

   int ans;
   uint ix;

   #ifdef XDBGX
    const char *dbgtxt;
   #endif

   x = get_chain(ch);

   if (get_anlpass() >= ANLPRT)   return 0;

   // TEMP add a safety barrier ...
   if (x->num > 20000) return 0;

   if (!val_rom_addr(start))
    {
     x->lasterr = E_INVA;
     #ifdef XDBGX
       DBGPRT (1,"Invalid addr %x", start);
     #endif

     return NULL;
    }


   if (get_cmd (CHAUX, start, C_XCODE))
     {
      #ifdef XDBGX
      DBGPRT(0,"XCODE bans scan %x", start);
      DBGPRT(1,0);
      #endif
      x->lasterr = E_XCOD;
      return 0;
     }

   if (get_cmdopt(OPTMANL))
      {
       #ifdef XDBGX
       DBGPRT(1,"no scan %x, manual mode", start);
       #endif
       return 0;
      }

   s = (SBK *) chmem(ch,0);           // new block (zeroed)

   s->start    = start;
   s->curaddr  = start;
   s->nextaddr = start;
   s->chn      = ch;        // which chain
   s->proctype = 1;

   ix = bfindix(x, s);            //only on ->start

   ans = x->cheq(x, ix,s);

   if (ans)
    {        // no match - do insert

     chinsert(ch, ix, s);

     #ifdef XDBGX
     dbgtxt = "Add";
     #endif
      // s is answer, do not return as stack update is below.
    }
   else
    {                                 // match - duplicate
     s = (SBK*) x->ptrs[ix];          // block which matches
     x->lasterr = E_DUPL;

     #ifdef XDBGX
      dbgtxt = 0;
      DBGPRT(1,"Dup Scan %x", start);
     #endif


if (!s->stop)
{
    DBGPRT(0,0);         //but duplicate has not been scanned yet !!!
}

    }



   if (s == caller)
    {   // this is possible with a jump back to block start
         #ifdef XDBGX
     DBG_sbk("Reject, new = caller",s);
     #endif
     return 0;                      // don't allow recursive call.
    }


   // even if a duplicate, reset following items, and stack for each call/jump

   if (type & C_USR) s->usrcmd = 1;         // added by user command

   ix = type & 0x1f;

  // s->jcaller = 0;

   if (ix == J_COND || ix == J_STAT)
     {
       if (!ans && !s->ssjp) set_rescan(s);                // duplicate, but not same scan type
       x->lasterr = 0;
       s->ssjp = 1;
 //      s->jcaller = caller;                      // jump from
     }

   if (ix == J_SUB || ix == J_VECT)
     {
       if (!ans && !s->ssub) set_rescan(s);
       x->lasterr = 0;
       s->ssub = 1;
 //      s->jcaller = 0;                        // not a jump
     }


  if (caller)
      {         // jump, so same caller, same subroutine, same stack.
//       s->logdata  = caller->logdata;        // keep nodata flag
       s->proctype = caller->proctype;       // keep processing type
       s->stkptr   = caller->stkptr;
       s->psw      = caller->psw;

       // copy entire stack and pointer from caller

       memcpy(s->stack,caller->stack, sizeof(s->stack));

       if (s->ssub)
        {

         if (ix == J_SUB)
           {   // new CALL or VECT CALL from caller, push caller on to stack
               // NB. push is  stkptr -= 2; [stkptr] = [Rx];
             fake_push(s,1, caller);
           }

      // if stat do jump history as well ?




         if (s->proctype == 1 && !x->lasterr)
          {
           add_subr(s->start);                                // don't add if gap scanning
           if (!chsubr.lasterr) new_autosym(1,s->start);      // add name (autonumbered subr)
          }
         }
      }            //end if caller



 // if (type & C_TST) s->proctype = 2;     // verify/gap scanning

  set_scan_banks(s, caller);

 // s->popreg  = 0;

  #ifdef XDBGX
   if (dbgtxt) DBG_sbk(dbgtxt,s);
  #endif
  return s;
}



SBK *add_escan (uint add, SBK *caller)
{

 /* add emu scan.
  * if addr is zero, COPY caller entirely. (for scan->emu copies),
  * else this is J_SUB scan and caller will always be set
  */

   SBK *s;

   int ans;
   uint ix;

   if (!caller || !caller->proctype) return 0;

   s = (SBK *) chmem(CHSTST,0);             // new block (zeroed)
   if (add) s->start = add;                 // standard add emu
   else s->start    = caller->start;        // copy of SBK to new emu for correct place in chain

  if (!val_rom_addr(s->start))
    {
     chstst.lasterr = E_INVA;
     #ifdef XDBGX
       DBGPRT (1,"Escan Invalid addr %x", s->start);         //never happens ?? should not
     #endif
      return 0;
    }


   ix = bfindix(&chstst, s);
   ans = cpscan(&chstst,ix,s);             // checks start address ONLY

   if (!ans)
    {                                      // match - duplicate
     s = (SBK*) chstst.ptrs[ix];           // block which matches
    }
   else
    {        // no match - do insert (s is valid pointer)
     chinsert(CHSTST, ix, s);
    }

   if (!add)
    {
      *s = *caller;                          // copy whole block exactly for emulate
    }
   else
    {
      //always a J_SUB - new block
      s->stkptr   = caller->stkptr;
      s->psw      = caller->psw;

      s->curaddr  = s->start;                   // always restart for new emulates
      s->nextaddr = s->start;
      set_scan_banks(s, caller);

   //but if args found then maybe stack is not up to date, so check it first.
     {
       int i;
       SBK *x;
       STACK *k;

       for (i = 7; i >= 0; i--)
          {
            k = caller->stack+i;
            if (!k->type) break;
            x = get_scan(CHSTST, k->start);
            if (x)
              {
                if (k->nextaddr != x->nextaddr)
                 {
                #ifdef XDBGX
                 DBGPRT (1,"Stack %x != blk %x FIX", k->nextaddr, x->nextaddr);
                 #endif
                 k->nextaddr = x->nextaddr;
                 }
              }
          }
     }
      memcpy(s->stack,caller->stack, sizeof(s->stack));
      fake_push(s,1,caller);
    }

   // even if a duplicate, can be rescanned as emulate again

   s->proctype = 4;               // emulating
   s->stop     = 0;
   s->inv      = 0;               // and always set for rescan
 //  s->popreg   = 0;
   s->estop    = 0;

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," EMUscan %x", s->start);
    if (!add) DBGPRT(0," Copy"); else
    DBGPRT(0," from %x" , caller->curaddr);
    DBGPRT(1,0);
   #endif
 return s;
}


uint find_argblk(SBK *s, uint ix, OPS *o)
 {
  RST *g;
  RPOP *p;
  STACK *k;

  const OPC *opl;
  uint ofst;
  SBK *scb;

     // SBK *test, *test1;                 //DEBUG


 // if (!k) return 0;

  k = get_stack(s,ix);
  g = get_rgstat(o->addr);

  if (!g) return 0;


  #ifdef XDBGX
     DBGPRT(1,"%x find argblk %x %x", s->curaddr, k->start, k->nextaddr);
  #endif

  //drop p but use inside and set for rstat g....

  // FKL *add_link(uchar chsce, void *sce, uchar chdst, void *dst, uchar type)


 // p->prtarget = 0;
 // p->petarget = 0;          //safety
 // scb = 0;                // probably redundant     //g->scanblk = 0;
 // g->argsblk = 0;

  ofst = find_opcode(0, k->nextaddr, &opl);   // get opcode immediately before possible args

  // look for CALL,   if jump....

  if (!ofst || opl->sigix != 17)
   {
    #ifdef XDBGX
      DBGPRT(1,"%x Not found CALL !!", ofst);
    #endif

    // MORE HERE !! to find a jump ??? should be a subr if have args
    return 0;
   }

  do_one_opcode(ofst, &tscan, &tinst);

      //g->scanblk = get_scan(CHSCAN, tinst.opr[1].addr);       // scan version


  scb = get_scan(CHSCAN, tinst.opr[1].addr);       // 'real' scan version
   //  p->tgtscan = get_scan(CHSCAN, tinst.opr[1].addr);       // 'real' scan version

  if (!scb)
      {
               #ifdef XDBGX
        DBGPRT(1,"NO SCAN BLK!!"); //create one ??  can only happen in emulate, can the whole emulate??
        #endif
        return 0;
      }

  if (scb->adtsize)
      {
        #ifdef XDBGX
          DBGPRT(1,"args ignored - preset size");           //preset size, no emul
        #endif
        return 0;
      }


 // x->popreg = o->addr;


  g->popped = 1;
  g->pinx = s->stkptr+ix;            // popped and pointer index

  p = get_rpop(g->pinx);

  p->popreg = o->addr;

  p->tgtscan = scb;


/*      move this in here
 *   if (type == 1)    //true CALLs only - do argblks
     {    //find_arg_blks(s,o);

      g = get_rgstat(o->addr);
      if (x)       // from local stack
        {
         o->val = x->nextaddr;
         x->popreg = o->addr;

p is NULL
         if (find_call(p,s,x))          //P not set !!!!
             {
              g->popped = add_rpop(o->addr, &p);  // was 1;     // find subr to which args get added
         //     add_pop_reg(o->addr);
             }

         DBGPRT(0, "Pop R%x", o->addr);

       if (p->tgtscan)  DBGPRT(0, " tgt[0] = %x (%x)", p->tgtscan->nextaddr,p->tgtscan->start);
       if (p->argblk[0])  DBGPRT(0, " args[0]= %x (%x)", p->argblk[0]->nextaddr,p->argblk[0]->start);

       /,*
      if (s->proctype == 4)
      {
 if (p->petarget)  DBGPRT(0, " petarget = %x (%x)", p->petarget->nextaddr,p->petarget->start);
       if (p->argseblk)  DBGPRT(0, " argseblk = %x (%x)", p->argseblk->nextaddr,p->argseblk->start);
      }*,/



       DBGPRT(1,0);
        }

*/



 // scx = get_scan(CHSTST, tinst.opr[1].addr);       // 'emu' scan version (ptarget)

  if (s->proctype == 4)
    {
      p->tgtemu = get_scan(CHSTST, tinst.opr[1].addr);       // emu version
      if (!p->tgtemu)
         {
           #ifdef XDBGX
        //   DBGPRT(1,"NO EMU SCAN pointer !! STOP (%x)", p->tgtscan->start);
                    DBGPRT(1,"NO EMU SCAN pointer - create (%x)", p->tgtscan->start);
           #endif

      // this can happen when a getter is JUMPED to, so create an emu from original

          p->tgtemu = add_escan(0,p->tgtscan);
         }
    }

  if (s->proctype == 1 && p->tgtscan) p->argscan = get_scan(CHSCAN,k->start);


  if (s->proctype == 4 && p->tgtemu) p->argemu = get_scan(CHSTST,k->start);

//  {
   //   g->ptarget = scb;                //g->scanblk;  subr which gets called
      // add_link(CHRST, g, CHSTST, scb,0);             // link g to emu scan
  //  }


 // if (p->prtarget) g->argsblk = x;

   // add_link(CHRST, g, CHSTST, x);             // link x for nextaddr args
 //  DBGPRT(1,"ans 1");

return 1;
 }



uint fake_pop(SBK *s, OPS *o)
{
   // Officially,  Rx = [stkptr];  (SP)+=2;
   // this subr autowraps pointer

   // pop returns value and changes stack pointer
   // but does not erase anything
   // handles psw as well, which multibanks use
   // used for both real and emu
   // o is destination OPS for pop


 //  SBK *x;
   STACK *k;

 //  RST *g;
 //  RPOP *p;


   // do a fake pop via sbk stack

   if (!o) return 0;
   if (!s) return 0;

   if (!valid_reg(o->addr)) return 0;

   k = get_stack(s,0);                       //current stack entry

   // type = get_stack(s,0,&x,&psp);           // from current stkptr




  // type = 1   standard CALL
  // type = 2   immediate address push
  // type = 3   pushp  (PSW)

   if (valid_reg(o->addr) == 2)
    {
      o->val = 0;        // POP 0,  zero reg, ignore it.
     }

   else
   {
     if (k->type == 1)    //true CALLs only - do argblks
       {
        o->val = k->nextaddr;
        k->popreg = o->addr;
        find_argblk(s,0,o);
       }
    if (k->type > 1)
      {
       o->val = k->start & 0xff;
       #ifdef XDBGX
       DBGPRT(0, " POPP or IMM not expected = %x", o->val);
       #endif
      }
   }



  s->stkptr++;                             // move ptr to do the actual POP

  return k->type;

}
  /*    g = get_rgstat(o->addr);
      if (x)       // from local stack
        {
         o->val = x->nextaddr;
         x->popreg = o->addr;

 // p is NULL
         if (find_call(p,s,x))          //P not set !!!!
             {
              g->popped = add_rpop(o->addr, &p);  // was 1;     // find subr to which args get added
         //     add_pop_reg(o->addr);
             }

         DBGPRT(0, "Pop R%x", o->addr);

       if (p->tgtscan)  DBGPRT(0, " tgt[0] = %x (%x)", p->tgtscan->nextaddr,p->tgtscan->start);
       if (p->argblk[0])  DBGPRT(0, " args[0]= %x (%x)", p->argblk[0]->nextaddr,p->argblk[0]->start);

       /,*
      if (s->proctype == 4)
      {
 if (p->petarget)  DBGPRT(0, " petarget = %x (%x)", p->petarget->nextaddr,p->petarget->start);
       if (p->argseblk)  DBGPRT(0, " argseblk = %x (%x)", p->argseblk->nextaddr,p->argseblk->start);
      }/



       DBGPRT(1,0);
        }   */


/* do the field extract and sign from g_val is this kind of trick faster ???

  if (fstart) val >>= fstart;                  // shift down (so start bit -> bit 0)
  mask = 0xffffffff >> (31 + fstart - fend);   // make mask of field to match
  val &= mask;                                 // and extract required field
*/

uint fmask(uint fstart, uint fend)
{
   uint ans, i;
   ans = 0;

   fend &= 0x1f;                       // safety
   for (i = fstart; i <= fend; i++)
     {
       ans |= (1 << i);
     }
   return ans;
}





void fixbitfield(SYM *s, uint add, uint fstart, uint fend)
{
 uint i, lfend;

// move start to correct byte address for syms
// fend is field end plus sign (0x20) plus write (+x40)
// fend also extra flags, such as SYS and CMD.
// CHANGE RULES for byte defaults

 lfend = (fend & 0x1f);                 // 0 -> 31, get size only.
 if (fend & 0x60) s->pbkt = 1;          // sign or write set

 if (fend & C_USR) s->usrcmd = 1;         // by user command

 if (fend & C_SYS) s->sys  = 1;           // system generated

 // map fstart (then fend) into correct BYTE address

 fstart &= 0x1f;              // safety, max 31

 if (fstart > 7)
    {
      i = (fstart)/8;     // number of extra bytes
      add += i;           // add the bytes to start addr
      i *= 8;             // bits to subtract
      fstart -= i;

      if (lfend > fstart) lfend -= i;       // subtract from end too, stay in sync
      if (lfend < fstart) lfend  = fstart;  // safety, 1 bit
    }

   s->fmask = fmask(fstart, lfend);         // set up field mask for later use

  i = lfend % 8;                            //  remainder

  if (fstart || i !=7) s->pbits = 1;        // bits for anything not whole bytes

  if (fstart == 0 && i == 7)  s->whole = 1; // whole num of bytes, can be default

  lfend |= (fend & 0x60);           //  put back signed and write flags
  s->addr   = add;
  s->fstart = fstart;
  s->fend   = lfend;

}

void mark_ol_syms(uint ix)
{
    // done after insert, so all checks passed.
    // check if

SYM *ch, *t;

if (!ix) return;

ch = (SYM *) chsym.ptrs[ix];         // chain block to check
t =  (SYM *) chsym.ptrs[ix-1];       // previous (larger?) entry

if (ch->addr & 1)  return;           // odd addresses must be byte anyway
if (t->addr != ch->addr) return;      // addresses don't match. OK

if ((ch->fend & 0x60) != (t->fend & 0x60)) return;    //sign,write don't match

if (t->whole && ch->whole)
  {
    if (t->fend > ch->fend) ch->pbits = 1;
    else t->pbits = 1;
  }

}


void check_sym_range(SYM* s, SYM* t)
{

   // address range checks.

   if (!s->rstart && !t->rstart && s->rend == 0xfffff && t->rend == 0xfffff)
     return;        // no ranges specified, keep error

   chsym.lasterr = 0;           // clean sym overlap error

   if (s->rstart < t->rstart && s->rend > t->rend) return;         // spans do separately.
   if (s->rstart > t->rstart && s->rend < t->rend) return;         // contained
   if (s->rend   < t->rstart)   return;                            // before
   if (s->rstart > t->rend)     return;                            // after

   chsym.lasterr = E_OVRG; // range overlaps

}



void check_sym_overlaps(SYM* s, SYM* t)
{
   // for range and bit field overlaps.
// t is chained, s is new

/*error numbers
#define E_DUPL  1           // duplicate

#define E_OVLP   7          // overlap addresses
#define E_OVRG   8          // overlapping ranges

*/


   // if bit range, check no overlaps....
   //if addr and write matches and range overlaps - check bits?
   //if addr and write and bits overlap  - check range???

   chsym.lasterr = 0;           // no error yet

   if (t->addr != s->addr)  return;                     // address, OK

   if ((t->fend & 0x60) != (s->fend & 0x60)) return;    // read versus write, sign, OK

 //simpler - fields can overlap but not be identical

   if (s->fstart != t->fstart) return;
   if (s->fend   != t->fend)   return;


//drop overlap checks - not required...
 //  if (s->whole != t->whole)  return;                   // bit versus default/whole, OK
 //  if (s->whole && t->whole)
 //    {
       // only whole syms can overlap as long as different sizes
 //      if (s->fend != t->fend) return;
 //    }

   // bit field overlap checks

 //  if (s->fend   < t->fstart)  return;   // before
 //  if (s->fstart > t->fend)    return;   // after
 // chsym.lasterr = E_OVLP;     // sym overlaps, check ranges

   chsym.lasterr = E_DUPL;     // sym duplicate, check ranges

}


void new_symname (SYM *xsym, cchar *fnam)
 {
   // replace symbol name
   // symname cannot be NULL

  int oldsize;
  if (!fnam || !*fnam) return ;                 // must have a name
  if (xsym->usrcmd && xsym->name) return ;      // name set by user CMD

  oldsize = xsym->symsize;                    // old size, with null
  xsym->symsize = strlen (fnam) + 1;          // new size, with null

  if (xsym->symsize > SYMSZE) xsym->symsize = SYMSZE;   // safety check

  if (xsym->name && strcmp(xsym->name, fnam)) chsym.lasterr =  E_NAMREP;  // mark replacement

  xsym->name = (char *) mem (xsym->name,oldsize,xsym->symsize);  // get new sym size

  if (xsym->name)
    {
      strncpy (xsym->name, fnam, xsym->symsize);                   // and replace it
      xsym->name[xsym->symsize-1] = '\0';                          // safety end of string
    }
 }







SYM *add_sym (cchar *fnam, uint add, uint fstart, uint fend, uint rstart, uint rend)

 {
   // chain for symbols bitno = -1 for no bit
   // fend as OPS as field end plus sign (+32) plus write (+64)

   SYM *s, *t;
   int ans;
   uint ix;


   if (get_anlpass() >= ANLPRT) return 0;
   if (!fnam || !*fnam) return 0;          // must have a symname.

   s = (SYM *) chmem(CHSYM,0);

   //map into byte addressed (ans other flags)

   fixbitfield (s, add, fstart, fend);

   s->rstart  = rstart;                    //range start and end
   s->rend    = rend;

   ix = bfindix(&chsym, s);

   ans = chsym.cheq(&chsym, ix, s);              // zero if matches

   t = (SYM *) chsym.ptrs[ix];         // chain block found

// !ans is an exact duplicate, including addr,fstart,fend, start range, but not end range.

    if (!ans)
      {       //duplicate

        #ifdef XDBGX
          t = (SYM*) chsym.ptrs[ix];
          DBGPRT(0,"Dup sym %x",add);
          DBGPRT(0," %c%s%c " ,'"',fnam, '"');
          DBGPRT(0," with ");
          DBGPRT(0," %c%s%c " ,'"',t->name, '"');
          DBGPRT(1,0);
        #endif

       s = (SYM*) chsym.ptrs[ix];
       chsym.lasterr = E_DUPL;
       if (fend & C_REN)  new_symname (s, fnam);     // allow name replace, will reset error
       return s;
     }

   if (ix < chsym.num)
     {
      check_sym_overlaps(s, t);      // sets last err if any overlaps.

      if (chsym.lasterr) check_sym_range(s,t);  // OK if no range error

      if (chsym.lasterr)             // may need multiple syms checked ??
       {
       #ifdef XDBGX
        DBGPRT(0,"add sym %x",add);
        DBGPRT(0," %c%s%c " ,'"',fnam, '"');
        if (chsym.lasterr ==  E_OVRG)  DBGPRT(0,"range"); else DBGPRT(0,"field");
        DBGPRT(0," overlap with ");
        DBGPRT(0," %c%s%c " ,'"',t->name, '"');
        DBGPRT(1,0);
        #endif
        return 0;
       }

    }

     // do insert
     new_symname (s, fnam);

     chinsert(CHSYM, ix, s);

     mark_ol_syms(ix);

     #ifdef XDBGX
      DBGPRT(0,"add sym %x",add);
      if (fnam) DBGPRT(0," %c%s%c " ,'"',s->name, '"');
      if (s->rstart)      DBGPRT (0," %05x - %05x" , s->rstart, s->rend);
      DBGPRT (0," B%d %d", s->fstart, s->fend);
      if (s->fend & 64)  DBGPRT(0," write");        //s->writ
      if (s->pbkt) DBGPRT(0," pbkt");
      if (s->immok) DBGPRT(0," imm");
      if (s->usrcmd)   DBGPRT(0,"  USR");
      DBGPRT(1,0);
     #endif

 return s;

}



MATHX *get_mterm(void *fid, int pix)
{
 uint ix;
 MATHX *m;

 m = (MATHX*) chmem(CHMATHX,1);
 m->fid = fid;
 m->pix = pix;    // and param index

 ix = bfindix(&chmathx, m);

 if (!chmathx.cheq(&chmathx, ix,m))
 {
   return (MATHX*) chmathx.ptrs[ix];
 }

chmathx.lastix = chmathx.num;   //invalidate if not found
return NULL;
}





MATHX* add_mterm (void *fid, int pix)
  {
   // add additional data block with fid as key

   // sys embedded ones have name of function........for now.
   // allow fid = zero, so a->fid = a + 5 as dummy key
   // then link name IN to a ?

  MATHX *a;
  uint ix;
  long ans;

  a = (MATHX *) chmem(CHMATHX,0);

  a->fid = fid;    //parent
  a->pix = pix;    // and param index

  ix = bfindix(&chmathx,a);

  ans = chmathx.cheq(&chmathx, ix, a);

  if (!ans)

  { // exists already.  Clear it and reset for use. may not be necessary ?
   chmathx.lasterr = E_DUPL;
   // memset ?
   return (MATHX *) chmathx.ptrs[ix];      //   return NULL;
  }

    chinsert(CHMATHX, ix, a);

  chmathx.lastix = chmathx.num;        //must reset any queries
  return a;
}






MATHN *get_mname(void *fid, char *name )
{
 uint ix;
 MATHN *m;

//allow empty name ???

 m = (MATHN*) chmem(CHMATHN,1);
 m->fid = fid;
 m->mathname = name;               //just copy pointer for search
 if (name) m->nsize = strlen(name) + 1;

 ix = bfindix(&chmathn, m);


 if (!chmathn.cheq(&chmathn, ix,m))
 {
   return (MATHN*) chmathn.ptrs[ix];
 }

chmathn.lastix = chmathn.num;   // invalidate if not found
return NULL;
}



MATHN *add_mname(void *fid, cchar *name)
{

  //  create a 'base' math function, by its name

  MATHN *n;
  int ans;
  uint ix;

//  allow empty name and search by fid.   TO DO

  n = (MATHN *) chmem(CHMATHN,0);
  n->fid = fid;

  if (!name) n->nsize = 0;          //no name
  else n->nsize = strlen (name) + 1;          // include null, strlen does not...

  if (n->nsize)
    {
     n->mathname = (char *) mem (0,0,n->nsize+1);    // get new sym size, allow for null
     strncpy (n->mathname, name, n->nsize);  // and copy it
     n->mathname[n->nsize] = '\0';
    }

  ix = bfindix(&chmathn,n);

  ans = chmathn.cheq(&chmathn, ix, n);

  if (!ans)
  {  // exists already.  Clear it and reset for use. may not be necessary ?
  //  allow a duplicate.............
    chmathn.lasterr = E_DUPL;
    return (MATHN*) chmathn.ptrs[ix];                //NULL;
  }

  chinsert(CHMATHN, ix, n);

  chmathn.lastix = chmathn.num;        //must reset any queries
  return n;
}





MATHN *add_sys_mname(void *fid, uint ix)
{
  char *t;
  AUTON *names;

  names = get_autonames();

  names += ix;         //get to specified entry

  strcpy (tempname,names->pname);

  t = tempname;

  while (*t) t++;        //to end of string
// only if params, and assume int val ???
  sprintf(t,"%u",names->spf++);

  return add_mname(fid,tempname);
}

/*
MATHX* add_smtermx (void *fid, uint func, MNUM *p1, MNUM *p2)
{
//adds a system calc, with name (from ix), and links to fid (= adt)

// how to stop duplicates ?????

  MATHN *n;
  MATHX *c;
  MXF *fc;
  MNUM *p;

 // int i;

  c = 0;

 make_sysname(func,p1);

 fc = get_mathfunc(func);
 n = add_mname(tempname);     // add temp system name

 // assume lasterr is only ever DUPL.....

 if (n && !chmathn.lasterr)
  {            // name valid
    n->calctype =  fc->defctype;        // address calc (enc)
  }

//if n is a duplicate name, still attempt add mathterm

  c = add_mterm(n,0);

  if (c && !chmathx.lasterr)
     {
      c->func = func;                // actual sys func
      c->calctype = fc->defctype;    // calctype
      c->calc = fc->calc;
      c->fbkt = fc->bkt;
   //   c->sys = 1;
      c->npars = fc->fnpars;         // num pars

      p = c->fpar;
      p->ival = 1;             // ref (=1)
      p->dptype = DP_REF;         // = reference (=x)

// assumes sys funcs have only one or two params.....but always an X

    //  p =
 *(c->fpar + 1) = *p1;
 *(c->fpar + 2) = *p2;
   //   p->ival = p1;
   //   p->datatype = 1;   // integer
 //     *p = *p1;


  //    p = c->fpar + 2;
   //   p->ival = p2;
  //    p->datatype = 1;   // integer
//p = *p2;



     }

// if n or c is duplicate, still add link

    add_link(CHADNL,fid,CHMATHX,c,1);        //safe if fid or c is null

return c;              //but this should not return zero for a duplicate.
}
*/



uint fix_bare_addr(uint addr)
{
  // sort out bank number and default
  // remember bank+1 throughout

  uint x;
  x = nobank(addr);

  // < 0x400 is a register, no bank
  if (valid_reg(x)) return x;

  // single banks always 9 (databank)
  if (!get_numbanks())      return x | 0x90000;

  // no bank and multibank, default to databank, may not always be right !!
  if (!g_bank(addr))  return x | 0x20000;

  return addr;
}




uint *get_syms(uint add, uint fstart, uint fend, uint pc)
{
    // get ALL possible address matches.
    // fstart used for field mode, fend used against access size

    // list [0] is read default, [1] is write

  SYM *s, *t;
  uint ix, end, num;
  uint *list;

  list = (uint*) mem (0,0, 70*sizeof(uint));
  for (ix = 0; ix < 70; ix++) list[ix] = chsym.num+1;

  add = fix_bare_addr(add);         //  for bank, not bit

  end = add + bytes(fend) -1;        //for overlap check, size of search

  s = (SYM*) chmem(CHSYM,3);         // block for search , a little gap..........

  s->addr   = add;
  s->fend   = 0x7f;     // largest possible fend (31 + write + sign)
  s->rstart = pc;
  s->rend   = pc;                   // range required for binary search..........
  ix = bfindix(&chsym, s);

  num = 2;                              //defaults at [0] and [1]
  while (ix < chsym.num && num < 70)
    {
     uint z;
     t = (SYM*) chsym.ptrs[ix];        // chain entry found

     t->prtadj = t->addr - add;        // keep track of multiple byte addresses

     if (t->addr > end) break;         // address not match

     if (pc >= t->rstart && pc <= t->rend)
       {         // within sym range, deflt read in [0] , deflt write in [1]

        if (t->whole)
         {     // 'whole' symbols byte or word can be default sym.
               // longest occurs first, read and write
           if (t->fend & 0x40) z = 1; else z = 0;      //which entry, read or write

           if (list[z] > chsym.num) list[z] = ix;                // new match (will be largest)
           else
            {
              // smaller than largest, but nearer match by size. fstart is zero if whole.
              if (t->fend > fend) list[z] = ix;
            }
         }
        else
         {
           list[num++] = ix;              // add to list
         }
       }
     ix++;
    }
  return list;      //num of non default matches
}



SYM* get_sym(uint add, uint fstart, uint fend, uint pc)
{
    uint i;
    SYM *s, *t;
    uint *list;

    s = 0;

   list = get_syms(add, fstart, fend, pc);

    // list has all valid symbols by address and range.
    // fend includes read/write, signed,unsigned

    t = (SYM*) chsym.ptrs[chsym.newins];                //get sym from chmem in get_syms

    fixbitfield(t, add, fstart, fend);                  //adjust 't' to correct bits and address

    // non defaults start at 2, null end marker
    for (i = 2; i < 70; i++)
        { // check each sym for best match
          if (list[i] >= chsym.num) break;     //end of list

         s = (SYM*) chsym.ptrs[list[i]];

         if (s->fstart == t->fstart && s->fend == t->fend && s->addr == t->addr)
           {
            break;         //perfect match first (includes read/write)
           }
        }

     if (list[i] < chsym.num) return s;        // found perfect match

     // no match if it got here, so use default

     if (list[1] < chsym.num && (fend & 0x40)) return  (SYM*) chsym.ptrs[list[1]];      // default write sym
     if (list[0] < chsym.num)                  return  (SYM*) chsym.ptrs[list[0]];      // default sym (read or write)

    mfree(list,70*sizeof(uint));

    return 0;                                          // nothing found
}



JMP *get_fjump(uint addr)
{
  JMP *j;
  uint ix;

  j = (JMP*) chmem(CHJPF,0);
  j->fromaddr = addr;

  ix = bfindix(&chjpf, j);


  if (chjpf.cheq(&chjpf, ix ,j)) return NULL;    // no match
  return (JMP*) chjpf.ptrs[ix];
}



JMP *add_jump (SBK *s, uint to, int type)
{
  JMP *j;
  const OPC *x;
  uint ix;
int ans;

  if (get_anlpass() >= ANLPRT) return NULL;

  if (!val_rom_addr(to))
    {
     #ifdef XDBGX
       ix  = (nobank(to));   // don't want cal_cons
       if (ix >= 0xd000 && ix <= 0xd010) return NULL;
       if (ix == 0x1f1c || ix == 0x1000 || ix == 0x1800) return NULL;
       DBGPRT(1,"Invalid jump to %x from %x", to, s->curaddr);
     #endif
     return NULL;
    }

  j = (JMP *) chmem(CHJPF,0);
  j->toaddr     = to;
  j->fromaddr   = s->curaddr;
 // j->fsubaddr   = s->subaddr;
  j->jtype      = type;
  j->size       = s->nextaddr-s->curaddr;   // excludes compare


  // is this preceeded by a compare ?
  // get size of cmp as well

  ix = find_opcode(0, s->curaddr, &x);

  if (x && x->sigix == 10)  j->cmpcnt = s->curaddr - ix;   // count back for cmp


  if (!bankeq(j->toaddr, j->fromaddr))  j->bswp = 1;        // bank change
  else
  if (j->jtype && j->fromaddr >= j->toaddr)
     {
      j->back = 1;
     }


  // insert into the two jump chains

  ix = bfindix(&chjpf, j);      // FROM chain
  ans = chjpf.cheq(&chjpf, ix, j);

  if (!ans)
   {
      j =  (JMP *) chjpf.ptrs[ix];             // match - duplicate
      chjpf.lasterr = E_DUPL;
   }
  else
   {

     chinsert(CHJPF, ix, j);                    // do insert in FROM chain
     ix = bfindix(&chjpt, j);                 // and in TO chain
     chinsert(CHJPT, ix, j);
   }



//  check_jump_span(j);

 #ifdef XDBGX
    if (ans)  DBGPRT (0,"Add"); else DBGPRT (0,"Dup");
    DBGPRT (0," jump %x-%x %s", j->fromaddr, j->toaddr, get_jtext(j->jtype) );
    if (j->retn)  DBGPRT(0," (ret)");
    DBGPRT(1,0);
   #endif


  return j;
}










uint get_tjmp_ix(uint ofst)
{
  // find to-jump - as this is multiple,
  // bfindix (via cpjump) will find first one

  JMP * j;
  uint ix;

  if (!chjpt.num) return 1;

  j = (JMP*) chmem(CHJPF,0);
  j->toaddr = ofst;
  ix = bfindix(&chjpt, j);

  // bfindix WILL find first to jump as from is zero (code in cpjmp)

  if (ix >= chjpt.num) return ix;

  j = (JMP*) chjpt.ptrs[ix];

  if (j->toaddr != ofst) return chjpt.num;

  return ix;            //which is also lastix....
}


SIG * get_sig (uint ofst)
{
  SIG *t, *s;
  uint ix;

  t = (SIG*) chmem(CHSIG,1);
  t->start = ofst;

  ix = bfindix(&chsig, t);

  if (ix >= chsig.num) return  NULL;    // no match
  //check it's right address ....
  s = (SIG*) chsig.ptrs[ix];

  if (s->start != ofst) return NULL;


  return s;               //(SIG*) chsig.ptrs[ix];

}

LBK *get_prt_cmd(LBK *dflt, BANK *b, uint ofst)
  {
    // find next command for printout from cmd chain
    // if none found use a default (fill) block

    LBK *c  ;
    int ixc ;

    dflt->start = ofst;
    dflt->end = 0;
    dflt->dflt = 1;

    ixc = bfindix(&chcmd,dflt);
    c = (LBK*) chcmd.ptrs[ixc];

    if (!eqcmd(&chcmd,ixc,dflt)) return c;    // match

    // not match, c will be next block,  get an end address, setup default blk

    dflt->end = b->maxromadd;      // max for bank

    if (dflt->end > c->start && ofst < c->start)  dflt->end = (c->start-1);   // dflt up to c (next blk)

    dflt->fcom = C_DFLT;

    if (dflt->end < dflt->start && dflt->end < b->maxromadd)
     {
       #ifdef XDBGX
        DBGPRT(0,"PANIC for %x  %x!!! ", dflt->start, dflt->end);
        DBGLBK(0,c,"NEXT");
       #endif
       dflt->end = dflt->start+1;
     }

return dflt;

}

//find_next_tj

uint find_tjump (uint ofst)
{
  // find trailing jump, for brackets, matches fjump
  // prints close bracket for each jump found

 JMP * j;
 uint ix, ans;

 ans = 0;
 ix = get_tjmp_ix(ofst);   // find first 'to' jump

 while (ix < chjpt.num)
  {
   j = (JMP*) chjpt.ptrs[ix];
   if (j->toaddr != ofst) break;    // stop as soon as different address

   if (j->cbkt) ans++;
   ix++;
  }
  return ans;
}

/*
JMP *find_fjump (uint ofst, int *x)
{
  // find forward (front) jumps and mark, for bracket sorting.
  // returns 8 for 'else', 4 for while, 2 for return, 1 for bracket
  // (j=1) for code reverse

  JMP *j;

 // if (get_anlpass() < ANLPRT) return 0;

  *x = 0;

  j = get_fjump(ofst);

  if (!j) return 0;


  if (!j->jtype)  *x = 0;   // true return (RET)

  if (j->retn)  *x = 2;      // jump to a return

 // if (j->jtype == J_ELSE) *x = 4;
  if (j->obkt) *x = 1;     // bracket - reverse condition in source

 // if (j->jtype == J_NULL) *x = 8;
// if (j->jelse) *x |= 4;          //pstr(" } else { ");
//if (j->jor)   pstr(0," ** OR **");

  return j;
}*/


uint bchop(uint *x, uint val)
{
  // local version of bfindix for arrays. Assumes count in x[0].
  // uses c (as x+1) for the data items
  // array addr, val, array size

  uint tst, min, max;
  uint *c;

  min = 0;
  max = x[0];
  c = x+1;

  while (min < max)
    {
      tst = min + ((max - min) / 2);               // safest way to do midpoint of (min, max);
      if(val > c[tst]) min = tst + 1;              // val > tst, so new minimum is (tst+1)
      else  max = tst;                             // val <= tst, so new maximum is tst
    }
  return min;
}


void insval(uint *x, uint ix, uint val)
{
  // insert val at index ix
  // assumes array is contiguous

 if (ix > 32) return;           // array is full (should not happen)

 uint *c;
 c = x+1;

 // move entries up one from ix to make a space.
 memmove(c+ix+1,c+ix, sizeof(uint) * (x[0]-ix));    // fastest for contiguous

 c[ix] = val;                              // insert new block (x->num) at ix
 x[0]++;
}



uint get_unique(uint *x, uint val)
 {
   // count in x[0];

   uint ix;
   uint *c;

   if (!x[0])  return 0;

   c = x+1;

   ix = bchop(x, val);

   if (c[ix] == val) return 1;      //found !!

   return 0;

}


uint add_unique(uint *x, uint val)
 {
   // count in x[0];
   uint ix;
   uint *c;

   c = x+1;

   ix = bchop(x, val);

   if (x[0] && c[ix] == val) return 1;      //found !!

   insval(x, ix, val);
   return 0;
 }



uint score_col(uint ix, uint s, uint c)
{
  // s is score, c is condition

   if (!(mtype[ix] & 0x1000) && c) mtype[ix] += s;
   else
    {
     mtype[ix] = 0x1000;
     c = 0;
    }
 return c;
}

// do one col DOWN.....and make it a LOT stiffer.
// do each type at a time, and stop when it fails.......
// then check other cols

//up to 32 rows of 1-32 cols

//also need END OF BANK safety check

void do_col_template(uint start, INST *cinst)
 {
   uint rows, addr,  w, scix, rowsize, colno, p;  //, end;
   int val, lastval, diff, lastdiff;

   memset(scores, 0, sizeof(scores));        //clear all 1024 scores (32x32)

   for (rowsize = 1; rowsize < 32; rowsize++)
    {
     for (colno = 0; colno < rowsize;  colno++)
      {
        // new run for each column
        memset(mtype, 0, sizeof(mtype));

        //end  =  where proposed struct would end..............for vect check

        valb[0] = 0;   //clear saved values
        addr = start+colno;

        // set up last pars for scores

        val = g_byte(addr);

        lastval  = g_byte(start+(rowsize*2));          //2 elements up
        lastdiff = g_byte(start+(rowsize*3));

        diff = lastdiff - lastval;     //row 3 - row 2
        val  = val - diff;    //to make first row match.

        for (rows = 0; rows < 32;  rows++)
         {
          // rows within the column, up to 32 rows, (go down col)
          // and look for possible structure.

          scix = (rowsize*32)+rows;     //  to get to right cell

       //   end = start + (rowsize*rows);      //where end of struct would be (-1 ?)
          addr = start+colno+(rowsize*rows);

          lastval = val;
          val = g_byte(addr);

          lastdiff = diff;
          diff = val - lastval;                             // set value difference.

          p = __builtin_popcount(val);

          // = 1 if repeat value, add and order column values (byte)
          if (add_unique(valb,val)) mtype[8] += 2;    // +1 if repeat value, add and order column values (byte)

          score_col(0,2,(p == 1));    // bit mask, byte
          score_col(1,2,(p == 7));

          w = g_word(addr);
          p = __builtin_popcount(w);

          score_col(2,2,(p == 1));        // bit mask, word
          score_col(3,2,(p == 15));

//for debug
       //   p = val_rom_addr(w);
       //   p = get_opflag(w);
       //   p = get_datflag(w);


          // vect, check ROM addr, no olap, no data ?
          //add mtype check to stop lots of useless checks
     //     w |= g_bank(start);               // maybe not for multibanks ???

        //  if (!(mtype[4] & 0x1000)) score_col(4,1,check_vect(w, end, cinst));

          score_col(6,1,(diff > 0 && lastdiff > 0));  // sequence goes up

          score_col(7,1,(diff < 0 && lastdiff < 0));  // sequence goes down

          // assign score here, always zero in row [0]

         // add difference scores here by ORDERED unique array, not row pick


          lastdiff = valb[2]- valb[1];     // top pair........

          for (w = 2;  w < valb[0]; w++)    //count in [0]
                {
                 if (mtype[5] & 0x1000) break;
                 diff = valb[w] - valb[w-1];
                 if (diff)
                    {  // equals scored elswhere
                     if (diff == lastdiff*2) diff /= 2;   // missing row
                     if (diff == lastdiff) mtype[5] += 2;
                     else  mtype[5] |= 0x1000;
                     lastdiff = diff;
                    }
                }

         // total row score, incremented by each column checked
          p = 1;
          for (w = 0;  w < 8; w++)
             if (!(mtype[w] & 0x1000))
              {
               p = 0;
               scores[scix] += mtype[w];
              }

          if (p) break;            //all cols marked invalid, so abandon rows
        }     // end of row loop for column
      }    // end of column loop


     // now check for max score across rows (col scores are now added up)

       scix = (rowsize*32);       // col 0 score
       p = scores[scix];
       scores[scix] = (scores[scix]*10)/rowsize;   // as for below

       for (rows = 1; rows < 32;  rows++)
         {
          // row scores, drop after highest score

          scix = (rowsize*32)+rows;     //  to get to right cell

          if (scores[scix] > p) {p = scores[scix]; scores[scix] = (scores[scix]*10)/rowsize;}
          else {scores[scix] = 0; p = 0x1000;}
         }

    }    // end of rows loop

}


//no bfindix for stat and pop register structs, uses fixed chain size of 1024 (0x400), add to chain only once.




/*
void * get_rg(uint ch,uint reg)
{
  //combined for rgstat and rgpop as they use same code.........
  CHAIN *x;
  x = get_chain(ch);

  if (!x->pnum) adsch(ch);

  //alloc new block for each register, requested, not at start (like a remote chain).

  reg = nobank(reg);                // safety for indirect addresses
  if (reg > max_reg()) return NULL;

  return x->ptrs[reg];
}

void *add_rg(uint ch,uint reg)
{
  CHAIN *x;
  x = get_chain(ch);

  if (!x->pnum) adsch(ch);

  if (!val_general_reg(reg)) return NULL;          // not any special regs

  reg = nobank(reg);                // safety for indirect addresses

  if (!x->ptrs[reg])
   {                     //alloc only as required
     x->ptrs[reg] = cmem(0,0,x->esize);
   }

  return x->ptrs[reg];
}




RST *add_rgstat(uint reg)
{
RST *x;

    x = (RST*) add_rg(CHRST, reg);
    if (x) x->dreg = reg;
    return x;
}





RPOP *add_rgpop(uint reg)
{
    RPOP *x;

    x =(RPOP*) add_rg(CHPOP, reg);
    if (x) x->dreg = reg;
    return x;
}


RST *get_rgstat(uint reg)
{
       return (RST*) get_rg(CHRST, reg);
}


RPOP *get_rgpop(uint reg)
{
   return (RPOP*) get_rg(CHPOP, reg);

}

*/


uint match_gap(uint start, uint end, uint rowsize)
{
    uint ix, ans;
    DTD *d;

    get_dtd(start);
    ix = chdtd.lastix;

    ans = 0;
    while (ix < chdtd.num)
      {
        d = (DTD *) chdtd.ptrs[ix];      // nearest start
        if (d->stdat > end) break;       // finished

        //check doubled or half ??
        if (d->gap1 == rowsize)
          {
           ans+=2;
            #ifdef XDBGX
           DBGPRT(1,"size %d match at %x (%d)", d->gap1, d->stdat, ans);
           #endif
          }

      if ((d->gap1*2) == rowsize)
          {
           ans ++;
            #ifdef XDBGX
           DBGPRT(1,"size %d doubled at %x (%d)", d->gap1, d->stdat, ans);
           #endif
          }

   //   if (d->gap1 == (rowsize*2)) DBGPRT(1,"size %d half at %x (%d)", d->gap1, d->stdat, ans);

        ix++;
      }


   return ans;
}



uint do_col_patt (uint start, uint *end, INST *cinst)
 {
   uint scix, gp, x;
   int rowsize, rowno;
   uint ans;












   #ifdef XDBGX
     DBGPRT (1, "Do patt analysis %x", start);         //or %x-%x", *start,*end);
   #endif

   do_col_template(start, cinst);

   // scores now in scores array
   //assumes immediates already set up in discover-struct

   #ifdef XDBGX
     DBG_patt(scores);   //print pattern match
   #endif

// now pick out best scores and correlate with immediates and gaps

        ans = 0;
        for (rowsize = 0;  rowsize < 32; rowsize++)
          {    // by colsize

            for (rowno = 31; rowno > 0; rowno--)
             {  // score by rowno

           scix = (rowsize*32) + rowno;    //  to get to right cell

            if (scores[scix])
              {                  // can do this more than once for an end match
                  gp = rowno+1;
                   #ifdef XDBGX
                  DBGPRT(0,"found %d x %d", rowsize, gp);
                  #endif
                  *end = start + (rowsize*gp);
                   #ifdef XDBGX
                  DBGPRT(1,"  end at %x", *end);
                  #endif

                  gp = match_gap(start,*end,rowsize);

                  x = get_unique(datadds,*end);

                  if (!x  && ((*end) & 1) && g_byte(*end) == 0xff)
                    {
                     (*end)++;
                     x = get_unique(datadds,*end);
                     if (x) ans = 0x1000;             // terminator flag
                    }

                  if (x)
                    {
                      if (gp > 1) ans |= rowsize;
                       #ifdef XDBGX
                      DBGPRT(1,"MATCH with %x", *end);
                      if (rowsize & 0x1000)  DBGPRT(1," + TERM");
                      #endif
                    }
                   else ans = 0;

              }
             if (ans) break;         // from this rowno

            }        // end rowno loop
          if (ans) break;         // from this rowsize
          }      //end rowsize loop

if (ans) (*end) --; else *end = start+1;   //true end is one less
   return ans;
 }




uint discover_struct(uint start, uint end, INST *cinst)
 {
   DTD *d, *s;
   LBK *x;
   ADT *a;
   uint ix, apend, rows, term;

   #ifdef XDBGX
    DBGPRT(1,"In discover_struct, %x - %x", start, end);
   #endif

  if (end < start)  return 0;          // safety check

   datadds[0] = 0;

   // look for nearest track (stdat) item - Move this out ??

   s = 0;
   get_dtd(start);

   if (chdtd.lastix >= chdtd.num) return 0;        // not found

   // assemble next immediate data items for struct detection
   //could change this to loopstyle for non-matches, skip down list ??


   ix = chdtd.lastix;
   while (ix < chdtd.num)
    {
      d = (DTD *) chdtd.ptrs[ix];
      if (!s)  s = d;      // first one, nearest start

           if (d->stdat > end) break;

           if ((d->modes & 2))
             {        //immediates only for this list,
               add_unique(datadds,d->stdat);

               #ifdef XDBGX
               DBGPRT(1,"next %x (M%x, S%d, G%d B%d) ", d->stdat, d->modes, d->bsze, d->gap1, d->bcnt);
               #endif
             }
           ix++;
       }


  ix = 1;           //first item in dataadds[1]

  while (ix <= datadds[0])
   {
     start = datadds[ix];
     apend = end;

     rows = do_col_patt(start, &apend, cinst);  // this is col_template + extra verification
 #ifdef XDBGX
     if (rows) DBGPRT(1,"XMATCHX %x rows = %d end = %x", start, rows & 0xff, apend);
#endif
     // if !rows then try next immediate ??

     if (!rows && ix < datadds[0] && datadds[ix+1] < (start+3))
       {         // could be a vect, or a word
        add_cmd(start, apend, s->bsze,0);
        return 1;
       }

     term = (rows & 0x1000) ? 1 : 0;
     rows &= 0xfff;

     #ifdef XDBGX
        DBGPRT(1,"row size = %d",rows);
     #endif

     //check for overlaps here.
     if (rows > 2 && rows < 32)
      {
       x = add_cmd(start,apend,C_STCT,0);
       if (x)
        {
         a = add_adt(x,0);
         if (a)
          {
           a->cnt = rows;
           if (term)
            {
             x->term = 1;
             #ifdef XDBGX
               DBGPRT(1,"with terminator");
             #endif
            }
          }
        }
       return 1;
      }

   #ifdef XDBGX
    DBGPRT(1,"reject rowsize = %x", rows);
   #endif


    // jump to next immediate if answer fails.
    // wouldn't this simply be the next immediate ??


     term = ix;           //current item in dataadds[1]
     apend = start+1;

     while (term <= datadds[0])
       {
        if (datadds[term] > apend)
          {
             ix = term-1;
             break;
          }
        term++;
       }
    ix++;
   }

//HERE _ could check for a function or table or list ????

return 0;              //TEMP !!

}



void do_dtk_links(void)
{   //check links for push (and others?)
uint ix;
int gap;
DTD *d, *x;
TRK *k;
//FKL *l;
//void * fid;

//start in opcode chain, look for multiple data items

 for (ix = 0; ix < chdtk.num; ix++)
      {
        k = (TRK*) chdtk.ptrs[ix];

        if (k->base)
        {
            x = get_dtd(k->base);
            if (x) x->bcnt++;
        }
       if (k->opcsub == 1)
         {
           x = get_dtd(k->rgvl[1]);
            if (x) x->icnt++;
        }

      }


   for (ix = 0; ix < chdtd.num; ix++)
      {
        d = (DTD*) chdtd.ptrs[ix];




//try to get incremented loops to add to bcnt for base........
// not enough here yet..

       if (d->inc && ix > 0)
       {
         uint jx;
         jx = ix-1;
         gap = 0;
         while (jx < chdtk.num)
          {
            x = (DTD*) chdtd.ptrs[jx];
            if (x->stdat != d->stdat - d->inc) break;
            d = x;
            gap++;
            jx--;
          }
         d->bcnt = gap;
       }
      }


}

/*
int chk_code_gap(LBK *s, LBK *n, DTD *d, INST *cinst)
{
  // see if gap is code.......return 1 if processed 0 if not.

   uint start, end, addr, rflag;

   start = s->end+1;           //global for extra checks
   end  = n->start-1;
   rflag = 0;

   if (end <= start) return 0;


   if (chk_aux_olap(start, end))
     {      // does  overlap xcode ??
     #ifdef XDBGX
      DBGPRT(1,"XCODE set for ");
      #endif
      return 0;
     }

   // does gap end or include 0xf0 ?? more likely to be code. Flag it

   addr = start;
   while (addr < end)
      {
       if (g_byte(addr++) == 0xf0)
         {
          rflag = addr-1;
          #ifdef XDBGX
            DBGPRT(1,"0xf0 found at %x", rflag);
          #endif
          break;
         }
      }


   // is data present near front of gap ?

   if (d && d->stdat < end && !d->psh)        //start+5 && !d->psh)
      {
          #ifdef XDBGX
        DBGPRT(1, "data found at %x" , d->stdat);
        #endif

     //   if (!rflag)
         {
        #ifdef XDBGX
        DBGPRT(1, "Abandon code scan");
        #endif
           return 0;        // no scan if no 0xf0 found
         }
      }


      //    need more checks ....
     #ifdef XDBGX
       DBGPRT(1,"Code scan ");
       #endif

   //    while (start < end)
      //    {    // until a valid scan;
         if (! scan_stst(start,cinst) && rflag)                 //need to know if invalid.............

       // might be better to rescan as a 'full' instead....this is too aggressive....

   //   if (turn_scans_into_code ())


   //    if (turn_gapscans_into_code() && rflag)
       {   //invalid scan(s)

     //   start++;
     //
           if (rflag < end) start = rflag + 1;
           else  start = d->stdat + d->bsze;         // desparate try, start after data item

          //if a list, could try adding increments of d->bsze ??
             #ifdef XDBGX
              DBGPRT(1,"Move scan to %x", start);
              #endif
              scan_stst(start, cinst);
             // turn_gapscans_into_code ();
       }


    //   }
    //   else break;
     //     }
          // but this might skip some stuff, so must refind the next gap.
          //   get_cmd(start,0);
          //   if (chcmd.lastix < chcmd.num) ix = chcmd.lastix-1;

         //    continue;
      //  if (start < end )return 1;
               //end code->code gap


       return 1;
}
*/

//before final data but after first batch os scans......


// experimentals

// may as well scan whole bank 1 ???
uint verify_func(uint start, uint end, uint isize)
 {
  uint sadr, rowsize, xsize;
  int ival, uval, lastival, lastuval, uup, sup, endval;
  int sval, lastsval;
  ADT *a, *b;
  LBK  *n;


    DBGPRT(1,"In verify at %x", start);

  if (!isize) return 0;

  sadr   = start;
  endval = 0;
  uup    = 0;            // unsigned
  sup    = 0;            // signed

  if (isize == 0x2f) endval = 0x8000;
  if (isize == 0x27) endval = 0x80;

  xsize = isize & 0x1f;                 //drop sign

  ival = g_val(start, 0,xsize);

  // input must go down from max to min, but not checked for sign.
  // output can go either way, and need to track sign
  // min may repeat at end, not done here


        rowsize = bytes(xsize);
        lastival = ival;
        start += rowsize;
        uval = g_val(start,0,xsize);      // first row
        lastuval = uval;
        sval = g_val(start,0,xsize|0x20);      // first row signed
        lastsval = sval;

        while(ival <= lastival)
          {    //now go down the possible func
            lastival = ival;
            if (start >= end) break;
            start += rowsize;
            ival = g_val(start, 0, xsize);

            start += rowsize;
            lastuval = uval;
            lastsval = sval;

            uval = g_val(start, 0, xsize);
            sval = g_val(start, 0, xsize|0x20);      // first row signed

            if (!uup && uval > lastuval) uup = 1;
            if (!uup && uval < lastuval) uup = -1;

            if (uup ==  1 && uval < lastuval) uup = 3;
            if (uup == -1 && uval > lastuval) uup = 3;

            if (!sup && sval > lastsval) sup = 1;
            if (!sup && sval < lastsval) sup = -1;

            if (sup ==  1 && sval < lastsval) sup = 3;
            if (sup == -1 && sval > lastsval) sup = 3;

            if (uup == 3 && sup == 3) break;

            if (ival == endval)
              { //hit end, reset for full row to handle repeats.
                start += rowsize;  //skip to end of this row
                xsize = (xsize * 2) + 1;                           // full row size (unsigned)
                endval = g_val(start - (rowsize*2),0, xsize);         // and its full row value
                ival = endval;         //temp !!
                break;   //as negative is 32 bit need to add rowsize ??
              }
          }

if (ival != endval)  return 0;            //not found


        //now check end of func rows......need for func no of rows....


  uval = 0;

//  if (end > start + uup) end = start + uup;     // allow 10 rows extra
  //not if it overlaps the next command !!!

  while (start < end)
    {       // any matching final whole rows
     start += bytes(rowsize*2);
     ival = g_val(start,0, xsize);
     uval++;
     if (uval > 8) break;
     if (ival != endval)
     {//start -= bytes(rowsize*2);

     break;}
    }

DBGPRT(1, "out at %x (%x)", start, ival);

if (start > end) start = end;         //safety check


  if ( start - sadr > rowsize * 8)         //at least 4 rows
          {

           n = add_cmd (sadr, start, C_FUNC,0);
           if (n)
           {
           a = add_adt(vconv(n->start),0);
           a->fend = isize;
           a->dptype = DP_DEC;
              set_pfwdef(a);

           b = add_adt(a,0);
           b->fend = isize & 0x1f;        // drop sign.
           b->dptype = DP_DEC;
           if (uup == 3 && (sup == 1 || sup == -1)) b->fend |= 0x20;
              set_pfwdef(b);
           n->size = adn_totsize(n);
           return start- sadr;                 //found
          }


        }
        else DBGPRT(1, "fail");
     return 0;   // not found
 }

void find_func(uint start, uint end)
{
   // 2 or 4 byte functions in gaps

  uint max, ival, size;


  max = maxromadd(start);

  if (end > max) end = max;            //safety end of bank

  while (start < end)
   {


       if (start == 0x223f8)
{
    DBGPRT(0,0);
}


size = 0;


  if (!(start & 1)) // word if even
    {
     ival = g_word(start);
     if (ival == 0xffff) size = verify_func(start,end, 0xf);     // could be a word function, 4 bytes
     else
     if (ival == 0x7fff) size = verify_func(start,end, 0x2f);
    }

  if (!size)
   {
     ival = g_byte(start);
     if (ival == 0xff)   size = verify_func(start,end, 7);       // could be a byte function, 2 bytes
     else
     if (ival == 0x7f)   size = verify_func(start,end, 0x27);
    }

    if (size) start += size;

    else start++;
   }
}


/*

            {



 rowsize = bytes(a->fend) + bytes(b->fend);    // whole row size in BYTES


if (rowsize == 4)
  {          //only word func - check for table dim style (top byte only)
    //   int val, lastval, endval;  declared above int lval;
 //  uint chg;
// prob need more rules (like eval should always be <= lval...but val must change also
// and end up at zero ??)

// need to check not signed as well

  adr  = k->start + bytes(a->fend);            // output row
  eadr = k->end;  // ( - rowsize ?);                               // end
  endval = 1;                           // flag
  vmatch = 0;                           // value changed
  lastval = 0x2000;                     // 32 in top byte
  while (adr < eadr)                      // check input value for each row (UNSIGNED)
    {
     val = g_val(adr,0, (b->fend & 31));      // next input value as UNSIGNED
     if (val & 0xff)
        { //bottom byte set
         endval = 0;
         break;
        }
     if (val != lastval) vmatch = 1;

     if (val > 1000) endval = 0;         // > 16 in top byte, so not a scale

      adr += rowsize;
      lastval = val;
    }

  // all lower bytes are zero, values have changed and <= 16
  // so assume it can be reduced
  if (endval && vmatch) add_mcalc(b, 4, 256);          // = (x/256) (upper byte)

 }

}

*/

















void scan_gaps(void)  //  (uint (*gs (SBK* s, SBK *n))   uint (*prt) (uint,cchar *, ...) ) //  ABCDEF   call subr from here ?? loop can be reused...
{
   // base detect first - combined cmd chain.

  LBK *s, *nxt;
  DTD *d;
  uint ix, jx,  start, end;      //, ans; //i, xend,


  #ifdef XDBGX
    DBGPRT(1,0);
    DBGPRT(2, "Gap analysis");
  #endif

//find_func(0x22060,0x2ffff);            //experiment




  for (ix = 0; ix < (chcmd.num-1); ix++)
   {
     s = (LBK*) chcmd.ptrs[ix];
     if (nobank(s->end) <= 0x201e) continue;

     jx = ix;
     while (jx < chcmd.num)
      {        //skip bytes,words etc
       nxt = (LBK*) chcmd.ptrs[++jx]; // next scan block
       if (nxt->fcom > C_TEXT) break;

      }

    if (g_bank(s->start) != g_bank(nxt->start)) continue;   //need bank check !!

    if (s->fcom <= C_TEXT) start = s->start;
    else start = s->end+1;
    end  = nxt->start-1;

    if (end < start) continue;

    #ifdef XDBGX

    // DIRS *get_cmd_def(uint cmd)
      DBGPRT(0,"\n\nGAP %x-%x", start, end);
      DBGPRT(1, "  %s <-> %s", get_cmd_str(s->fcom), get_cmd_str(nxt->fcom));
    #endif

    // simplest check first. Is it fill ? Wrong ....

    d = 0;              //get first data item
    get_dtd(start);
    jx = chdtd.lastix;
    if (jx < chdtd.num) d = (DTD *) chdtd.ptrs[jx];

    #ifdef XDBGX
     if (d) DBGPRT(1,"Nearest data at %x", d->stdat);
    #endif

    find_func(start, end);


/*
  if (n->fcom > C_BYTE)  // do as separate pass - isn't right here
        {  // next cmd bigger than byte, not tables, use up to first data item.
         uint flag;
         i = start;
         if (d && d->stdat < end) xend = d->stdat-1; else xend = end;
         flag = 0;

         while (i <= xend)
          {
           flag = 1;
           jx = g_byte(i);
           if (jx != 0xff)
            {
             flag = 0;
             break;
            }
           i++;
          }

         if (flag)
           {
            add_cmd(start, xend,0,0);       // fill
            if (!chcmd.lasterr) ans = 1;   // new command OK
           }
        }
*/


    //get nearest next data item from start, and other imds


     // code gap, do a scan.  check no data in here ??
     // this is actually too restrictive, as code may exist next to data.
/*
    if (!ans && s->fcom == C_CODE && n->fcom == C_CODE)
      {
       ans = chk_code_gap(s, n, d);
       if (ans)
        {
         // code scan may change chain, so must reset position for next gap.
         get_cmd(start,0);
         if (chcmd.lastix < chcmd.num) ix = chcmd.lastix-1;    // -1 would be nice, but loops
        }
      }

    if (!ans)
    {


//check for no data here and do a code scan..................
  //  if (d  && d->stdat >= start && d->stdat < end)
  //      {
          //  ?/?do data strut
          ans = discover_struct(start,end);
  //      }

//if !ans, try next immediate.....or do this in discover.........


       if (ans && chcmd.lastix < chcmd.num) ix = chcmd.lastix-1;    // rescan after change ?? -1 would be nice, but loops


    }

*/

  }      //end of cmd chain loop


  #ifdef XDBGX
   DBGPRT(2,0);
  #endif

}

//LOOP lp;



void turn_dtk_into_data(void)
{

  uint ix,  dcmd, start;   //,lstart;    //, end;cix, dix,
  uint push;
 // TRK *d, *n;
  DTD *d;
//  LBK *k;         //, *p;
 // JMP* j, *m;

#ifdef XDBGX
  DBGPRT(2,0);
  DBGPRT(1,"dtk to data");
#endif

//start with all loops must be spotted via autoinc

/* for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (DTK*) chdtk.ptrs[ix];
    if (d->opcsub == 1 )
     {
      lstart = 1;
       while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {
              p = get_cmd(start,0);
              DBGPRT(0,"%x IMD = %x ", d->ofst,a->stdat);
 if (p) DBGPRT(1," IGNORED %x-%x %s",p->start, p->end,cmds[p->fcom]); else DBGPRT(1,0);
//get cmd first.................

          //    DBGPRT(1,"%x-%x %s",p->start, p->end,cmds[p->fcom]);
            //  else DBGPRT(1, "***");
             }
         }
      //  DBGPRT(1,0);
      }
   }

/pushes first ??
// these are ALL INCORRECT except for 79f9 on A9L

 for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (DTK*) chdtk.ptrs[ix];
  //  dcmd = bytes(d->fend);       //may add an fcom instead...

    if (d->psh)       // && d->opcsub == 1)
       { //imd push
    //    DBGPRT(0, "%x is push ", d->ofst);
        lstart = 1;

        while ((a = get_dtkd(&chdtka, d, lstart)))
         {
          lstart = a->stdat+1;          //next start
          start = a->stdat;
          if (val_rom_addr(start))
             {

              get_cmd(start,0);
              cix = chcmd.lastix;
              while (cix < chcmd.num && start)
                {
                  p = (LBK*) chcmd.ptrs[cix--];
                  if (p->end < start) break;             //no overlap
                  if (p->start <= start && p->end >= start) start = 0;
                }
              if (start) DBGPRT(1,"push %x from %x",start, d->ofst);
             }
//get cmd to make sure it's not used.
         }
   //     DBGPRT(1,0);
       }
   }
  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);
  //scan dtk chain and add data entries
        DBGPRT(2,0);



  for (ix = 0; ix < chdtk.num; ix++)
   {
    d = (TRK*) chdtk.ptrs[ix];
    dcmd = bytes(opctbl[d->opcix].fend1);       //may add an fcom instead...
    if (opctbl[d->opcix].sigix == 14) push = 1; else push = 0;

    // find attached data start(s)

    lstart = 1;

#ifdef XDBGX
    if (d->ainc) DBGPRT(1, "%x is INC++ sz %d", d->ofst, d->ainc);
    #endif

    while ((a = get_dtkd(&chdtko, d->ofst, lstart)))
       {
        lstart = a->stdat+1;          //next start
        start = a->stdat;


  //      add_cmd(start, start+bytes(d->fend)-1,bytes(d->fend), d->ofst);       // add data entry TEMP

#ifdef XDBGX
        DBGPRT(1,"%x dat=%x opc %d",d->ofst, a->stdat, d->opcsub);
#endif
        // get nearest command, for overlap checks with data

      //  end = start + bytes(d->fend)-1;      //end of this data item
        p = get_cmd(start,0);

            if (p && p->start <= start && p->end >= start)
             {
               //overlap - kill if matches criteria

               if (p->fcom == dcmd)
                {
#ifdef XDBGX
                 DBGPRT(1,"duplicate command %x %s", start, cmds[dcmd]);
#endif
                 start = 0;                         // kill insert
                 break;
                }
             // words/bytes/longs within func/tab (+ struct/timer for now). args ?? - different chain)
               if ( dcmd >= C_BYTE && dcmd <= C_LONG && p->fcom >= C_TABLE && p->fcom <= C_TIMR)
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap struct %x %s", start, cmds[dcmd]);
#endif
                 start = 0;
           //      a->olp = 1;
                 break;
                }
               else
                {
#ifdef XDBGX
                 DBGPRT(1,"overlap %x %s from %x with %x-%x %s", a->stdat, cmds[dcmd], d->ofst, p->start, p->end,cmds[p->fcom]);
#endif
                 if (p->fcom == C_CODE && !push)         //push
                   {
#ifdef XDBGX
                     DBGPRT(1, " **** CODE ***");
#endif
//a->inv = 1;                  // temp..........
if (p->end-start < 16 )  //&& start > 0x9201f)
      {
#ifdef XDBGX
        DBGPRT(1,"inx?");
#endif
       }

                   }
                //and extra work to go here..............
                 break;
                }
             }
     //     }          //  end overlap check



// if imd, then look for a To at d->ofst+d->ocnt, for a loop with start at...
// AA has start and END, to be careful of...check by using inc....



        if (start && d->ainc)
          {
            // inc is set, look for a loop. may be better after an imd ??? will be before the inc ??
            j = 0;
            cix = get_jump(&chjpf,d->ofst);       // from jump (after inc opcode)
            while (cix < chjpf.num)
             {
               m = (JMP*) chjpf.ptrs[cix];
               if (m->toaddr > d->ofst) break;      // must span d->ofst
               if (m->toaddr <= d->ofst && m->toaddr != m->fromaddr &&m->fromaddr >= d->ofst && m->back) j = m;
               cix++;



    // next jump ? prev jump?
          //          if (dix < chjpt.num) j = (JMP*) chjpt.ptrs[dix]; else break;

             }

           //    j = (JMP*) chjpt.ptrs[cix];   // candidate jump


               if (j && j->back)          // && j->fromaddr > d->ofst)
                 {  //loop spans this increment.
                    //new subr ??
                    //need to check loop size too, A9L JUMPS to SUBRS....
#ifdef XDBGX
                   DBGPRT(1,"Loop? %x (%x-%x)", d->ofst, j->toaddr,j->fromaddr);
#endif
                  // set up loop holder
                   memset(&lp,0,sizeof(LOOP));              //clear loop struct
                   lp.increg = d->rgvl[1];
                   lp.incr = d->ainc;
                   lp.start = j->toaddr;                  // where loop starts
                   lp.end = j->fromaddr;

                   dix = ix;

while (dix && dix < chdtk.num)
 {
  DTD *b;
   n = (TRK*) chdtk.ptrs[dix--];

   if (n->ofst < lp.start-32) break;
   if (n->opcsub == 1 && n->rgvl[2] == lp.increg)
     {
       b = get_dtkd(&chdtko, n->ofst, 1);
     if (b) {  lp.datastart = b->stdat;
#ifdef XDBGX
       DBGPRT(1,"loop imd %x = %x", n->ofst, b->stdat);
#endif
       break;}
     }
  }



}

}
go back to find start address of designated register..............
// so this must be FROM ordered..............



// can use ix here for further loop
  //     n = (DTK*) chdtk.ptrs[ix];






       while (d->ofst < l.end && ix < chdtk.num)
          {     //inside loop
            ix++;
            n = d;
            d = (DTK*) chdtk.ptrs[ix];
             if (d->ainc && d->ofst != n->ofst && d->rreg == n->rreg) l.incr += d->ainc;
             else
               {
                 if (!l.incr2) l.incr2 = d->start[0]-n->start[0];
               //  else if (l.incr2 != d->start[0]-n->start) DBGPRT(1,"inc mismatch");
                }
      //       DBGPRT(1,"L %x", d->from);
          }

            DBGPRT(1,"loop analyse %x-%x for R%x inc %x %x start %x",l.start,l.end, l.increg, l.incr, l.incr2, l.datastart);
     //  ix--;   // to stay at end of loop

// and HERE is where to find start, end etc conditions ??
// inc should be FIRST in loop.............
// if jump is STAT then need to find exit condition
        }
     }


*/
// else DBGPRT(1,"no p");

//TEMP FIXUP for data

 for (ix = 0; ix < chdtd.num; ix++)
   {
    d = (DTD*) chdtd.ptrs[ix];

    start = d->stdat;
    push  = d->psh;

    //    dcmd = d->bsze;        // but this is now a BITMAP
    dcmd = 1;
    if (d->bsze & 2) dcmd = 2;
    if (d->bsze & 4) dcmd = 4;




    if (val_rom_addr(start) && !push && d->modes > 1)
       {
          // valid rom addr, ignore reg to reg and imds
          // split out add_cmd for ix value fo range checks ? or just do bfindix ??

          //BUT MANY OF THESE ARE DODGY !!


        //k =
        add_cmd(start, start+dcmd-1,dcmd, 0);      //d->ofst);       // add data entry

       }



}       // end of subr

}




void turn_scans_into_code (void)
 {

  // turn scan list into code commands.

  uint ix;
  SBK *s;

  #ifdef XDBGX
  DBGPRT(1," scanblks -> code n=%d", chscan.num);
  #endif

  for (ix = 0; ix < chscan.num; ix++)
   {
    s = (SBK*) chscan.ptrs[ix];

    #ifdef XDBGX
      DBGPRT(1,"[%d] scan %x - %x", ix, s->start, s->nextaddr);
    #endif

    if (s->stop && s->proctype == 1)      // && !s->ignore)                        // && !ignore ???
      {   // valid block to turn to code, not inv
       add_cmd (s->start,s->nextaddr,C_CODE,0);
      }
   }            // for loop
 }


// END of file
