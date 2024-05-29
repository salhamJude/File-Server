#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dirent.h>

#define DEFAULT_BUFLEN 1024
void PANIC(char* msg);
#define PANIC(msg)  { perror(msg); exit(-1); }
unsigned int port = 1875;
char *valueD,*valueP, *valueU;
FILE *users;
bool sign_flag = false;
void cmdArgument(int argc, char *argv[]);
void* Child(void* arg);
char* parseRequest(char* input);
char** parseArguments(char* input, int* s);
char* handleConnexion(char* input, bool* c, int* at,char** username, char** password, char** path);
char* ListCommand(char* path, bool c);
void foldercheck(char* name, char** path);
char* GetCommand(char* path, bool c, char* line);
char* PutCommand(char* path, bool c, char* line, int client);

int main(int argc, char *argv[]){

    cmdArgument(argc,argv);
    int sd,opt,optval;
    struct sockaddr_in addr;

    if ( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        PANIC("Socket");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    // set SO_REUSEADDR on a socket to true (1):
    optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);


    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
        PANIC("Bind");
    if ( listen(sd, SOMAXCONN) != 0 )
        PANIC("Listen");

    printf("System ready on port %d\n",ntohs(addr.sin_port));
    const char welcomeMessage []= "Welcome to Bob's file server.\n";
    while (1)
    {
        int client;
        socklen_t addr_size = sizeof(addr);
        pthread_t child;

        client = accept(sd, (struct sockaddr*)&addr, &addr_size);
        printf("Connected: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        if ( !send(client, welcomeMessage, sizeof(welcomeMessage), 0)) {
            printf("Send failed\n");
        }
        if ( pthread_create(&child, NULL, Child, &client) != 0 )
            perror("Thread creation");
        else
            pthread_detach(child);  /* disassociate from parent */
    }

    return 0;
}

void cmdArgument(int argc, char *argv[]){ //This function check if the program is called with the right arguments
    int opt;
    int  pflag, uflag,dflag;
    

    while ((opt = getopt(argc, argv, "d:p:u:")) != -1) {
        switch (opt) {
            case 'd':
                dflag =1; valueD = optarg;
            case 'p':
                pflag = 1;valueP =optarg;
                break;
            case 'u':
                uflag = true;valueU = optarg;
                break;
            default: 
                std::cerr << "Usage: " << argv[0] << "[-d value] [-p value] [-u value]\n";
                exit(EXIT_FAILURE);
        }
    }

    if (!pflag or !uflag or !dflag) {
        std::cerr << "Usage: " << argv[0] << " [-f value] [-o value]\n";
        exit(EXIT_FAILURE);
    }

    int i = optind;
    while (i < argc)
    {
        std::cerr << "No aditionnal argument needed" << std::endl;
        exit(EXIT_FAILURE);
    }
    port = atoi(valueP);

}
void* Child(void* arg) //This function is the child thread that will handle the client request
{   
    char line[DEFAULT_BUFLEN];
    int bytes_read;
    int client = *(int *)arg;
    char* r;
    bool connected = false;
    char* response;
    int accesTry = 0;
    char *username, *password, *path;
    do
    {
        bytes_read = recv(client, line, sizeof(line), 0);
        if (bytes_read > 0) {
            r = parseRequest(line);
            if(r){ 
                if(strcasecmp(r, "USER") == 0){
                    response = handleConnexion(line,&connected,&accesTry,&username,&password,&path);
                    if ( !send(client,response, strlen(response), 0)) {
                        printf("Send failed\n");
                    }
                }else if(strcasecmp(r, "LIST") == 0){
                    response = ListCommand(path,connected);
                    if ( !send(client,response, strlen(response), 0)) {
                        printf("Send failed\n");
                    }
                }else if(strcasecmp(r, "GET") == 0){
                    response = GetCommand(path,connected,line);
                    if ( !send(client,response, strlen(response), 0)) {
                        printf("Send failed\n");
                    }
                }else if(strcasecmp(r, "PUT") == 0){
                    printf("put");
                }else if(strcasecmp(r, "DEL") == 0){
                    printf("del");
                }else if(strcasecmp(r, "QUIT") == 0){
                    printf("quit");
                }else{
                    if ( !send(client,"unkown command\n", sizeof("unkown command\n"), 0)) {
                        printf("Send failed\n");
                    }
                }
                free(r);
            }else{
                if ( !send(client,"Enter a command\n", sizeof("Enter a command\n"), 0)) {
                        printf("Send failed\n");
                }
            }
            memset(line,0,DEFAULT_BUFLEN);
            
        } else if (bytes_read == 0 ) {
            printf("Connection closed by client\n");
            break;
        } else {
            printf("Connection has problem\n");
            break;
        }
    } while (bytes_read > 0);
    close(client);
    return arg;
}
char* parseRequest(char* input){  //This function retreive the request of the user
    char* request = strtok(strdup(input), " ");
    char** argRequest = (char**)malloc(4 * sizeof(char*));
    if (!argRequest) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    int i = 0;

    char* r = strdup(request);
    /*while (request != NULL && i < 4) {
        argRequest[i] = strdup(request);
        if (!argRequest[i]) {
            printf("request : Memory allocation failed!\n");
            return NULL;
        }
        request = strtok(NULL, " ");
        i++;
    }

    for (int j = 0; j < i; j++) {
        free(argRequest[j]);
    }*/
    free(argRequest);
    int len = strlen(r);
    if (len > 0 && r[len - 1] == '\n') {
        r[len - 1] = '\0';  
    }
    return r;
}
char** parseArguments(char *input, int *s) //This function retreive the arguments of the command
{

    char* request = strtok(input, " ");
    char** argRequest = (char**)malloc(4 * sizeof(char*));
    if (!argRequest) {
        printf("arg : Memory allocation failed!\n");
        return NULL;
    }

    int i = 0;
    while (request != NULL && i < 4) {
        argRequest[i] = strdup(request);
        if (!argRequest[i]) {
            printf("arg : Memory allocation failed!\n");
            return NULL;
        }
        request = strtok(NULL, " ");
        i++;
    }

    *s = i;
    return argRequest;
}
char* handleConnexion(char* input, bool* c, int* at,char** username, char** password, char** path){  //This function handle the connexion of the user
    
    char* response;
    if(*c == true){ // check if the user is already connected
        char** args;
        int nbArg;
        args = parseArguments(input, &nbArg);
        if(nbArg == 3){
            if(!strcmp(*username,args[1]) && !strcmp(*password,args[2])){ // check if it is the same user
                response = (char*)"You are already logged in\n";
                return response;
            }else{ // if it is not the same user, try to connect with the new user
                users = fopen(valueU,"r");
                if(users){
                    char* l = NULL;
                    size_t len =0;
                    bool found = false;
                    char* val[2];
                    while(getline(&l,&len,users) != -1){
                        char *data = strtok(l,":");
                        int i =0;
                        while(data != NULL){
                            val[i] =  data;
                            data = strtok(NULL,":");
                            i++;
                        }
                        if(!strcmp(val[0],args[1]) && !strcmp(val[1],args[2])){
                            found = true;
                            break;
                        }
                        
                    }
                    if(found){
                        *c = true;
                        *username = val[0];
                        *password = val[1];
                        foldercheck(*username, path);
                        response = (char*)"200 USER granted to access\n";
                    }else{
                        *at += 1;
                        response = (char*) "400 USER not found\n";
                    }
                    
                }else{ // if the file which contains all the users can not be opened    
                    response = (char*) "Can not be connected, please try again\n";
                }
            }
        }else{ // if the user does not enter any argument, only the command
            *at += 1;
            response = (char*) "400 USER not found\n";
        }

        for(int i = 0; i < nbArg; i++){
            free(args[i]);
        }
        free(args);
    }else{ // if the user is not connected yet
        if(  3 > *at){
            char** args;
            int nbArg;
            args = parseArguments(input, &nbArg);
            if(nbArg == 0){ // if the user does not enter any argument, only the command
                *at += 1;
                response = (char*) "400 USER not found\n";
            }
            if(args){
                if(nbArg == 3){ // if the user enter the command and the username and the password
                    users = fopen(valueU,"r");
                    if(users){
                        char* l = NULL;
                        size_t len =0;
                        bool found = false;
                        char* val[2];
                        while(getline(&l,&len,users) != -1){
                            char *data = strtok(l,":");
                            int i =0;
                            while(data != NULL){
                                val[i] =  data;
                                data = strtok(NULL,":");
                                i++;
                            }
                            if(!strcmp(val[0],args[1]) && !strcmp(val[1],args[2])){
                                found = true;
                                break;
                            }
                            
                        }
                        if(found){ // if the user is found in the file
                            *c = true;
                            *username = val[0];
                            *password = val[1];
                            foldercheck(*username, path);
                            response = (char*)"200 USER granted to access\n";
                        }else{ // if the user is not found in the file
                            *at += 1;
                            response = (char*) "400 USER not found\n";
                        }
                        
                    }else{ // if the file which contains all the users can not be opened
                        response = (char*) "400 USER not found\n";
                    }
                }else{
                    response = (char*) "Missing arguments\n";
                }
            }else{ // if the user does not enter any argument, only the command
                *at += 1;
                response = (char*) "400 USER not found\n";
            }

            for(int i = 0; i < nbArg; i++){ // free the memory
                free(args[i]);
            }
            free(args);

        }else{ // if the user tried more than 3 times
            *at += 1;
            response = (char*) "You tried more than 3 times, the system is locked, please try later\n";
        }
    }
    return response;
}
char* ListCommand(char *path, bool c){ //This function show all file inside the user directory
    if(!c){
        return (char*)"You are not connected\n";
    }
    struct dirent *dp;
    struct stat fileStat;
    DIR *dir = opendir(path);

    if (!dir) {
        printf("Cannot open directory: %s\n", path);
        return NULL;
    }

    size_t bufferSize = 1024;
    char *listing = (char *)malloc(bufferSize);
    if (!listing) {
        printf("Memory allocation failed.\n");
        closedir(dir);
        return NULL;
    }
    listing[0] = '\0';

    snprintf(listing + strlen(listing), bufferSize - strlen(listing), "Listing of directory: %s\n", path);
    while ((dp = readdir(dir)) != NULL) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", path, dp->d_name);

        if (stat(filepath, &fileStat) < 0) {
            printf("Failed to get information about %s\n", dp->d_name);
            continue;
        }

        // Append file/directory name and size to the listing string
        snprintf(listing + strlen(listing), bufferSize - strlen(listing), "%-30s", dp->d_name);

        if (S_ISDIR(fileStat.st_mode)) {
            snprintf(listing + strlen(listing), bufferSize - strlen(listing), "\n");
        } else {
            snprintf(listing + strlen(listing), bufferSize - strlen(listing), "%lld bytes\n", (long long)fileStat.st_size);
        }
    }

    closedir(dir);
    return listing;
}

void foldercheck(char* name,char** path){
    char* cp = (char*)malloc(strlen(valueD) + strlen(name) + 2);
    if(!cp){
        printf("Memory allocation failed!\n");
        return;
    }
    strcat(cp,valueD);
    strcat(cp,"/");
    strcat(cp,name);
    if (opendir(cp)) {
        printf("Folder exists\n");
    } else {
        if(mkdir(cp, 0755) == 0) printf("Folder created\n");
        else printf("Unable to create folder\n");
    }
    *path = strdup(cp);
    free(cp);
}
char *GetCommand(char *path, bool c, char *line)
{
    if(!c){
        return (char*)"You are not connected\n";
    }
    char* response;
    char** args;
    int nbArg;
    args = parseArguments(line, &nbArg);
    if(nbArg == 2){
        char* fpath = (char*)malloc(strlen(path) + strlen(args[1]) + 1);
        strcat(fpath,path);
        strcat(fpath,"/");
        strcat(fpath,args[1]);
        fpath[strlen(fpath)-1] = '\0';
        printf("path : %s\n",fpath);

        FILE* fp = fopen(fpath, "r");
        if (fp == NULL) {
            free(fpath);
            return (char*) "404 File not found.\n";
        }
        fseek(fp, 0, SEEK_END); 
        long file_size = ftell(fp); 

        if (file_size == -1) {
            free(fpath);
            fclose(fp);
            return (char*) "The file is empty.";
        }
        fseek(fp, 0, SEEK_SET); 
        char* buffer = (char*) malloc(file_size + 1);
        if (buffer == NULL) {
            free(fpath);
            fclose(fp);
            return (char*) "Could not proceed your operation, please try again."; 
        }

        size_t bytes_read = fread(buffer, 1, file_size, fp);
        if (bytes_read != (size_t)file_size) {
            free(buffer);
            free(fpath);
            fclose(fp);
            return (char*) "Could not proceed your operation, please try again."; 
        }
        buffer[file_size] = '\0';
        fclose(fp);
        response = strdup(buffer);
        free(fpath);
    }
    else{
        return (char*)"Missing arguments\n";
    }

    return response;
}
char* PutCommand(char* path, bool c, char* line, int client) { //This function handle the put command
  if (!c) {
    return strdup("You are not connected\n");
  }

  char* response;
  char** args;
  int nbArg;
  args = parseArguments(line, &nbArg);

  if (nbArg == 2) {
    size_t path_len = strlen(path);
    size_t arg1_len = strlen(args[1]);
    size_t fpath_len = path_len + 1 + arg1_len + 1; 
    char* fpath = (char*)malloc(fpath_len);
    if (fpath == NULL) {
      free(args); 
      return strdup("500 Internal server error.\n");
    }

    snprintf(fpath, fpath_len, "%s/%s", path, args[1]);

    FILE* fp = fopen(fpath, "w");
    if (fp == NULL) {
      free(fpath);
      free(args); 
      return strdup("400 File cannot save on server side.\n");
    }

    size_t total_data_len = 0;
    char* all_data = NULL;

    while (1) {
      char buffer[DEFAULT_BUFLEN];
      ssize_t bytes_read = recv(client, buffer, sizeof(buffer), 0);
      if (bytes_read <= 0) {
        break; 
      }

      size_t new_data_len = total_data_len + bytes_read;
      char* new_data = (char*)realloc(all_data, new_data_len + 1); 
      if (new_data == NULL) {
        free(all_data); 
        fclose(fp);
        free(fpath);
        free(args); 
        return strdup("500 Internal server error.\n");
      }

      all_data = new_data;
      memcpy(all_data + total_data_len, buffer, bytes_read);
      total_data_len = new_data_len;

      if (total_data_len >= 5 && strncmp(all_data + total_data_len - 5, "\r\n.\r\n", 5) == 0) {
        all_data[total_data_len - 5] = '\0'; 
        fwrite(all_data, 1, total_data_len - 5, fp);
        break;
      }else if (total_data_len >= 3 && strncmp(all_data + total_data_len - 3, "\n.\n", 3) == 0) {
        all_data[total_data_len - 3] = '\0'; 
        fwrite(all_data, 1, total_data_len - 3, fp);
        break;
      }
    }

    if (ferror(fp)) { 
      free(all_data);
      fclose(fp);
      free(fpath);
      free(args); 
      return strdup("400 File cannot save on server side.\n");
    }

    fclose(fp);
    free(all_data);
    free(fpath);
    free(args); 

    char response[100];
    sprintf(response, "200 File received successfully. %ld bytes transferred.\n", total_data_len - 5);
    return strdup(response);
  } else {
    free(args); 
    return strdup("Missing arguments\n");
  }
}