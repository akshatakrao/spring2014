#ifndef TECHDEC_H_INCLUDED
#define TECHDEC_H_INCLUDED
#endif


char* getDestinationFilePath(char* inputFilePath);
void localDecrypt(char* filePath, char* passphrase);
void remoteDecrypt(char* filePath, char* passphrase, char* port);
char* extractMACFromFile(char* filePath);
char* extractEncryptedStringFromFile(char* filePath);
int verifyMAC(char* encryptedContents, char* mac);





