
/***********************************************************************
GUI Wrapper for SAD console disassembler  WIN32 API   Version 0.1
***********************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <shlobj.h>
#include  <stdlib.h>

// event defines

#define DS_EXIT 1001
#define DS_DISS 1002
#define DS_OPNF 1004

#define DS_OUTP 1005
#define DS_MSGS 1006
#define DS_DIRF 1007
#define DS_CMNT 1008
#define DS_CNFX 1009

#define DS_HELP 1010
#define DS_BUTT 1011
#define DS_TSTX 1012

#define DS_TIM1 1040

#define NUMPATHS  6



HMENU hMenu, hFileMenu, hToolsMenu, hViewMenu, hHelpMenu;
OPENFILENAME ofn;

const char g_szClassName[] = "myWC";


char gstr [512];                  // generic for string assembly
char tstr [512];                  // for timer generic
char fname [NUMPATHS][512];       // full filenames
char basepath[512];               // where this prog is (minus names)
char sadpath[512];                 // full path and name of sad.ini
char basename[64];                  // binary name by itself
char wtitle[40] = "EEC Disassembler         ";

struct xx
{
 const char *sfx;           //file suffix after name
 const char *fcmt;             // and comment associated
} fs [NUMPATHS] = {

{".bin"    , "Folder Location for Binaries (xx.bin)"  },
{"_lst.txt", "Folder Location for Listings (xx_lst.txt)" },
{"_msg.txt", "Folder Location for Warnings (xx_msg.txt)" },
{"_dir.txt", "Folder Location for Directives (xx_dir.txt)" },
{"_cmt.txt", "Folder Location for Comments (xx_cmt.txt)" },
{""        , " Full pathname of SAD.exe (disassembler)" }
};


void put_config(HWND hwnd)
{
 // put config if sad.ini is null

  int i;
  char *t;
  FILE *fh;

  fh = fopen(sadpath, "wt");    //open for write
  if (!fh)
   {
    MessageBox(hwnd, "Can't create sad.ini", NULL, MB_OK);                       // error opening file
    return;
   }

  for (i =0; i < NUMPATHS; i++)
   {
    if (i < NUMPATHS-1)
     {
      t = strrchr(fname[i], '\\');
      t++;
      if (*t)  *t = 0;           //safety - drop anything after path
     }
    fprintf (fh, "%s       #%s\n", fname[i],fs[i].fcmt);
   }

    fclose(fh);


}

void get_config(HWND hwnd)
 {
  short i;
  char *t;
  FILE *fh;

  fh = fopen(sadpath, "rt");    //open for read
  if (!fh)
   {  // should not happen if checked and/or created
    MessageBox(hwnd, "Can't open sad.ini", NULL, MB_OK);                       // error opening file
    return;
   }

  for (i = 0; i< NUMPATHS; i++)
     {
      if (fgets (gstr, 256, fh))          // read one line
        {
         t = strchr(gstr, ' ');        // space
         if (!t) t = strchr(gstr, '#');       // or comments
         if (!t) t = strchr(gstr, '\n');       // or newline
         if (t)
           {
            *t = '\0';
            t--;
            if (*t != '\\' && i < NUMPATHS-1) strcat(t+1, "\\"); // not for SAD.exe path
           }
         strcpy(fname[i],gstr);
        }
      }

     fclose(fh);                 //close leaves handle addr
 }


//-------------------------------------------------------------

void build_str(char *dst, int forb, const char* st, ...)
{
  va_list args;
  va_start (args, st);

  if (forb) dst += strlen(dst);
  if (st) vsprintf (dst,st, args);

  va_end (args);
}


int FileExists(char* file)
{
   WIN32_FIND_DATA FFdata;

   HANDLE handle = FindFirstFile(file, &FFdata) ;

   if (handle != INVALID_HANDLE_VALUE)
    {
     FindClose(handle);
     return 1;
    }
   return 0;
}

void bld_fname(char *x, int ix)
{
  build_str(x,0,fname[ix]);
  build_str(x,1,"%s%s", basename,fs[ix].sfx);
}


void do_menustates(void)
{
    int i;

    for (i =1; i < NUMPATHS-1; i++)
    {
       bld_fname(tstr,i);

       if (FileExists(tstr))
           EnableMenuItem(hMenu,i+1004,MF_BYCOMMAND|MF_ENABLED);
       else
           EnableMenuItem(hMenu,i+1004,MF_BYCOMMAND|MF_GRAYED);
    }

}

BOOL filopen(HWND hwnd, const char* filter, const char* title, const char* path)
{

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = gstr;
  ofn.lpstrFile[0] = '\0';
  ofn.hwndOwner = hwnd;
  ofn.nMaxFile = sizeof(gstr);
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1;
  ofn.lpstrInitialDir = path;               // set THIS to required path
  ofn.lpstrFileTitle = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

  return GetOpenFileName(&ofn);

}


void get_binfile(HWND hwnd)
{
  if(filopen(hwnd, "EEC Binary(*.bin)\0*.bin\0","Select EEC Binary",fname[0]))
    {
     gstr[ofn.nFileExtension-1] = '\0';             // clear extension and dot
     strcpy(basename,gstr+ofn.nFileOffset);
     EnableMenuItem(hMenu,DS_DISS,MF_BYCOMMAND|MF_ENABLED);
     do_menustates();

     sprintf(wtitle+25, "%12s", basename);          // Add bin name to title
     SetWindowText(hwnd,wtitle);
     SetTimer(hwnd,DS_TIM1,1000, NULL);             // update edit menus on timer
    }
}

void get_exefile(HWND hwnd)
{
  filopen(hwnd, "EXE File(*.exe)\0*.exe\0","Select EEC disassembler",fname[5]);
  strcpy(fname[5],gstr);
}


LPITEMIDLIST get_pidl(char *fn)
 {
  LPITEMIDLIST pidl;
  pidl = 0;
  if (fn)
    {
      //get the name of the folder and put it in path
      SHGetPathFromIDList ( pidl, fn);    // wrong way round
      //free memory alloced
      IMalloc * imalloc = 0;
      if ( SUCCEEDED( SHGetMalloc ( &imalloc )) )
        {
         imalloc->Free ( pidl );
         imalloc->Release ( );
        }
        return pidl;
    }
    return 0;
 }


void free_lpitem(LPITEMIDLIST pidl)
 {
   //free an alloced LPITEM...
  if (!pidl) return;

  IMalloc * imalloc = 0;
  if ( SUCCEEDED( SHGetMalloc ( &imalloc )) )
   {
    imalloc->Free ( pidl );
    imalloc->Release ( );
   }
 }

// callback func here.....
int CALLBACK BrowseSet( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  switch(uMsg)
  {
    case BFFM_INITIALIZED:
       SendMessage( hwnd, BFFM_SETSELECTION, TRUE, lpData );
       break;
  }
  return 0;
}


int get_dir(int ix)
 {
  BROWSEINFO bi = { 0, 0, 0, 0, 0, 0, 0, 0 };
  LPITEMIDLIST pidl;
  int ans;

  // build  initial dir string....
  build_str(gstr,0,fname[ix]);

  bi.lpszTitle  = fs[ix].fcmt;
  bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
  bi.lpfn       = BrowseSet;
  bi.lParam     = (LPARAM) gstr;

  *(fname[ix]) = '\0';           // safety
  pidl = SHBrowseForFolder (&bi);

  ans = 0;
  if ( pidl)
    {      //get the name of the folder and put it in path
      SHGetPathFromIDList ( pidl, fname[ix]);
      strcat(fname[ix], "\\");             // add trailing slash
      free_lpitem(pidl);
       ans = 1;
    }
   return ans;
 }



char *get_dirs(HWND hwnd)
{
 //BROWSEINFO bi = { 0, 0, 0, 0, 0, 0, 0, 0 };
 int ans, i;

 build_str(gstr,0,"This option selects the locations for the file types\n");
 build_str(gstr,1,"and for the disassembler program itself (SAD.exe)\n");
 build_str(gstr,1,"via windows select dialogs.\n\n");
 build_str(gstr,1,"You can also edit .INI file directly via Edit menu option.\n");

 ans = MessageBox(hwnd, gstr,"Setup sad.ini", MB_OKCANCEL);

 if (ans == IDCANCEL) { return 0;}

 for (i = 0; i < NUMPATHS-1; i++)
  {
   if (!get_dir(i) || *(fname[i]) == 0) break;  // fn[i] check too
  }

   get_exefile(hwnd);


     if (i >= NUMPATHS-1)
     {
      put_config(hwnd);
      MessageBox(hwnd, "DONE","Setup sad.ini", MB_OK);
     }
    else get_config(hwnd);      // restore state from file
    return 0;
}


void setupmenu (HWND hwnd)
  {
    HICON hIcon;

    hMenu = CreateMenu();        // top menu of tree

    hFileMenu = CreatePopupMenu();         // File
    AppendMenu(hFileMenu, MF_STRING, DS_OPNF, "&Select Bin");
    AppendMenu(hFileMenu, MF_STRING, DS_EXIT, "E&xit");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hFileMenu, "&File");

    hToolsMenu = CreatePopupMenu();
    AppendMenu(hToolsMenu, MF_STRING|MF_GRAYED, DS_DISS, "&Disassemble");
    AppendMenu(hToolsMenu, MF_STRING, DS_TSTX, "&Setup");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hToolsMenu, "&Tools");

    hViewMenu = CreatePopupMenu();
    AppendMenu(hViewMenu, MF_STRING|MF_GRAYED, DS_OUTP, "&List File");    //modifymenu to change it
    AppendMenu(hViewMenu, MF_STRING|MF_GRAYED, DS_MSGS, "&Msgs File");
    AppendMenu(hViewMenu, MF_STRING|MF_GRAYED, DS_DIRF, "&Dirs File");
    AppendMenu(hViewMenu, MF_STRING|MF_GRAYED, DS_CMNT, "&Cmts File");
    AppendMenu(hViewMenu, MF_STRING, DS_CNFX, "&Ini File");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hViewMenu, "&Edit");

    hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, DS_HELP, "&About");
    // add a sad -v here
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hHelpMenu, "&Help");

    SetMenu(hwnd, hMenu);

    hIcon = (HICON)LoadImage(NULL, "chip.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    if(hIcon)
      {
       SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
      }
    else
      {
       MessageBox(hwnd, "Could not load icon", "Error", MB_OK | MB_ICONERROR);
      }
 }


LRESULT CALLBACK MsgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  int ix;
  switch(Message)
   {
    case WM_CREATE:
         setupmenu(hwnd);
         break;

    case WM_COMMAND:
         ix = LOWORD(wParam)- 1004;
         switch(LOWORD(wParam))
          {
           case DS_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

           case DS_OPNF:
                get_binfile(hwnd);
                break;

           case DS_DISS:
                build_str(gstr,0,"-c %s  %s",basepath,basename);
                ShellExecute(NULL, "open", fname[5], gstr,NULL, SW_NORMAL);
                break;


           case DS_HELP:
                MessageBox(hwnd, "SAD GUI Wrapper\n Version 0.1", "SAD Wrapper", MB_OK);
                break;

           case DS_OUTP:
           case DS_DIRF:
           case DS_MSGS:
           case DS_CMNT:
                bld_fname(gstr,ix);
                ShellExecute(NULL, "open", gstr, NULL ,0, SW_NORMAL);
                break;

         case DS_CNFX:
                ShellExecute(NULL, "open", sadpath, NULL ,0, SW_NORMAL);
                break;



           case DS_TSTX:
                get_dirs(hwnd);
                break;

           default:
              sprintf(gstr,"%d",LOWORD(wParam));
              MessageBox(hwnd, "Something went wrong", gstr, MB_OK);

            break;
          }
         break;

    case WM_TIMER:
         do_menustates();
         break;                // OK this works !

    case WM_CLOSE:
         DestroyWindow(hwnd);
         break;
     case WM_DESTROY:
         PostQuitMessage(0);
         break;
     default:
         return DefWindowProc(hwnd, Message, wParam, lParam);
        }
        return 0;
}

// ****** start HERE *******

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
        WNDCLASSEX wc;
        HWND hwnd;
        MSG Msg;
        int ex;

        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = 0;
        wc.lpfnWndProc   = MsgProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = g_szClassName;
        wc.hIconSm               = NULL;

        if(!RegisterClassEx(&wc))
         {
           MessageBox(NULL, "Window Registration Failed!", "Error!",
                        MB_ICONEXCLAMATION | MB_OK);
           return 0;
         }

        hwnd = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                g_szClassName,
                wtitle,           //"EEC Disassembler",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
                NULL, NULL, hInstance, NULL);

        if(hwnd == NULL)
         {
          MessageBox(NULL, "Window Creation Failed!", "Error!",
                        MB_ICONEXCLAMATION | MB_OK);
          return 0;
         }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

// get working dir, and check whther sad.ini exists

   GetCurrentDirectory(MAX_PATH,basepath);    // seems OK

   build_str(gstr,0,basepath);                // copy basepath to fnpath
   build_str(gstr,1,"\\sad.ini");
   strcpy(sadpath,gstr);

   ex = FileExists(sadpath);

   if (!ex)
    {
     //populate all paths with default
     for (ex = 0; ex < NUMPATHS; ex++) sprintf(fname[ex], "%s\\", basepath);

     MessageBox(hwnd, "Can't find SAD.ini\nCreating a default one\n" , "Information", MB_OK);
     put_config(hwnd);
    }

  get_config(hwnd);

  basename[0] = 0;          // binary name

// now handle events.

    while(GetMessage(&Msg, NULL, 0, 0) > 0)
        {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
         }
        return Msg.wParam;
}
