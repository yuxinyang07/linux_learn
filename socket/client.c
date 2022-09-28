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

int main (int argc,char *argv[])
{
    int fd = 0;
    int serverfile;
    int clientfile;
    int  addrlen;
    int isendlen;
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    unsigned char SendBuf[1000];
    if(argc != 2){
        printf("usage: \n");
        printf("%s<service ip>\n",argv[0]);
        return -1;
    }
    clientfile = socket(AF_INET,SOCK_STREAM,0);
    if(clientfile == -1){
        printf("socket creat failed \n");
        return -1;
    }
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(MY_PORT);
    if(0 == inet_aton(argv[1], &client_addr.sin_addr)){
        printf("invalid server ip \n");
            return -1;
    }
    memset(my_addr.sin_zero,0 ,8 );
    fd = connect(clientfile,(struct sockaddr *)&client_addr,sizeof(struct sockaddr));
    if(fd < 0){
        printf("connect error \n");
        return -1;
    }
    while(1){
       if(fgets(SendBuf,999,stdin)){
           isendlen=  send(clientfile,SendBuf,strlen(SendBuf),0);
           if(isendlen <= 0){
            printf("send failed \n");
            return -1;
           }
       }
        
    
      
    }
    return 0;
}
