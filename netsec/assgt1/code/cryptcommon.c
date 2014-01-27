#include<cryptcommon.h>
#include<gcrypt.h>
#include<stdint.h>
#include<errorcodes.h>
#include<stdio.h>

/**
*  init_gcrypt - Initializes the GCrypt library
*		 Creates secure memory	
**/
void init_gcrypt()
{
	if(!gcry_check_version(GCRYPT_VERSION))
	{
		fprintf(stderr, "\nInvalid LibGcrypt version\n");
		exit(INVALID_GCRYPT_VERSION);
	}

	//Suppress warnings
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

	//Allocate 8K of secure memory
	gcry_control(GCRYCTL_INIT_SECMEM,SECURE_MEMORY_SIZE_IN_BYTES,0);

	//Resume warnings
	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

	//Complete initialization
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

	if(gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
	{
		printf("\nInitialization Complete");
	}
	else
	{
		printf("\n Initialization failed");
		exit(CRYPT_INITIALIZATION_FAILED); //TODO: Add the exit code to header
	}

}

/**
* Generate Key
**/
void generate_key(const char* passphrase, uint8_t keybuffer[])
{
	gcry_error_t err;
	int i = 0;
	
	err = gcry_kdf_derive("abcdefgh", strlen("abcdefgh"), KEY_ALGO, KEY_HASH_ALGO, KEY_SALT, strlen(KEY_SALT),NUMBER_OF_ITERATIONS ,sizeof(keybuffer), (void *)&keybuffer[0]);
 
	if(0 != err)
	{
		fprintf(stderr, "\nKey derivation failed due to %s", gcry_strerror(err));
	//	return -1;
	}

  printf("\nKey: ");  
	for(i = 0; i < KEY_SIZE; i++)
	{
		printf("%x ",keybuffer[i]);
	}
  printf("\n");
}


/**
Clean up
**/
void finalize_gcrypt()
{
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

	//Zeroises the secure memeory and destroys the handler	
	gcry_control(GCRYCTL_TERM_SECMEM);

	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);


}

/**
 *
 */
char* generateHMAC(char* inputString, uint8_t keybuffer[])
{
  gcry_error_t err;
  gcry_mac_hd_t handle;
  char* macString, *macString2;
  size_t mac_length = MAC_SIZE;

  err = gcry_mac_open(&handle, GCRY_MAC_HMAC_SHA512, GCRY_MAC_FLAG_SECURE, NULL);
  //TODO: Catch error

  err = gcry_mac_setkey(handle, (const void *)&keybuffer[0], KEY_SIZE);
  //TODO: Catch error

  err = gcry_mac_write(handle, (void *)inputString, strlen(inputString));
  //TODO: Catch Error

  macString = malloc(MAC_SIZE + 1);
  macString[MAC_SIZE] = '\0';

  err = gcry_mac_read(handle, (void *)macString, &mac_length); 
  
  printf("\nMAC: %s", macString);
  printf("\nMAC Length: %d", mac_length);

  gcry_mac_close(handle);
 
  return macString; 
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
 * Get File Size as a multiple of block size
 * **/
static long getFileEncryptedLength(long fileLength)
{

   //TODO: Replace with block size/KEY SIze 
	if(fileLength < KEY_SIZE)
		fileLength = KEY_SIZE;
	else
	{
		fileLength = fileLength%KEY_SIZE + fileLength;
	}
	
	printf("\nFile length : %ld ", fileLength);

  return fileLength;
}

