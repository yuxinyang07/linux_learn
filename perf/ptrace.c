#include<stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc,char** argv)
{
    pid_t pid;
    struct user_regs_struct reg;
    pid = fork();
    if(pid ==0){
        printf("child pid \n");
        ptrace(PTRACE_TRACEME,0,NULL,NULL);
        printf("just test \n");
        execl("bin/ls","ls",NULL);
    }else{
        wait(NULL);
        ptrace(PTRACE_GETREGS,pid,NULL,reg);
        printf("Register: rdi[%ld],rsi[%ld] , rdx[%ld], rax[%ld],   \
                 orig[%ld]\n",reg.rdi,reg.rsi,reg.rdx,reg.rax,reg.orig_rax);

        ptrace(PTRACE_CONT,pid,NULL,NULL);
        sleep(1);

    }


    
    return 0;
}
