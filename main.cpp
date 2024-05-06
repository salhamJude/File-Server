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

#define DEFAULT_BUFLEN 1024
void PANIC(char* msg);
#define PANIC(msg)  { perror(msg); exit(-1); }
unsigned int port = 1875;
char *valueD,*valueP, *valueU;
FILE *users;
bool sign_flag = false;
void cmdArgument(int argc, char *argv[]);



int main(int argc, char *argv[]){

    cmdArgument(argc,argv);
  
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