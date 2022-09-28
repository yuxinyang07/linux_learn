#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>


#define MY_PORT 8888
#define BACK_LOG 50
int main (int argc,char *argv[])
{
    int fd = 0;
    int serverfile;
    int clientfile;
    int  addrlen;
    int clientnum = 0;
    int revlen = 0;
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    unsigned char RecevBuf[1000];
    serverfile = socket(AF_INET,SOCK_STREAM,0);
    if(serverfile == -1){
        printf("socket creat failed \n");
        return -1;
    }
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MY_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(my_addr.sin_zero,0 ,8 );
    fd = bind(serverfile,(const struct sockaddr *)&my_addr,sizeof(struct sockaddr));
    if(fd < 0){
        printf("bind error \n");
        return -1;
    }
    if(listen(serverfile,BACK_LOG) <0){
        printf("listen failed \n");
        return -1;
    }
    addrlen = sizeof(struct sockaddr_in);
    while(1){
        
         clientfile = accept(serverfile,(struct sockaddr *)&client_addr,&addrlen);
         if(clientfile < 0){
            printf("accept error \n");
            return -1;
         }else{
          clientnum ++;
           printf(" get connect form clientnum%d %s \n",clientnum,inet_ntoa(client_addr.sin_addr));  
           if(!fork()){
            while(1){
           revlen = recv(clientfile,RecevBuf,999,0);
           if(revlen <= 0){
            close(clientfile);
            return -1; 
           }else{
            RecevBuf[revlen] = '0';
            printf(" recev buff = %s \n",RecevBuf);
           }
           }
            
        }
            
      }
      
    }
    
    
}
