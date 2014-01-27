#ifndef ENCRYPT_H_INCLUDED
#define ENCRYPT_H_INCLUDED
#endif

//Encryption Constants


char* readFileIntoString(FILE* file);
char* encryptString(char* inputString, long contentSize, const char* pwd);
void encrypt_file_inpath(char* filepath, const char* password);
char* encrypt_file(FILE* file, const char* password);
void encryptFile();
