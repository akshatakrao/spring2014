#ifndef TECHRYPT_H_INCLUDED
#define TECHRYPT_H_INCLUDED
#endif

#define SYNTAX_MESSAGE "\nSyntax: techcrypt <input_file> [-d <IP:Port>] [-l]\n"
#define PASSWORD_SIZE 15

void remoteSecureCopy(FILE* file, char* ipPortString, char* encryptedContents, size_t len);
void localSecureCopy(FILE* file, char* inputFilePath , char* encryptedContents, size_t len);
void writeToFile(FILE* file, char* contents);
void makeCopy(char* commandLineOption, char* filePath, char* ipPortString);


