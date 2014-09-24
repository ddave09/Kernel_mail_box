Kernel-mailbox-system-calls
===========================

New system calls for asynchronous communication among system processes. <br/>

Here I assume that you have linux-3.11.1 running. if you have different version please change your directory name from
 linux-3.11.1 to whatever is appropriate to you while following below steps. <br/>

how to successfully compile and run? <br/>
1.) Add system calls declaration in linux-3.11.1/include/linux/syscalls.h or just copy below lines in to the file <br/>
    asmlinkage long sys_mkMbox421(unsigned long mbxID); <br/>
    asmlinkage long sys_rmMbox421(unsigned long mbxID); <br/>
    asmlinkage long sys_countMbox421(void); <br/>
    asmlinkage long sys_listMbox421(unsigned long *mbxList, unsigned long K); <br/>
    asmlinkage long sys_sendMsg421(unsigned long mbxID,  char *msg, unsigned long N); <br/>
    asmlinkage long sys_receiveMsg421(unsigned long mbxID, char *msg, unsigned long N, unsigned char flag); <br/>
    asmlinkage long sys_countMsg421(unsigned long mbxID); <br/>
    
2.) Add system calls in system calls table. It depends on your machine's archtecture. <br/>
    [Add accordingly](http://lxr.free-electrons.com/source/arch/x86/syscalls/syscall_64.tbl)
    e.g  path : linux-3.11.1/arch/x86/syscalls/syscall_64.tbl <br/>
3.) Move mbox.h header file in linux-3.11.1/include/linux or figure out how to add dependencies in Makefile <br/>
4.) Have your files in subfolder under linux-3.11.1 folder. <br/>
5.) Add your sub-folder on line "core-y := /usr" in main make file for linux source. e.g My make file and source files  for this project are  contained in kernel_mail_box folder under linux-3.11.1, so for me it is 
"core-y := /usr /kernel_mail_box" <br/>
6.) Enter into your extracted kernel directory from terminal. Follow below steps <br/>
    1.) dave@ubuntu:~$ cd Documents/linux-3.11.1 <br/>
    2.) dave@ubuntu:~/Documents/linux-3.11.1$ make <br/>
    3.) dave@ubuntu:~/Documents/linux-3.11.1$ make install <br/>

make will take a while so be patient. if you really want to expedite your compilation, Compiler cache (ccache) is a very nice technology for that. The below link is an excellent tutorial from intel. <br/>
[cache](https://software.intel.com/en-us/articles/accelerating-compilation-part-1-ccache) <br/>
