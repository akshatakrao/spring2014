#include<stdio.h>
#include<decrypt.h>
#include<unistd.h>
#include<errorcodes.h>
#include<stdlib.h>
#include<string.h>
#include<cryptcommon.h>
#include<gcrypt.h>

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

long getSizeOfEncryptedContent(FILE* file)
{
    long size;
     
    fseek(file, 0, SEEK_END);
	  size = ftell(file);
	  rewind(file);

    return size;
}


/**
 * Extract MAC from File
 */
void extractMACFromFile(char* filePath, char mac[])
{
	  long int size;
    int i = 0;
    char c = 0;  

    FILE *file = fopen(filePath, "r");
    size = getSizeOfEncryptedContent(file);
    fseek(file, -64, SEEK_END);

    for(i = 0; i < 64; i++)
    {
        mac[i] = fgetc(file);
    }

   // return &mac[0];
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
   
    return encryptedContents;
}

/**
 * Verify MAC
 */
int verifyMAC(char* encryptedContents, char* mac, char* passphrase)
{
    int macVerified = 1, i = 0;
    char* generatedMAC;

    uint8_t keybuffer[KEY_SIZE];
   
    memset(keybuffer,0x0,sizeof(keybuffer));
 
    generate_key(passphrase, keybuffer);

    generatedMAC = generateHMAC(encryptedContents, keybuffer);

    printf("\nMAC1: %s", mac);
    printf("\nMAC2: %s", generatedMAC);

    for(i = 0; i < 64; i ++)
    {
      if(mac[i] != generatedMAC[i])
      {
          printf("\nNot same: %d", i);
          macVerified = 0;
          break;
      }

    }

    return macVerified;
}


/**
*
**/
char* readFileIntoString(FILE* file)
{
	long int size;	
	char* fileString;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	fileString = calloc(size + 1, 1);
	fread(fileString, 1, size, file);

	return fileString;	
}


/**
 *
 */
void localDecrypt(char* filePath, char* passphrase)
{
    char *encryptedContents, *encryptedContents2, *destinationFilePath, *decryptedContents,*generatedMAC;
    long size;
    FILE* file, *outputFile;
    int i = 0, same = 1;
    uint8_t keybuffer[KEY_SIZE];
    char mac[MAC_SIZE];

    if(access(filePath, F_OK) != 0)
    {
       fprintf(stderr, "Input file does not exist!");
       exit(INVALID_FILE);
    }
 
     
    extractMACFromFile(filePath, mac);
//    printf("\nMAC: %s Count: %d", mac, strlen(mac));
   

    encryptedContents = extractEncryptedStringFromFile(filePath);
  //  printf("\nEncryptedContents: %s", encryptedContents);

    file = fopen(filePath, "r");

    size = strlen(encryptedContents);

    decryptedContents = decryptContents(encryptedContents, size, passphrase);

    memset(keybuffer,0x0,sizeof(keybuffer));

    printf("Passphrase1: %s", passphrase); 
    generate_key(passphrase, keybuffer);

    generatedMAC = generateHMAC(encryptedContents, keybuffer);
  //  printf("\nGenerated MAC: %s %d", generatedMAC, strlen(generatedMAC));


    for(i = 0; i < MAC_SIZE; i++)
    {
        if(generatedMAC[i] != mac[i])
        {
    //        printf("Not same: %d", i);
            same = 0;
            break;
        }
    }

    if(same)
    {
        printf("\nMAC verified\n");
    }
    else
    {
        fprintf(stderr, "\nThe MAC does not verify for the encrypted file");
        exit(INVALID_MAC);

    }

    //strcpy(destinationFilePath, filePath);
    //strcat(destinationFilePath, ".dec"); 

    destinationFilePath = "output.txt";

    if(access(destinationFilePath, F_OK) == 0)
    {
       fprintf(stderr, "\nOutput File Already Exists!");
       exit(OUTPUT_FILE_EXISTS);
    }

    outputFile = fopen(destinationFilePath,"w");
    fwrite(decryptedContents, strlen(decryptedContents), 1, outputFile);

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
 

