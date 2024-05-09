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