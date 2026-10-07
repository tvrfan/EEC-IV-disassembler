# EEC-IV-disassembler 

Semi Automatic Disassembler for Ford EEC-IV and V binaries

Split into separate subdirectories.

Latest Stable Version          4.13
Latest development Version     6.01


--- Running SAD ---

NOTES - 

1) This app is intended to help understand how EEC code works, not as a tuning or commercial tool.

2) Please be aware that user commands (in _dir.txt file) are always treated as 'master'. In some cases, these can actually break the processing.
Therefore - if something doesn't look right, please try running SAD WITHOUT any directive file, or make a _dir version only with SYMBOL commands in it.
This extra check may help show a user command error, and/or new information in case of a SAD bug.

-------------------------------------------------

Docs subdirectory

SAD_user_manual.pdf	            disassembler documentation and user manual.

SAD_commands_definition.pdf		complete command definition, and comments file definition.

SADWIN.pdf	                    Windows GUI Wrapper documentation.
 
Version.txt	Short description of bugs fixed and changes made for each version.

-------------------------------------------------

Win32 Subdirectory

SADvvv.exe   Windows executable      (32 bit build) 

chip.ico     icon file

---------------------------------------

Win32/SADwin Subdirectory

SADWIN.exe   Windows GUI 'Wrapper'

SADwin.cpp   Source code        (uses WIN32 API)

chip.ico     icon file          (same as Windows above)

Notes -  SADwin will create a default config file (sad.ini) for you on its first run,
         which you can then setup to your preference via SADWIN.

---------------------------------------

Linux64 Subdirectory 

SADvvv     Linux executable         (64 bit build).

After download, make SAD files executable if necessary with chmod.

sad.ini    an example file to show path locations, not auto generated. (no SADWIN equiv yet)

---------------------------------------

Development Subdirectory

Similar dir tree to main, with latest development builds.  Probably stable, but no guarantees.

---------------------------------------


-- Config Notes

config file (of directory locations) is sad.ini (both Windows and Linux)

Plain text.  Edit sad.ini as required for your setup.

If sad.ini is not in same directory as SAD executable then use command  SAD -c 'path' where path is location of sad.ini.

If no sad.ini present, then everything asssumed to be in same directory as SAD.

Drag and drop (Windows) assumes everything in same directory as dropped binfile.

----------------------------------

Source Subdirectory

Contains sources and headers to build SAD.  Last few stable versions are available.

----------------------------------

Build Notes

I use Codelite IDE as my build/test environment, on Linux Mint and a Windows VM under Linux (Virtualbox). 
  (Codelite uses gcc in Linux and MingW32 in Windows)

Any common compiler, IDE, linker, etc. should work, there is no OS specific code except for SADWin whihch uses Win32 API for graphics.
I have not sorted out a makefile, but it's a straight compile and link of the .cpp and .h files.

Separate Subdirectory SADWin contains the source for the Win32API graphic interface for SAD (binfile select,
viewing various files and config)

