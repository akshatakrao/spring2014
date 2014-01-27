#include<stdio.h>
#include<decrypt.h>
#include<unistd.h>
#include<errorcodes.h>
#include<stdlib.h>
#include<string.h>
#include<cryptcommon.h>

//READ MAC FROM DECRYPTED FILE
//READ ENCRYPTED STRING FROM FILE
//COMPARE
//DECRYPT ENCRYPTED STRING
//ERROR IF FILE exists
//WRITE ENCRYPTED STRING TO FILE

#define SYNTAX_MESSAGE "\nSyntax: techdec <output_file> [-d <port>] [-l]\n"

/**
 * Get Destination File Path
 */
char* getDestinationFilePath(char* inputFilePath)
{
    return "decrypted.txt";
        
    char* extension, *outputFilePath;
    //Check if string ends with .gt.  Else error

    char* start = (inputFilePath + strlen(inputFilePath) - 3);
    size_t number_of_characters = 3;
    strncpy(extension, &inputFilePath[strlen(inputFilePath) - 3], number_of_characters);

    printf("\nExtension: %s", extension);
    if(strcmp(extension, ".gt") == 0)
    {
        fprintf(stderr, "\nInvalid Encrypted File Extension");
        exit(INVALID_FILE);
    }
    else
    {
        strncpy(outputFilePath, inputFilePath, inputFilePath + (strlen(inputFilePath) - 4));
        printf("\nOutput File Path: %s", outputFilePath);
    }

    //Extract the substring
}

/**
 * Extract MAC from File
 */
char* extractMACFromFile(char* filePath)
{
    char* mac;
	  long int size;	

    FILE *file = fopen(filePath, "r");

    fseek(file, -64L, SEEK_END);

    mac = malloc(64);
	  fread(mac, 1, 64, file);

    return mac;
}

long getSizeOfEncryptedContent(FILE* file)
{
    long size;
     
    fseek(file, 0, SEEK_END);
	  size = ftell(file);
	  rewind(file);

    return size;
}
/**
 * Extract Encrypted String
 */
char* extractEncryptedStringFromFile(char* filePath)
{
    char* encryptedContents;
    long int size;

    FILE *file = fopen(filePath, "r");

    size = getSizeOfEncryptedContent(file);    
    encryptedContents = malloc(size - 64);
    fread(encryptedContents, 1, size-64, file);
    
    printf("\nLength of encryptedContents: %d", strlen(encryptedContents)); 
    return encryptedContents;
}

/**
 * Verify MAC
 */
int verifyMAC(char* encryptedContents, char* mac, char* passphrase)
{
    int macVerified = 1;
    char* generatedMAC;

    uint8_t keybuffer[KEY_SIZE];
   
    memset(keybuffer,0x0,sizeof(keybuffer));
 
    generate_key(passphrase, keybuffer);

    generatedMAC = generateHMAC(encryptedContents, keybuffer);

    if(strcmp(mac, generatedMAC) != 0)
    {
        macVerified = 0;
    }

    return macVerified;
}

/**
 *
 */
void localDecrypt(char* filePath, char* passphrase)
{
    char* mac, *encryptedContents, *destinationFilePath, *decryptedContents;
    FILE* file;
    long size;

    if(access(filePath, F_OK) != 0)
    {
       fprintf(stderr, "Input file does not exist!");
       exit(INVALID_FILE);
    }
  
    mac = extractMACFromFile(filePath);
    printf("MAC: %s", mac);
    encryptedContents = extractEncryptedStringFromFile(filePath);
    printf("\nEncryptedContents: %s", encryptedContents);

    file = fopen(filePath, "r");

    size = getSizeOfEncryptedContent(file);

    decryptedContents = decryptContents(encryptedContents, size, passphrase);
    printf("Decrypted Contents: %s", decryptedContents);

    if(!verifyMAC(encryptedContents, mac, passphrase))
    {
        fprintf(stderr, "\nThe MAC does not verify for the encrypted file");
        exit(INVALID_MAC);
    }
    else
    {
        printf("\nMAC verified");
    }


   destinationFilePath =  getDestinationFilePath(filePath);

}

/**
 *
 */
void remoteDecrypt(char* filePath, char* passphrase, char* port)
{
}

int main(int argc, char* argv[])
{
    char* commandLineOption, *filePath, *passphrase, *port;

	  if(argc < 3)
    {
          printf("\nInsufficient arguments\n");
          printf(SYNTAX_MESSAGE);
          exit(INVALID_SYNTAX);
    }
    else if((argc == 4) && (strcmp(argv[2],"-d") != 0))
    {
           printf("\nIncorrect Syntax");
           printf(SYNTAX_MESSAGE);
           exit(INVALID_SYNTAX);
    }
    else if((argc == 3) && (strcmp(argv[2],"-l") != 0))
    {
           printf("\nIncorrect Syntax");
           printf(SYNTAX_MESSAGE);
           exit(INVALID_SYNTAX);
    }

    commandLineOption = argv[2];
    filePath = argv[1];
    port = argv[3];

    passphrase = requestPassphrase();

    if(strcmp(commandLineOption, "-l") == 0)
    {
        localDecrypt(filePath, passphrase);
    }
    else if(strcmp(commandLineOption, "-d") == 0)
    {
        remoteDecrypt(filePath, passphrase, port);
    }
    else
    {
        fprintf(stderr, "\nInvalid File Syntax");
        exit(INVALID_SYNTAX);
    }
}
 

