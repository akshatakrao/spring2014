#ifndef DECRYPT_H_INCLUDED
#define DECRYPT_H_INCLUDED
#endif

char* decryptContents(char* encryptedString, long fileSize, const char* password);
void decryptFile(char* file, const char* password);
char* decrypt_file_inpath(char* filepath, const char* password);


