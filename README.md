# EEC-IV-disassembler

Semi Automatic Disassembler for Ford EEC-IV and V binaries

NOTE - this code is intended to help understand only how each EEC works, not as a tuning or commercial tool.

Split into separate subdirectories.

Latest Version is 4.0.4

-------------------------------------------------

Docs subdirectory

SAD.pdf    disassembler documentation (NB. not updated for version 4 yet)

SADWIN.pdf  Windows Wrapper documentation

Version.txt  Short description of bugs fixed and changes made

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

Contains sources and headers to build SAD
I have used CodeBlocks and Codelite as compilers/IDE, should be able to use any common compiler/linker
Not sorted out a makefile, it's a straight compile of the 3 .cpp files 

separate Subdir SADWin containing source, for the simple Win32API graphic interface for SAD (binfile select,
viwing various files and config.


 

