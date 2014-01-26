#include<stdio.h>
#include<gcrypt.h>
#include<encrypt.h>
#include<errorcodes.h>

/*
1. Stage 1  - Take a file and encrypt it 
2. Stage 2 - Send file to remote
3. Stage 3 - Network daemon
4. Stage 4 - Local mode
*/

#define SYNTAX_MESSAGE "Syntax: techcrypt <input_file> [-d <IP:Port>] [-l]"

int main(int argc, char *argv[])
{
    char *filePath, *commandLineOption, *ipPortString;
    
    FILE *file;

	  if(argc <= 3)
    {
          printf("\nInsufficient arguments");
          printf(SYNTAX_MESSAGE);
          exit(INVALID_SYNTAX);
    }
    else if(((argc == 4) && (strcmp(argv[2],"-d") != 0)) && ((argc == 3) && (strcmp(argv[3],"-l") != 0)))
    {
           printf("\nIncorrect Syntax");
           printf(SYNTAX_MESSAGE);
           exit(INVALID_SYNTAX);
    }

    filePath = argv[1];

    if(access( filePath, F_OK) != -1)
    {
       fprintf(stderr, "Invalid File Path");
       exit(INVALID_FILE);
    }
    
    file = fopen(filePath, "r, ccs=UTF-8");
  
    
    if(strcmp(commandLineOption,"-d") == 0)
    {
      ipPortString = argv[3];
      remoteSecureCopy(file, ipPortString);  
    }
    else if(strcmp(commandLineOption, "-l") == 0)
    {
      localSecureCopy(file, ipPortString);
    }
    else
    {
      fprintf(stderr, "Invalid syntax");
      printf(SYNTAX_MESSAGE);
      exit(INVALID_SYNTAX);
    }
       
}

/**
 *
 */
void remoteSecureCopy(FILE* file, char* ipPortString)
{

}

/**
 *
 */
void localSecureCopy(FILE* file)
{


}




/**
Prints the syntax for the techrypt command	
*/
void printSyntax()
{

}
