#include<gcrypt.h>
#include<stdio.h>
#include<encrypt.h>
#include <wchar.h>
#include<stdint.h>
#include<string.h>

/**
* encrypt_file_inpath
*
**/
void encrypt_file_inpath(char* filePath, const char* pwd)
{
	//TODO: Validate if file exists and if filepath is valid
	
	FILE* f = fopen(filePath, "r, ccs=UTF-8");
	
	if(!f)
	{
		fprintf(stderr, "\nUnable to create new file");
	}

	encrypt_file(f, pwd);

	//TODO: Error Checking
	fclose(f);

	return;
}

/**
* encrypt_file
*
**/
void encrypt_file(FILE* file, const char* pwd)
{
	gcry_cipher_hd_t handle;
	gcry_error_t crypt_err;
	gpg_error_t keygen_err;
	uint8_t keybuffer[KEY_SIZE];
	char* fileContents;
	char* encryptedContents;
	

	fileContents = readFileIntoString(file);
	//TODO: Test fileContents for empty or invalid



	crypt_err = gcry_cipher_open(&handle, ENCRYPTION_ALGO, ENCRYPTION_MODE, GCRY_CIPHER_SECURE);
	//TODO: Perform error checking      
	
        generate_key(pwd, keybuffer);
	//TODO: Error checking
	
	crypt_err = gcry_cipher_setkey(handle, (const void *)&keybuffer[0],KEY_SIZE);
	//TODO: Error checking


	gcry_cipher_encrypt(handle, encryptedContents, strlen(encryptedContents), 
	
/*	gcry_cipher_encrypt(handle,  

	


	
	crypt_err = gcry_cipher_close(handle);
	//TODO: Perform error checking
	wint_t c;

	for ( c; (c=fgetwc(file)) != WEOF;)
		printf("%c",c);	
	return;
*/
}


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
Clean up
**/
void finalize_gcrypt()
{
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

	//Zeroises the secure memeory and destroys the handler	
	gcry_control(GCRYCTL_TERM_SECMEM);

	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);


}
int main()
{
	char* path="test.txt";
	uint8_t keybuffer[KEY_SIZE];

	generate_key("whatsup", keybuffer);
	
	printf("%s ", readFileIntoString(path));
	//encrypt_file_inpath(path,"");
	
}


/**
* Generate Key
**/
void generate_key(const char* passphrase, uint8_t keybuffer[])
{
	gcry_error_t err;
	int i = 0;
	
	err = gcry_kdf_derive(passphrase, strlen(passphrase), KEY_ALGO, KEY_HASH_ALGO, KEY_SALT, strlen(KEY_SALT),NUMBER_OF_ITERATIONS ,sizeof(keybuffer), (void *)&keybuffer[0]);
 
	if(0 != err)
	{
		fprintf(stderr, "\nKey derivation failed due to %s", gcry_strerror(err));
	//	return -1;
	}
	
	for(i = 0; i < KEY_SIZE; i++)
	{
		printf("%x ",keybuffer[i]);
	}
}


/**
**/
char* readFileIntoString(char* filepath)
{
	FILE* file = fopen(filepath, "r, ccs=UTF-8");
	long int size = ftell(file);	
	char* fileString;

	if(file == NULL)
	{
		fprintf(stderr, "\nThe file path is invalid");
		exit(INVALID_FILE);		
	} 

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	fileString = calloc(size + 1, 1);
	fread(fileString, 1, size, file);

	return fileString;	
}
