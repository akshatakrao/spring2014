#include<gcrypt.h>
#include<cryptcommon.h>
#include<errorcodes.h>
#include<stdio.h>

/**
 *Decrypt encrypted string
 */
char* decryptContents(char* encryptedString, long fileSize, const char* password)
{
  gcry_cipher_hd_t handle;
  gcry_error_t crypt_err;
  gpg_error_t keygen_err;
  uint8_t keybuffer[KEY_SIZE];
  int errorCode;
  char* decryptedContents;
  int sizeOfDecryptedContents = fileSize - MAC_SIZE;	
  //TODO: Test for empty string

  crypt_err = gcry_cipher_open(&handle, ENCRYPTION_ALGO, ENCRYPTION_MODE, GCRY_CIPHER_SECURE);
  //TODO: Perform error checking      

  memset(keybuffer,0x0,sizeof(keybuffer));  
  generate_key(password, keybuffer);
  //TODO: Error checking
	
  crypt_err = gcry_cipher_setkey(handle, (const void *)&keybuffer[0],KEY_SIZE);
  //TODO: Error checking

  printf("Size of Decrypted Contents: %ld", sizeOfDecryptedContents);
  decryptedContents = (char *)malloc(sizeOfDecryptedContents);

  gcry_cipher_setiv(handle, &IV, 16);   
  
  errorCode = gcry_cipher_decrypt(handle, decryptedContents, sizeOfDecryptedContents, encryptedString, sizeOfDecryptedContents);
  
   
  if(errorCode)
  {
	  fprintf(stderr, "Failed to decrypt file: %s %s\n", gcry_strsource(errorCode), gcry_strerror(errorCode));
	  exit(DECRYPTION_FAILED);
  }

  return decryptedContents;
}

char* decrypt_file_inpath(char* filepath, const char* password)
{
   char* decryptedContents;

  //
  return "";
}

/**
 * Decrypt File
 */
void decryptFile(char* filepath, const char* password)
{ 
	init_gcrypt();
	decrypt_file_inpath(filepath, password);
	finalize_gcrypt();
}

