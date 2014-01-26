#include<stdio.h>
#include<gcrypt.h>
#include<encrypt.h>
#include<errorcodes.h>
#include<unistd.h>
#include<techrypt.h>

/*
1. Stage 1  - Take a file and encrypt it 
2. Stage 2 - Send file to remote
3. Stage 3 - Network daemon
4. Stage 4 - Local mode
*/

#define SYNTAX_MESSAGE "\nSyntax: techcrypt <input_file> [-d <IP:Port>] [-l]\n"

char* requestPassphrase();
void remoteSecureCopy(FILE* file, char* ipPortString);
void localSecureCopy(FILE* file);


int main(int argc, char *argv[])
{
    char *filePath, *commandLineOption, *ipPortString;
    
    FILE *file;

	  if(argc < 3)
    {
          printf("\nInsufficient arguments\n");
          printf(SYNTAX_MESSAGE);
          exit(INVALID_SYNTAX);
    }
    else if((argc == 4) && (strcmp(argv[2],"-d") != 0))
    {
           printf("\nIncorrect Syntax1");
           printf(SYNTAX_MESSAGE);
           exit(INVALID_SYNTAX);
    }
    else if((argc == 3) && (strcmp(argv[2],"-l") != 0))
    {
           printf("\nIncorrect Syntax2");
           printf(SYNTAX_MESSAGE);
           exit(INVALID_SYNTAX);
    }

    filePath = argv[1];

    if(access( filePath, F_OK) != 0)
    {
       fprintf(stderr, "Invalid File Path");
       exit(INVALID_FILE);
    }
    
    file = fopen(filePath, "r, ccs=UTF-8"); 
    
    commandLineOption = argv[2];
    if(strcmp(commandLineOption,"-d") == 0)
    {
      ipPortString = argv[3];
      remoteSecureCopy(file, ipPortString);  
    }
    else if(strcmp(commandLineOption, "-l") == 0)
    {
      localSecureCopy(file);
    }
    else
    {
      fprintf(stderr, "Invalid syntax");
      printf(SYNTAX_MESSAGE);
      exit(INVALID_SYNTAX);
    }
       
}

/**
 *Request Passphrase
 */
char* requestPassphrase()
{
  char* password = malloc(sizeof(char) * PASSWORD_SIZE);
  printf("Enter passphrase: ");
  fgets(password, PASSWORD_SIZE, stdin);

  return password; 
}

/**
 * Remote Secure Copy
 */
void remoteSecureCopy(FILE* file, char* ipPortString)
{
  char* passphrase;
  //TODO: Check if IP and Port are Valid

  passphrase = requestPassphrase();

  //Create an encrypted temp file
  //Create a socket connection
  //Copy the file 

}

/**
 * Local Copy
 */
void localSecureCopy(FILE *file)
{
  const char* passphrase;
  passphrase = requestPassphrase();
  
  
  //Check if output file already exists
  //Create encrypted File
}
