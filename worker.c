#include "list.h"

char* arg[8];
DIR * dirp;
DIR * subdir;
int dirs_to_read, count_files, total, error;
char input[64];
char*** dates_file;
list_node*** head;
char** country;
int fdp, fdw;

void sig_handler(int signo){
    int i, j, k, total, flag;
    struct dirent * entry;
    struct dirent * dir_file;
    char path[64], path2[64], fdate[32], buf[512];
    date inDate, outDate;
    char recordID[32], enex[8], patientFirstName[32], patientLastName[32], diseaseID[32], age[4];
    int range[10][4];
    FILE* fp;

    if (signo == SIGINT || signo == SIGQUIT){
        return;
    }else if (signo == SIGUSR1){
        kill(getppid(), SIGUSR1);
        for (i=0; i<dirs_to_read; i++){
            do{
                entry=readdir(dirp);
            }while(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0);
            strcpy(path, "");
            strcat(path, input);
            strcat(path, "/");
            strcat(path, entry->d_name);
            subdir=opendir(path);
            dir_file = readdir(subdir);
            flag=1;
            while(dir_file!=NULL){
                do{
                    dir_file = readdir(subdir);
                    if (dir_file==NULL)
                        break;
                }while(strcmp(dir_file->d_name, ".")==0 || strcmp(dir_file->d_name, "..")==0);

                if (dir_file!=NULL){
                    flag=1;
                    for(j=0; j<count_files; j++){
                        if (strcmp(dates_file[i][j], dir_file->d_name)==0)
                            flag=0;
                    }
                }
                if (flag==1){
                    strcpy(path2, "");
                    strcat(path2, path);
                    strcat(path2, "/");
                    strcat(path2, dir_file->d_name);
                    fp=fopen(path2, "r");
                    for(j=0; j<10; j++){
                        for (k=0; k<4; k++)
                            range[j][k]=0; 
                    }
                    while(fscanf(fp,"%s %s %s %s %s %s",recordID , enex, patientFirstName, patientLastName, diseaseID, age)!=EOF){
                        total++;
                        strcpy(fdate, dir_file->d_name);

                        if (strcmp(enex, "ENTER")==0){      //if record is enter
                            string_to_date(fdate, &inDate);
                            string_to_date("-", &outDate);
                            if (check_list(head[i][j], recordID)==0){
                                for (j=0; j<10 ;j++){
                                    if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                                        break;
                                }
                                list_insert(&head[i][j], recordID, age, diseaseID, patientFirstName, patientLastName, inDate, outDate);
                                if ((atoi(age))<=20){
                                    range[j][0]++;          //range 0-20
                                }else if ((atoi(age))<=40){
                                    range[j][1]++;          //range 20-40
                                }else if ((atoi(age))<=60){
                                    range[j][2]++;          //range 40-60
                                }else{
                                    range[j][3]++;          //range 60+
                                }
                            }else{
                                printf("ERROR\n");
                                error++;
                            }
                        }else{                              //if record is exit
                            string_to_date(fdate, &outDate);
                            for (j=0; j<10 ;j++){
                                if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                                    break;
                            }
                            if (set_exitdate(head[i][j], recordID, outDate)==1)
                                error++;
                            
                        }

                        
                    }
                    sprintf(buf,"%s\n%s\n",dir_file->d_name, country[i]);
                    //printf("%s", buf);
                    write(fdw, buf, 512);
                    for (k=0; k<10; k++){
                        if(head[i][k]==NULL)
                                break;
                    sprintf(buf, "%s\nAge range 0-20 years: %d cases\nAge range 20-40 years: %d cases\nAge range 40-60 years: %d cases\nAge range 60+ years: %d cases\n",head[i][k]->diseaseID, range[k][0], range[k][1], range[k][2], range[k][3]);
                    write(fdw, buf, 512);
                    //printf("%s", buf);
                    }
                }

            fclose(fp);

            }

            closedir(subdir);
        }
        write(fdw, "DONE", 512);

    }
    return;
}

int main(int argc, char *argv[]){	//argv: 1-input 2-start_from 3-dirs_to_read 4-parent(read) pipe 5-worker(write) pipe

	int start_from, dirs_to_read, i, j, k, nread, count, return_count, l;

    DIR * subdir;
    struct dirent * entry;
    struct dirent * dir_file;
    char path[128];
    char path2[128];
    char recordID[32], enex[8], patientFirstName[32], patientLastName[32], diseaseID[32], age[4];
    FILE* fp;
    list_node*** head;
    date inDate, outDate, dfrom, dto;
    char fdate[32];

    int range[10][4];
    char buf[512];

    char* token;
    char command[64];

    char tempsort[128];

    strcpy(input, argv[1]);
    start_from=atoi(argv[2]);
    dirs_to_read=atoi(argv[3]);

    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGUSR1, sig_handler);

    total=0;    //total (success and error)
    error=0;    //only errors

    head=malloc(dirs_to_read*sizeof(list_node**));     //10 different diseases(one list for each) for every country to read
    country=malloc(dirs_to_read*sizeof(char*));        //array of strings to hold country names read
    
    for(i=0; i<dirs_to_read; i++){
        head[i]=malloc(10*sizeof(list_node*));
        country[i]=malloc(32*sizeof(char));
        for(j=0; j<10; j++)
            head[i][j]=NULL;
    }
    for (i=0; i<8; i++)
        arg[i]=malloc(64*sizeof(char));

    

    dirp = opendir(argv[1]);
//    printf("Gonna open %s and %s....,\n", argv[4], argv[5]);
    fdp=open(argv[4], O_RDONLY);
    fdw=open(argv[5], O_WRONLY);


    for(i=1; i<start_from; i++){
        do{
            entry=readdir(dirp);
        }while(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0);
    }


    for (i=0; i<dirs_to_read; i++){
        do{
            entry=readdir(dirp);
        }while(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0);
        strcpy(country[i], entry->d_name);
        strcpy(path, "");
        strcat(path, argv[1]);
        strcat(path, "/");
        strcat(path, entry->d_name);
        subdir=opendir(path);
        count_files=0;
        do{                     //read number of files in country directory
            do{
                dir_file = readdir(subdir);
                if (dir_file==NULL)
                    break;
            }while(strcmp(dir_file->d_name, ".")==0 || strcmp(dir_file->d_name, "..")==0);
            if (dir_file!=NULL)
                count_files++;
        }while(dir_file!=NULL);


        if (i==0){           //allocate memory only once
            dates_file=malloc(dirs_to_read*sizeof(char**));      //array of date files in folder to sort and read after
            for(j=0; j<dirs_to_read; j++){
                dates_file[j]=malloc(count_files*sizeof(char*));
                for(k=0; k<count_files; k++)
                    dates_file[j][k]=malloc(64*sizeof(char));
            }
        }
        rewinddir(subdir);

        for(j=0; j<count_files; j++){       //copy file names(dates) to array
            do{
                dir_file = readdir(subdir);
                if (dir_file==NULL)
                    break;
            }while(strcmp(dir_file->d_name, ".")==0 || strcmp(dir_file->d_name, "..")==0);
            if (dir_file!=NULL)
                strcpy(dates_file[i][j], dir_file->d_name);
        }

        strcpy(tempsort, "");                   //bubble sort to array of date names
        for (j=0; j<count_files; j++){
            for (k=0; k<(count_files-j-1); k++){
                strcpy(tempsort, dates_file[i][k]);
                string_to_date(tempsort, &inDate);
                strcpy(tempsort, dates_file[i][k+1]);
                string_to_date(tempsort, &outDate);
                if (date_older(inDate, outDate)==2){
                    strcpy(tempsort, dates_file[i][k]);
                    strcpy(dates_file[i][k], dates_file[i][k+1]);
                    strcpy(dates_file[i][k+1], tempsort);
                    printf("%s\n",tempsort);
                }
                
            }
        }
        rewinddir(subdir);

        for(l=0; l<count_files; l++){
            strcpy(path2, "");
            strcat(path2, path);
            strcat(path2, "/");
            strcat(path2, dates_file[i][l]);
            fp=fopen(path2, "r");
            for(j=0; j<10; j++){
                for (k=0; k<4; k++)
                    range[j][k]=0; 
            }
            while(fscanf(fp,"%s %s %s %s %s %s",recordID , enex, patientFirstName, patientLastName, diseaseID, age)!=EOF){
                total++;
                strcpy(fdate, dates_file[i][l]);

                if (strcmp(enex, "ENTER")==0){      //if record is enter
                    string_to_date(fdate, &inDate);
                    string_to_date("-", &outDate);
                    if (check_list(head[i][j], recordID)==0){
                        for (j=0; j<10 ;j++){
                            if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                                break;
                        }
                        list_insert(&head[i][j], recordID, age, diseaseID, patientFirstName, patientLastName, inDate, outDate);
                        if ((atoi(age))<=20){
                            range[j][0]++;          //range 0-20
                        }else if ((atoi(age))<=40){
                            range[j][1]++;          //range 20-40
                        }else if ((atoi(age))<=60){
                            range[j][2]++;          //range 40-60
                        }else{
                            range[j][3]++;          //range 60+
                        }
                    }else{
                        printf("ERROR\n");
                        error++;
                    }
                }else{                              //if record is exit
                    string_to_date(fdate, &outDate);
                    for (j=0; j<10 ;j++){
                        if(head[i][j]==NULL || (strcmp(head[i][j]->diseaseID, diseaseID)==0)) //find the list with the same disease
                            break;
                    }
                    if (set_exitdate(head[i][j], recordID, outDate)==1)
                        error++;
                    
                }

                
            }
            sprintf(buf,"%s\n%s\n",dir_file->d_name, country[i]);
            //printf("%s", buf);
            write(fdw, buf, 512);
            for (k=0; k<10; k++){
                if(head[i][k]==NULL)
                        break;
            sprintf(buf, "%s\nAge range 0-20 years: %d cases\nAge range 20-40 years: %d cases\nAge range 40-60 years: %d cases\nAge range 60+ years: %d cases\n",head[i][k]->diseaseID, range[k][0], range[k][1], range[k][2], range[k][3]);
            write(fdw, buf, 512);
            //printf("%s", buf);
            }

        fclose(fp);

        }

        closedir(subdir);
    }
    write(fdw, "DONE", 512);

    do{
        nread=read(fdp, &buf, 512);
        if(nread>0){

            if (strcmp(buf, "/listCountries")==0){            //listcountries
                for(i=0; i<dirs_to_read; i++){
                    sprintf(path, " %d", getpid());
                    strcpy(buf, country[i]);
                    strcat(buf, path);
                    write(fdw, buf, 512);
                }
                write(fdw, "OK", 512);
            }else if(strcmp(buf, "/diseaseFrequency")==0){       //diseasefrequency args: 1-disease 2,3- dates (4-country)
                nread=read(fdp, &buf, 512);
                for (i=0; i<8; i++)
                    strcpy(arg[i], "*");
                // printf("strings is: |%s|\n", buf);
                token=strtok(buf," ");
                for(i=0; i<8 && token!=NULL; i++){
                    strcpy(arg[i], token);
                    token=strtok(NULL, " ");
                }
                string_to_date(arg[2], &dfrom);
                string_to_date(arg[3], &dto);
                if(strcmp(arg[4],"*")==0){              //no country argument
                    count=0;
                    return_count=0;
                    for (i=0; i<dirs_to_read; i++){
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_dates(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                    }
                }else{                                  //country argument
                    count=0;
                    return_count=0;
                    for (i=0; i<dirs_to_read; i++){
                        if (strcmp(country[i], arg[4])==0)
                            break;
                    }
                    if (i<dirs_to_read){
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_dates(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                    }
                }
                strcpy(buf,"");
                sprintf(buf, "%d", return_count);
                write(fdw,buf, 512);
            }else if (strcmp(buf, "/search")==0){
                nread=read(fdp, arg[1], 512);
                strcpy(buf, "*");
                for (i=0; i<dirs_to_read; i++){
                    for (j=0; j<10; j++){
                        if (return_record(head[i][j], arg[1], buf)==0){
                            i=dirs_to_read;
                            break;
                        }
                    }
                }
                write(fdw, buf, 512);
            }else if (strcmp(buf, "/admissions")==0){
                nread=read(fdp, &buf, 512);
                for (i=0; i<8; i++)
                    strcpy(arg[i], "*");
                // printf("strings is: |%s|\n", buf);
                token=strtok(buf," ");
                for(i=0; i<8 && token!=NULL; i++){
                    strcpy(arg[i], token);
                    token=strtok(NULL, " ");
                }
                string_to_date(arg[2], &dfrom);
                string_to_date(arg[3], &dto);
                if(strcmp(arg[4],"*")==0){              //no country argument
                    for (i=0; i<dirs_to_read; i++){
                        count=0;
                        return_count=0;
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_dates(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                        if(i<dirs_to_read){
                            strcpy(buf,"");
                            sprintf(buf, "%s %d",country[i], return_count);
                            write(fdw,buf, 512);
                        }
                    }
                }else{                                  //country argument
                    count=0;
                    return_count=0;
                    for (i=0; i<dirs_to_read; i++){
                        if (strcmp(country[i], arg[4])==0)
                            break;
                    }
                    if (i<dirs_to_read){
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_dates(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                        strcpy(buf,"");
                        sprintf(buf, "%s %d",country[i], return_count);
                        write(fdw,buf, 512);
                    }
                }
                write(fdw,"DONE", 512);
            }else if (strcmp(buf, "/discharges")==0){
                nread=read(fdp, &buf, 512);
                for (i=0; i<8; i++)
                    strcpy(arg[i], "*");
                token=strtok(buf," ");
                for(i=0; i<8 && token!=NULL; i++){
                    strcpy(arg[i], token);
                    token=strtok(NULL, " ");
                }
                string_to_date(arg[2], &dfrom);
                string_to_date(arg[3], &dto);
                if(strcmp(arg[4],"*")==0){              //no country argument
                    for (i=0; i<dirs_to_read; i++){
                        count=0;
                        return_count=0;
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_discharges(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                        if(i<dirs_to_read){
                            strcpy(buf,"");
                            sprintf(buf, "%s %d",country[i], return_count);
                            write(fdw,buf, 512);
                        }
                    }
                }else{                                  //country argument
                    count=0;
                    return_count=0;
                    for (i=0; i<dirs_to_read; i++){
                        if (strcmp(country[i], arg[4])==0)
                            break;
                    }
                    if (i<dirs_to_read){
                        for (j=0; j<10; j++){       //find disease list
                            if (strcmp(head[i][j]->diseaseID, arg[1])==0){
                                count=count_discharges(head[i][j], dfrom, dto);
                                return_count+=count;
                                break;
                            }
                        }
                        strcpy(buf,"");
                        sprintf(buf, "%s %d",country[i], return_count);
                        write(fdw,buf, 512);
                    }
                }
                write(fdw,"DONE", 512);

            }else if (strcmp(buf, "/exit")==0){
                strcpy(buf, "");
                for(i=0; i<dirs_to_read; i++){
                    strcat(buf, country[i]);
                    strcat(buf,"\n");
                }
                write(fdw, buf, 512);
                write(fdw, "DONE", 512);
                sprintf(buf, "%d", total);
                write(fdw, buf, 512);
                sprintf(buf, "%d", error);
                write(fdw, buf, 512);
                break;
            }else{
//                printf("Kati paixthke edw me to buf: |%s|\n", buf);
            }
        }
    }while (strcmp(command, "/exit")!=0);


    // sprintf(buf, "%d", getpid());
    // strcpy(path, "log_file.");
    // strcat(path, buf);
    // fp=fopen(path, "w");
    // for(i=0; i<dirs_to_read; i++)
    //     fprintf(fp, "%s\n", country[i]);
    // fprintf(fp, "TOTAL %d\nSUCCESS %d\nERROR %d\n", total, total-error, error);

    closedir(dirp);
    for(j=0; j<dirs_to_read; j++){
        free(country);
        for(i=0; i<10; i++)
            delete_list(&head[j][i]);
        for(k=0; k<count_files; k++)
            free(dates_file[j][k]);
        free(dates_file[j]);
    }
    unlink(argv[4]);
    unlink(argv[5]);

    write(fdw, "KILLME", 512);

    return 0;

}