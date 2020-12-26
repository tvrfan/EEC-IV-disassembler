
#include <signal.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>                // linux
#include "shared.h"

#ifdef __ANDROID__
#include "android/log.h"
#endif

int    get_config(char **);
void   shutdwn   (void);
void   startup   (void);
short  disassemble    (char *);
char   *get_filemap (void);

void closefiles(void);

void DBG_data();
void prt_dirs(void);


extern HDATA fldata;

#ifdef __linux__
struct sigaction sa;                // for signal capture
#endif

char fstr[256];

short incpg = 0;
uint lastsym = 0;
int lastpass = -1;

const char *passd [] = {"Checking Binary ", "Process Commands", "Analyse Bin [1] " ,
                 "Analyse Bin [2] ","Analyse Bin [3] ", "Output Listing " };



char prg[] = { '|', '/', '-', '\\' };       //  for rotating symbol display 

void show_prog (int s)
{
  if (s < 0 || s > 5) s = 4;               // generic safety

  if (s && s != lastpass)
    {
      printf ("\n  %c  %s\r", prg[0], passd[s]);
      lastpass = s;
      incpg = 0;
      lastsym = 0;
    }
  incpg++;

  if (incpg > 50 )           // not if redirected to a file.....
    {
      incpg = 0;
      if (lastsym >= sizeof (prg)) lastsym = 0;
      printf ("  %c\r", prg[lastsym]);
      fflush (stdout);
      lastsym++;
    }
}



void prt_stars(void)
{
 short i;
  printf ("\n");
  for (i = 0; i < 50; i++) printf ("*");
  printf ("\n");
}


/* change to allow cmd line options
-c  config file location
-h  help info
-o  options ?
then <bin> file

*/

#ifdef __linux__
void segfault(int signal, siginfo_t *si, void *arg)
{
    printf("** Caught CRASH signal at address %p code %x\n", si->si_addr, si->si_code);
    printf("** Abandon disassembly\n\n");

    fprintf (fldata.fl[2], "\n\n ** Caught signal %d ** \n", signal); // same as wrnprt
    fprintf (fldata.fl[2],"** Abandon disassembly\n\n");

    fprintf (fldata.fl[6], "\n\n ** Caught signal %d ** \n", signal); // same as wrnprt
    fprintf (fldata.fl[6],"** Abandon disassembly\n\n");



 #ifdef XDBGX   
    if (signal == SIGINT) DBG_data();            // this seems to work, even if not technically safe...
 #endif

    prt_dirs();
    closefiles();

    exit(0);
}
#endif



int main (int argc, const char **argv)
{
  short go;
  char *x, *fn;
  char* pth[3];

#ifdef __linux__
  // set up signal catcher for disasters

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
#endif




  prt_stars();
  printf ("EEC-IV disassembler Version %s (%s)", SADVERSION, SADDATE);
  printf ("\nAbsolutely no warranty or liability");
  prt_stars();
 
  fn = 0;

  pth[0] = (char *) argv[0];
  pth[1] = 0;
  pth[2] = 0; 
  for (go = 1; go < argc; go++)
    {
     if (*argv[go] == '-')
     {
       x = (char *)(argv[go]+1);

       switch (*x)
        {
         default:
           printf("unknown option %s\n", argv[go]);
           return 0;

         case 'c' :
         case 'C' :
          pth[1] = (char *) argv[go+1];              // path of config file
          go++;
          break;
          
         case 'h':
         case 'H':
           printf ("\nsad -h   this help info");
           printf ("\nsad  <-c configdir> <-w configdir> <bin_filename>");
           printf ("\n   options");
           printf ("\n      -c for directory of sad.ini config file to read");
           printf ("\n      if no -c then looks for a sad.ini in same dir as sad exe");
           printf ("\n      asks for bin_filename if none specified\n");
          return 0;
        }

     }
     else fn = (char*) argv[go];
   }

 // printf("FN=%s\n",fn);
  // get_args here

  if (get_config(pth))   return 1;

/*******************************/


    if (fn)
       {
        printf ("\nFor binary file '%s'\n", fn);
        go = disassemble(fn);
        
        return go;
       }


     go = 1;
     while (go)
      {                                     //   fstat / stat    to check file
       printf ("\nEnter binary file name (full path allowed)\n");
       fgets (fstr, 64, stdin);
       if (!strcmp (fstr, "\n")) return 10;
       go = disassemble(fstr);                     // = 0 if successful
       fflush(stdout);
      }

 return go;
}

