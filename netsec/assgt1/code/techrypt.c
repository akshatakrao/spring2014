#include<stdio.h>
#include<gcrypt.h>
#include<encrypt.h>
#include<errorcodes.h>
#include<unistd.h>
#include<techrypt.h>
#include<cryptcommon.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

/*
1. Stage 1  - Take a file and encrypt it 
2. Stage 2 - Send file to remote
3. Stage 3 - Network daemon
4. Stage 4 - Local mode
*/

#define SYNTAX_MESSAGE "\nSyntax: techcrypt <input_file> [-d <IP:Port>] [-l]\n"


int main(int argc, char *argv[])
{
    char *filePath, *commandLineOption, *ipPortString;
    char* destinationFilePath;

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
    commandLineOption = argv[2];

    if(argc ==  4)
    {
        ipPortString = argv[3];
    }

    makeCopy(commandLineOption, filePath, ipPortString); 
}

/**
 * Make Copy
 */
void makeCopy(char* commandLineOption, char* filePath, char* ipPortString)
{
  const char* passphrase;
  char* encryptedContents;
  long fileSize; 
  size_t len;

  if(access(filePath, F_OK) != 0)
  {
       fprintf(stderr, "Invalid File Path");
       exit(INVALID_FILE);
  }
    
  FILE *file = fopen(filePath, "r, ccs=UTF-8"); 
 
  passphrase = requestPassphrase();
  encryptedContents = encrypt_file(file, passphrase); 
  len = 144;
  if(strcmp(commandLineOption,"-d") == 0)
  {
      remoteSecureCopy(file, ipPortString, encryptedContents, len);  
  }
  else if(strcmp(commandLineOption, "-l") == 0)
  {
      localSecureCopy(file, filePath, encryptedContents, len);
  }
  else
  {
      fprintf(stderr, "\nInvalid syntax");
      printf(SYNTAX_MESSAGE);
      exit(INVALID_SYNTAX);
  }


}

/**
 *
 */
char* getDestinationFilePath(char* filePath)
{
    char *destinationFilePath = malloc(strlen(filePath) + 3);
    strcpy(destinationFilePath, filePath);
    strcat(destinationFilePath, ".gt");

    printf("\nDestinationFilePath: %s", destinationFilePath);
    printf("\nOriginal file path: %s", filePath);
  
    return destinationFilePath;

}


/**
 * Remote Secure Copy
 */
void remoteSecureCopy(FILE* file, char* ipPortString, char* encryptedContents, size_t len)
{
  
   char* IP;
   char* portString;
   struct sockaddr_in serverAddress;
   in_port_t port = atoi(portString);
   int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), ret;
   size_t bytesSent; 

   if(sock < 0)
   {
       fprintf(stderr, "\nInvalid Socket");
       exit(INVALID_SOCKET);   
   }
  
   memset(&serverAddress, 0 , sizeof(serverAddress));
   serverAddress.sin_family = AF_INET;
   ret = inet_pton(AF_INET, IP, &serverAddress.sin_addr.s_addr));

   if(ret <= 0)
   {
      fprintf(stderr, "\nInvalid IP Address");
      exit(INVALID_ADDR);
   }

   serverAddress.sin_port = htons(port);

   if(connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
   {
       fprintf(stderr, "\nConnection Failed");
       exit(CONNECTION_FAILED);
   }

   bytesSent = 

    
  //TODO: Check if IP and Port are Valid
  //Create an encrypted temp file
  //Create a socket connection
  //Copy the file 

}

/**
 * Local Copy
 */
void localSecureCopy(FILE *inputFile, char* filePath, char* encryptedContents, size_t len)
{
    //Generate destination File Path
    //TODO: Test for complete filepaths 
    char* destinationFilePath = getDestinationFilePath(filePath);
    FILE* outputFile;

    if(access(destinationFilePath, F_OK) == 0)
    {
       fprintf(stderr, "Output File Already Exists!");
       exit(OUTPUT_FILE_EXISTS);
    }
    else
    {
      outputFile = fopen(destinationFilePath, "w");  
      writeToFile(outputFile, encryptedContents);
    }
}

/**
 *Write to File
 */
void writeToFile(FILE* file, char* encryptNMACContents)
{
  fwrite(encryptNMACContents, strlen(encryptNMACContents), 1, file);
  fclose(file);
} 


