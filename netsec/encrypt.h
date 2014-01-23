#ifndef ENCRYPT_H_INCLUDED
#define ENCRYPT_H_INCLUDED
#endif

//Error Codes
#define INVALID_GCRYPT_VERSION 1
#define INVALID_FILE 2

//Constants
#define SECURE_MEMORY_SIZE_IN_BYTES 8192

void encrypt_file_inpath(char* filepath, char* password);
void encrypt_file(FILE* file, char* password);
