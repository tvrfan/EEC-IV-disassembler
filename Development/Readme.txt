
Development versions information.


The development subdir contains the latest test versions.  These should be stable to use, but probably have minor bugs.

<version>.exe are Windows builds (32 bit, built with CodeLite/gnu) and <version>.amd64 are linux builds (64 bit, built with Codelite/gcc).


Dev_Versions.txt lists changes between these dev versions here only.   Stable release info in /Docs/Versions.txt 

Generic address rules from version 4.0.7

1. Output 
 In single banks, no bank numbers are shown anywhere. all addresses are 16 bits (4 digits)

 In multibanks, addresses below 0x400 are shown with no bank number - they are registers internal to CPU. 
   All other addresses have a bank (i.e. 5 digits). This might look strange in some cases, but is technically correct,
   as any address 0x400 and above must be external to CPU and therefore MUST have a bank.

 where SAD can verify a value must be an address, it will add the relevant bank automatically (typically bank 1)

2. Input (in commands).

 In single banks, no bank number is required.  

 In multibanks, any address 0x400 and over MUST HAVE A BANK (i.e. 5 digits).  Valid banks are 0,1,8,9
   To maintian compatibilty with other tools, addresses below 0x2000 can be input in SAD commands without a bank, 
   and they will automatically be assigned to Bank 1 

3. Running SAD

Please be aware that your commands (in .dir file) can actually break the processing, because user commands are always taken as the 'master'.
So if something doesn't look quite right, please try running SAD without a directive file, or use a version only with SYMbol commands in it.
This extra check may help show a user command error, and/or new information in case of a bug.  



Notes on regression errors.

Whilst I am sure there are regression bugs, please be aware that SAD 'final spec' is not fully defined, and some things have been swopped around deliberately as new info arrives.
BIG changes for version 5 with new command syntax and capabilities. 

Formats (and names) may change slightly as new things get discovered about the bins and CPU details.  This is still an ongoing process, exspecially for the 'specialist chips' which
appear on later boards.


Ongoing Issues 

1. Subroutine Arguments (and related symbol names)

 Until version 4, SAD could not handle variable arguments at all (that is where a subroutine gets variable numbers of bytes according to parameters).

 From Version 4, SAD now gets the right number of bytes, but it still cannot always determine the true size of each argument (ie bytes versus words).
 This leads to the question about symbol names.  Current rule is that arguments determined to be WORDS ARE checked for symbol names, bytes are NOT.
 Is this always right ??  I honestly don't know.   You can specify argument layout with the ARGS command, and what gets a symbol name. 

 Version 5 has a new tracking method which will likely achieve better match for sizing, but may still not work perfectly.

2. Symbol names may appear and disappear in places across versions. Questions around "when should symbols be automatically printed ?"  No, it's NOT always obvious.

3. Conditional jumps 
Because SAD cannot always resolve which opcode set the PSW for each conditional jump, it will sometimes show "if (0 = 0)" style.
The command 'pswset' has been added to specify where the 'source' for the conditional jump is.  Probably more work to do here.


4. Data structures not being found.

Where an address is referenced directly, then data is always found.   Same for an indexed [baseE+offset] style, used extensively in later bins.  
Where a 'list' of structures are referenced by [base + Register], SAD will typically only get the first one at base-addr.
This is because there is no easy way to know the possible value ranges of the register.
Loops which access lists of items are similar, only the first few will be identified.
Vector (pointer) lists do work as they are verified in a different way. Some code is missed entirely, an example being where 
a register is assigned an address which is pushed onto the stack and used later on.

5. Conditional jumps and 'psuedo code' - there is still some debate about the best way to represent some jumps in sensible way - JC/JNC, JV/JNV.  So these are likely to change.

6. Print Layout options may still have some quirks to be ironed out...and more features requested have not been done yet

