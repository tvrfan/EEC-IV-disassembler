

#ifndef _XDBG_H
#define _XDBG_H 1




#define XDBG 1                    // debug enabled

#ifdef XDBG
  #define WNPRT  wnprt
  #define WNPRTN wnprtn
  #define PBK   pbk
  #define PBKN  pbkn
  #define PRT_CADT prt_cadt
#else
  #define WNPRT
  #define WNPRTN
  #define PBK
  #define PBKN
  #define PRT_CADT
# endif

#endif
