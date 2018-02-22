# EEC-IV-disassembler
semi automatic disassembler for Ford EEC-IV and V binaries - latest is 3.03

SAD.pdf    disassembler documentation  (a little out of date, but still 96% correct)

SADWIN.pdf  Wrapper documentation

Windows and Linux versions now posted in Master branch.

-------------------------------------------------

WINDOWS with GUI interface 'Wrapper'

SAD.exe     version 3.03 for windows   (32 bit build)

SADWIN.exe  Windows GUI 'Wrapper'   (32 bit build)

chip.ico   icon file

! IMPORTANT !  do *NOT* use sad.ini in Windows, SADWIN will create one.  The sad.ini here is set for Linux and has wrong path structure for Windows.

---------------------------------------

FOR LINUX ONLY 

SADX   linux amd64 build.    After download, make this file executable if necessary with chmod

sad.ini    - an example file to show path orders, not auto generated. (no SADWIN equiv yet)

Edit sad.ini for your setup. If sad.ini is not in same directory as SADX  then run command

SADX -c 'path'   where path is location of sad.ini.

If no SAD.ini present, then everything must be in same directory as SADX

----------------------------------

NOTE - this code is intended to help understand only how each EEC works, not as a tuning or commercial tool. 
