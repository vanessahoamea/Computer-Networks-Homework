#pragma GCC diagnostic ignored "-Wstringop-overflow"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <utmp.h>
#include <string.h>

#define READ 0
#define WRITE 1

int main()
{
    int pipe_com[2], logged_in=0;
    char buffer[30], buffer2[200];

    pipe(pipe_com);
    
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
        close(pipe_com[WRITE]);

        int fd_return=open("fifo2.txt", O_WRONLY); //pt a returna raspunsurile
        if(fd_return==-1)
        {
            perror("Error opening fifo2.txt\n");
            exit(3);
        }

        int len=0;
        do
        {
            len=read(pipe_com[READ], buffer2, 200);
            buffer2[len]='\0';

            if(strcmp(buffer2, "quit")==0) //quit
            {
                write(fd_return, "kill", 4);
                exit(0);
            }

            if(strcmp(buffer2, "login-success")==0) //login
            {
                if(logged_in==0)
                {   
                    logged_in=1;
                    write(fd_return, "Login successful", 16);
                }
                else write(fd_return, "Already logged in", 17);
            }

            if(strcmp(buffer2, "login-error")==0)
                if(logged_in==0)
                    write(fd_return, "Login failed", 12);
                else write(fd_return, "Already logged in", 17);
            
            if(strcmp(buffer2, "logout")==0) //logout
            {
                if(logged_in==0)
                    write(fd_return, "No user is logged in at the moment", 34);
                else
                {
                    logged_in=0;
                    write(fd_return, "Logged out successfully", 23);
                }
            }

            if(strstr(buffer2, "g l u ")!=NULL) //get-logged-users
            {
                if(logged_in==0)
                    write(fd_return, "Error: must be logged in to execute this command", 48);
                else
                {
                    strcpy(buffer2, buffer2+6); len=len-6; buffer2[len]='\0';
                    write(fd_return, buffer2, len);
                }
            }

            if(strstr(buffer2, "g p i ")!=NULL) //get-proc-info
            {
                if(logged_in==0)
                    write(fd_return, "Error: must be logged in to execute this command", 48);
                else
                {
                    strcpy(buffer2, buffer2+6); len=len-6; buffer2[len]='\0';
                    write(fd_return, buffer2, len);
                }
            }

            if(strcmp(buffer2, "unknown")==0) //others
                write(fd_return, "Unknown command", 15);

        } while(len);

        close(pipe_com[READ]);
        close(fd_return);
    }
    else //child
    {
        close(pipe_com[READ]);

        int fd=open("fifo.txt", O_RDONLY);
        if(fd==-1)
        {
            perror("Error opening fifo.txt\n");
            exit(3);
        }
        int length=0;
        do
        {
            length=read(fd, buffer, 30);
            buffer[length]='\0';
            if(strcmp(buffer, "login")==0 || strcmp(buffer, "login:")==0 || strcmp(buffer, "get-proc-info")==0 || strcmp(buffer, "get-proc-info:")==0)
                write(pipe_com[WRITE], "login-error", 11);

            else if(strstr(buffer, "login")!=NULL) //login:username
            {
                int i, ok=0;
                char users_list[10], username[10];

                for(i=6;i<length;i++)
                    username[i-6]=buffer[i];
                username[length-6]='\n'; username[length-5]='\0';

                FILE *fp_users=fopen("users.txt", "r");
                if(fp_users==NULL)
                {
                    perror("Error opening users.txt\n");
                    exit(3);
                }
                while(fgets(users_list,10,fp_users))
                {
                    if(strcmp(users_list, username)==0)
                        ok=1;
                }
                fclose(fp_users);

                if(ok==1)
                    write(pipe_com[WRITE], "login-success", 13);
                else write(pipe_com[WRITE], "login-error", 11);
            }

            else if(strcmp(buffer, "get-logged-users")==0) //get-logged-users
            {
                char info[100], t1[9], t2[9];
                struct utmp *n;
                setutent(); n=getutent();
                while(n)
                {
                    if(n->ut_type==USER_PROCESS)
                    {
                        int h = (n->ut_tv.tv_sec/3600);
                        int m = (n->ut_tv.tv_sec -(3600*h))/60;
                        int s = (n->ut_tv.tv_sec -(3600*h)-(m*60));
                        sprintf(t1, "%d", m); strcat(t1, ":"); sprintf(t2, "%d", s); strcat(t1,t2);
                        strcpy(info, "g l u ");
                        strcat(info, n->ut_user); strcat(info, " | ");
                        strcat(info, n->ut_host); strcat(info, " | ");
                        strcat(info, t1); strcat(info, "\n");
                        write(pipe_com[WRITE], info, strlen(info));
                    }
                    n=getutent();
                 }
            }

            else if(strstr(buffer, "get-proc-info")!=NULL) //get-proc-info:pid
            {
                char path[20], line[50], result[200];

                strcpy(path, "/proc/"); strcat(path, buffer+14); strcat(path, "/status");
                strcpy(result, "g p i ");

                FILE *fp_pid=fopen(path, "r");
                if(fp_pid==NULL)
                {
                    perror("Error opening /proc\n");
                    exit(3);
                }
                while(fgets(line,50,fp_pid))
                {
                    if(strstr(line, "Name:")!=NULL || strstr(line, "State:")!=NULL || strstr(line, "PPid:")!=NULL || strstr(line, "Uid:")!=NULL || strstr(line, "VmSize:")!=NULL)
                        strcat(result, line);
                }
                fclose(fp_pid);

                write(pipe_com[WRITE], result, strlen(result));
            }

            else if(strcmp(buffer, "logout")==0) //logout
                write(pipe_com[WRITE], "logout", 6);

            else if(strcmp(buffer, "quit")==0) //quit
                write(pipe_com[WRITE], "quit", 4);

            else write(pipe_com[WRITE], "unknown", 7); //other commands

        } while(length);

        close(pipe_com[WRITE]);
        close(fd);
    }

    return 0;
}