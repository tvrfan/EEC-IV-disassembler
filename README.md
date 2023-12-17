# EEC-IV-disassembler 

Semi Automatic Disassembler for Ford EEC-IV and V binaries

NOTE - this code is intended to help understand only how each EEC works, not as a tuning or commercial tool.

Split into separate subdirectories.

Latest Stable Version      is 4.0.7

-------------------------------------------------

Docs subdirectory

SAD_user_manual_4.0.6_onwards.pdf	disassembler documentation and user manual.

SAD_commands_definition.pdf		complete command definition, and comments file definition.

SADWIN.pdf	Windows Wrapper documentation.

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

Notes -  SADwin will create a default config file for you on first run,
         which you can then setup to your preference

---------------------------------------

Linux64 Subdirectory 

SADvvv     Linux executable         (64 bit build).

After download, make SAD files executable if necessary with chmod.

sad.ini    an example file to show path locations, not auto generated. (no SADWIN equiv yet)

-- Notes --

Edit sad.ini for your setup.

If sad.ini is not in same directory as SADvvv  then use command  SAD -c 'path'   where path is location of sad.ini.

If no sad.ini present, then everything must be in same directory as SAD

----------------------------------

Source Subdirectory

Contains sources and headers to build SAD.  Last few versions are available.
I have used CodeBlocks and Codelite as compilers/IDE, should be able to use any common 'C' compiler/linker.
Not sorted out a makefile, it's a straight compile and link of the .cpp files. 

Separate Subdirectory SADWin containing the source for the Win32API graphic interface for SAD (binfile select,
viewing various files and config)

----------------------------------

Development Subdirectory
 
early releases of SAD versions for testing - probably stable, but not guaranteed.

Version.txt	Short description of bugs fixed and changes made for each version

NEW VERSION '5 ALPHA' RELEASED. READ note first before using. 


