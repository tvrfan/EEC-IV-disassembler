# EEC-IV-disassembler
semi automatic disassembler for Ford EEC-IV and V binaries - version 3.06

NOTE - this code is intended to help understand only how each EEC works, not as a tuning or commercial tool. 


Split into separate subdirectories.

Have released a 3.07 for user function requests, but kept 3.06 in the directories as well .
3.07 is not fully tested, but seems to work Ok so far, so have released it for user evaluation/testing

-------------------------------------------------

Docs subdirectory

SAD.pdf    disassembler documentation (inc 3.07)

SADWIN.pdf  Windows Wrapper documentation

Version.txt  Short description of bugs fixed and changes made

-------------------------------------------------

Windows Subdirectory

SAD.exe     version 3.06 and 3.07 for windows   (32 bit build)

SADWIN.exe  Windows GUI 'Wrapper'   (32 bit build)

chip.ico   icon file


Notes -  SADwin will create a default config file for you, which you can then setup to your preference

---------------------------------------

Linux Subdirectory 

SAD3_06 and SAD3_07  linux amd64 build.    After download, make this file executable if necessary with chmod.
sad.ini    an example file to show path locations, not auto generated. (no SADWIN equiv yet)

Notes

Edit sad.ini for your setup.

If sad.ini is not in same directory as SADx  then run command  SADx -c 'path'   where path is location of sad.ini.

If no sad.ini present, then everything must be in same directory as SADx

----------------------------------

Source Subdirectory

Contains sources and headers to build SAD
I have used CodeBlocks and Codelite as compilers/IDE, should be able to use any common compiler/linker

