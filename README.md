# EEC-IV-disassembler 

Semi Automatic Disassembler for Ford EEC-IV and V binaries

NOTE - this code is intended to help understand only how each EEC works, not as a tuning or commercial tool.

Split into separate subdirectories.

Latest Stable Version      is 4.0.6

-------------------------------------------------

Docs subdirectory

SAD_user_manual_to_4.0.6.pdf		disassembler documentation and user manual BEFORE 4.0.6.

SAD_user_manual_4.0.6_onwards.pdf	disassembler documentation and user manual.

SAD_commands_definition.pdf		complete command definition, and comments file definition.

SADWIN.pdf	Windows Wrapper documentation.

Version.txt	Short description of bugs fixed and changes made for each version.

-------------------------------------------------

Windows Subdirectory

SADvvv.exe   Windows executable      (32 bit build. 'vvv' is version number) 

chip.ico     icon file

---------------------------------------

Windows/SADwin Subdirectory

SADWIN.exe   Windows GUI 'Wrapper'   (32 bit build)

SADwin.cpp   Source code             (WIN32 API)

chip.ico     icon file               (same as Windows above)

Notes -  SADwin will create a default config file for you on first run,
         which you can then setup to your preference

---------------------------------------

Linux Subdirectory 

SADvvv     Linux executable         (amd64 build. 'vvv' is version number).

After download, make SAD files executable if necessary with chmod.

sad.ini    an example file to show path locations, not auto generated. (no SADWIN equiv yet)

-- Notes --

Edit sad.ini for your setup.

If sad.ini is not in same directory as SADvvv  then use command  SADvvv -c 'path'   where path is location of sad.ini.

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


