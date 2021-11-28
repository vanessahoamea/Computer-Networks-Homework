#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>

void trim(char *command)
{
    int i, k=0;
    for(i=0;i<strlen(command);i++)
        if(command[i]!=' ')
            command[k++]=command[i];
    command[k-1]='\0';
}

int main()
{
    int sockets[2];
    char command[30], response[200];

    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);

    if(access("fifo.txt", F_OK)==-1)
        if(mknod("fifo.txt", S_IFIFO | 0666, 0)==-1)
        {       
            perror("Error creating fifo.txt\n");
            exit(3);
        }

    if(access("fifo2.txt", F_OK)==-1)
        if(mknod("fifo2.txt", S_IFIFO | 0666, 0)==-1)
        {       
            perror("Error creating fifo2.txt\n");
            exit(3);
        }

    int pd=fork();
    if(pd==-1)
    {
        perror("Error: fork\n");
        exit(3);
    }
    if(pd>0) //parent
    {
        close(sockets[0]);

        char buffer[10];
        int len=read(sockets[1], buffer, 10);
        buffer[len]='\0';

        if(strcmp(buffer, "error1")==0)
        {
            perror("Error opening fifo.txt\n");
            exit(3);
        }
        if(strcmp(buffer, "error1")==0)
        {
            perror("Error opening fifo2.txt\n");
            exit(3);
        }

        close(sockets[1]);
    }
    else //child
    {
        close(sockets[1]);

        int fd=open("fifo.txt", O_WRONLY);
        if(fd==-1)
            write(sockets[0], "error1", 6);

        int fd_return=open("fifo2.txt", O_RDONLY);
        if(fd_return==-1)
            write(sockets[0], "error1", 6);

        while(1)
        {
            fgets(command, 30, stdin); trim(command);
            write(fd, command, strlen(command));

            int length=read(fd_return, response, 200);
            response[length]='\0';
            if(strcmp(response, "kill")==0)
                exit(0);
            printf("[%d] %s\n", length, response);
        }

        close(fd);
        close(fd_return);

        close(sockets[0]);
    }

    return 0;
}