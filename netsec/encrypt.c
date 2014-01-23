#include<gcrypt.h>
#include<stdio.h>
#include<encrypt.h>
#include <wchar.h>

/**
* encrypt_file_inpath
*
**/
void encrypt_file_inpath(char* filePath, char* pwd)
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
void encrypt_file(FILE* file, char* pwd)
{
	wint_t c;

	for ( c; (c=fgetwc(file)) != WEOF;)
		printf("%c",c);	
	return;
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

}

int main()
{
	char* path="test.txt";

	encrypt_file_inpath(path,"");
	
}
