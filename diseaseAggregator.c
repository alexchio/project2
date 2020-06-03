#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "list.h"

pid_t* pid;
int numWorkers;
int* fdp;
int* fdw;


void sig_handler(int signo){
    int i, count, return_count, errors, return_errors, nread;
    char path[64], buf[512], par[64], wor[64], id[64]  ;
    FILE* fp;



    if (signo == SIGINT || signo == SIGQUIT){
//        printf("\nreceived SIGINT\n");
        for(i=0; i<numWorkers; i++)
            write(fdp[i], "/exit", 512);
        count=0;
        return_count=0;
        errors=0;
        return_errors=0;
        strcpy(path, "");
        sprintf(buf, "%d", getpid());
        strcpy(path, "log_file.");
        strcat(path, buf);
        fp=fopen(path, "w");
        for (i=0; i<numWorkers; i++){
            do{
                nread=read(fdw[i],buf, 512);
                if(nread>0 && strcmp(buf, "DONE")!=0)
                    fprintf(fp, "%s", buf);
            }while(strcmp(buf, "DONE")!=0);
        }
        
        for (i=0; i<numWorkers; i++){
            read(fdw[i],buf, 512);
            return_count=atoi(buf);
            count+=return_count;
            read(fdw[i],buf, 512);
            return_errors=atoi(buf);
            errors+=return_errors;
        }

        fprintf(fp,"TOTAL %d\nSUCCESS %d\nFAIL %d\n", count, count-errors, errors);
        fclose(fp);
        for (i=0; i<numWorkers; i++){
            do{
                nread=read(fdw[i],buf, 512);
                if(nread>0 && strcmp(buf, "KILLME")!=0){
                    printf("Diavasa apo to paidi %d to |%s|\n", pid[i], buf);
                    kill(pid[i], SIGKILL);
                }
            }while(strcmp(buf, "KILLME")!=0);
        }
        for (i=0; i<numWorkers; i++){
            strcpy(par, "par");
            strcpy(wor, "wor");
            sprintf(id, "%d", i);
            strcat(par, id);
            strcat(wor, id);
            unlink(par);
            unlink(wor);
        }
        exit(0);
    }else if(signo == SIGUSR1){
        while(1){
            nread=read(fdw[i], &buf, 512);
            if (strcmp(buf, "DONE")==0)
                break;
            if(nread>0) 
                printf("%s\n", buf);
        }
    }

}



int main (int argc, char* argv[]){
    int i, bufferSize, file_count, dirs_to_read, workers_extra, start_from, nread, count, return_count, errors, return_errors;
    char input_dir[32], start[3], dirs[3];
    char par[32], wor[32], id[5];
    DIR * dirp;
    struct dirent * entry;


    char* arg[8];
    char* token;
    char string[512];
    char buf[512], path[512];
    char command[64];
    FILE* fp;


    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGUSR1, sig_handler);

    if (argc!=7){
        printf("Wrong number of arguments\n");
        return 1;
    }

    for (i=1; i<argc ; i+=2){       //read flags
        if (strcmp("-w", argv[i])==0)
            numWorkers=atoi(argv[i+1]);
        else if (strcmp("-b", argv[i])==0)
            bufferSize=atoi(argv[i+1]);
        else if (strcmp("-i", argv[i])==0)
            strcpy(input_dir, argv[i+1]);
        else{
            printf("Wrong flag\n");
            return 1;
        }
    }


    dirp = opendir(input_dir);  //read number of subdirectories
    file_count=-2;  //not to count . and ..
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR) {
            file_count++;
        }
    }
    closedir(dirp);

    if (numWorkers>file_count)  //if workers are more than directories
        numWorkers=file_count;

    dirs_to_read=file_count/numWorkers;
    workers_extra=file_count%numWorkers;    //first workers_extra will read one more dir


    pid=malloc(numWorkers*sizeof(pid_t));
    fdp=malloc(numWorkers*sizeof(int));
    fdw=malloc(numWorkers*sizeof(int));
    for (i=0; i<numWorkers; i++){
        strcpy(par, "par");
        strcpy(wor, "wor");
        sprintf(id, "%d", i);
        strcat(par, id);
        strcat(wor, id);
        mkfifo(par, 0666);
        mkfifo(wor, 0666);

    }


    start_from=1;
    for (i=0; i<numWorkers; i++){
        pid[i]=fork();

        if (pid[i]==0){     //worker process
            strcpy(par, "par");
            strcpy(wor, "wor");
            sprintf(id, "%d", i);
            strcat(par, id);
            strcat(wor, id);
            if(i<workers_extra){
                //exec me extra dirtoread
                sprintf(dirs,"%d", dirs_to_read+1);
                sprintf(start,"%d", start_from);
                execlp("./worker","worker",input_dir ,start, dirs, par, wor, (char *)NULL);

            }else{
                //exec xwris
                sprintf(dirs,"%d", dirs_to_read);
                sprintf(start,"%d", start_from);
                execlp("./worker","worker",input_dir ,start, dirs, par, wor, (char *)NULL);

            }
        }

        if (i<workers_extra)
            start_from+= (dirs_to_read+1);
        else
            start_from+= dirs_to_read;
    }
    
//    sleep(1);
    for(i=0; i<numWorkers; i++){
        strcpy(par, "par");
        strcpy(wor, "wor");
        sprintf(id, "%d", i);
        strcat(par, id);
        strcat(wor, id);
        fdp[i]=open(par, O_WRONLY);
        fdw[i]=open(wor, O_RDONLY);
    }
    for(i=0; i<numWorkers; i++){
        while(1){
            nread=read(fdw[i], &buf, 512);
            if (strcmp(buf, "DONE")==0)
                break;
            if(nread>0) 
                printf("%s\n", buf);
        }
    }

    for (i=0; i<8; i++)
        arg[i]=malloc(64*sizeof(char));
    do{
        for (i=0; i<8; i++)
            strcpy(arg[i], "*");
        fflush(stdin);
        scanf(" %[^\n]", string);
        strcpy(buf, string);
        token=strtok(string," ");
        strcpy(command, token);
        for(i=0; i<8 && token!=NULL; i++){
             strcpy(arg[i], token);
             token=strtok(NULL, " ");
        }

        token=strtok(string," ");
        strcpy(command, token);
        for(i=0; i<8 && token!=NULL; i++){
             strcpy(arg[i], token);
             token=strtok(NULL, " ");
        }
        if (strcmp(command, "/listCountries")==0){
            for(i=0; i<numWorkers; i++)
                write(fdp[i], "/listCountries", 512);
            for(i=0; i<numWorkers; i++){
                while(1){
                    nread=read(fdw[i], buf, 512);
                    if (strcmp(buf, "OK")==0)
                        break;
                    if(nread>0)
                        printf("%s\n", buf);
                }
            }            
            
        }else if (strcmp(command, "/diseaseFrequency")==0){
            count=0;
            for(i=0; i<numWorkers; i++){
                write(fdp[i], "/diseaseFrequency", 512);
                write(fdp[i], buf, 512);
            }
            for (i=0; i<numWorkers; i++){
                read(fdw[i],buf, 512);
                return_count=atoi(buf);
                count+=return_count;
            }
            printf("%d\n", count);
 
        }else if (strcmp(command, "/topk-AgeRanges")==0){

            //TODO

        }else if (strcmp(command, "/searchPatientRecord")==0){
            for(i=0; i<numWorkers; i++){
                write(fdp[i], "/search", 512);
                write(fdp[i], arg[1], 512);
            }
            for(i=0; i<numWorkers; i++){
                read(fdw[i], buf, 512);
                if (strcmp(buf, "*")!=0)
                    printf("%s\n", buf);
            }
        }else if (strcmp(command, "/numPatientAdmissions")==0){
            for(i=0; i<numWorkers; i++){
                write(fdp[i], "/admissions", 512);
                write(fdp[i], buf, 512);
            }
            for (i=0; i<numWorkers; i++){
                do{
                    nread=read(fdw[i],buf, 512);
                    if(nread>0 && strcmp(buf, "DONE")!=0)
                        printf("%s\n", buf);
                }while(strcmp(buf, "DONE")!=0);
            }

        }else if (strcmp(command, "/numPatientDischarges")==0){
            for(i=0; i<numWorkers; i++){
                write(fdp[i], "/discharges", 512);
                write(fdp[i], buf, 512);
            }
            for (i=0; i<numWorkers; i++){
                do{
                    nread=read(fdw[i],buf, 512);
                    if(nread>0 && strcmp(buf, "DONE")!=0)
                        printf("%s\n", buf);
                }while(strcmp(buf, "DONE")!=0);
            }
        

        }else if (strcmp(command, "/exit")==0){
            for(i=0; i<numWorkers; i++)
                write(fdp[i], command, 512);
            count=0;
            return_count=0;
            errors=0;
            return_errors=0;
            strcpy(path, "");
            sprintf(buf, "%d", getpid());
            strcpy(path, "log_file.");
            strcat(path, buf);
            fp=fopen(path, "w");
            for (i=0; i<numWorkers; i++){
                do{
                    nread=read(fdw[i],buf, 512);
                    if(nread>0 && strcmp(buf, "DONE")!=0)
                        fprintf(fp, "%s", buf);
                }while(strcmp(buf, "DONE")!=0);
            }
            
            for (i=0; i<numWorkers; i++){
                read(fdw[i],buf, 512);
                return_count=atoi(buf);
                count+=return_count;
                read(fdw[i],buf, 512);
                return_errors=atoi(buf);
                errors+=return_errors;
            }

            fprintf(fp,"TOTAL %d\nSUCCESS %d\nFAIL %d\n", count, count-errors, errors);
            fclose(fp);
            for (i=0; i<numWorkers; i++){
                do{
                    nread=read(fdw[i],buf, 512);
                    if(nread>0 && strcmp(buf, "KILLME")!=0){
                        printf("Diavasa apo to paidi %d to |%s|\n", pid[i], buf);
                        kill(pid[i], SIGKILL);
                    }
                }while(strcmp(buf, "KILLME")!=0);
            }
            break;
        }
    }while (strcmp(command, "/exit")!=0);


    for (i=0; i<numWorkers; i++){
        strcpy(par, "par");
        strcpy(wor, "wor");
        sprintf(id, "%d", i);
        strcat(par, id);
        strcat(wor, id);
        unlink(par);
        unlink(wor);
    }
    return 0;
}