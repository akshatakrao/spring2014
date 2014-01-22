#include<stdio.h>

/*
1. Stage 1  - Take a file and encrypt it 
2. Stage 2 - Send file to remote
3. Stage 3 - Network daemon
4. Stage 4 - Local mode
*/

void printSyntax();

int main(int argc, char *argv[])
{
	char* inputFile = "";

	//START-Validate Inputs
	if (argc < 5)
	{
		printf("\nInsufficient arguments");
		printSyntax();
		exit(1);
	}	
	
	//TODO: Validate syntax for command	
	//END - Validate commands
	
	//inputFile = argv[2];

//	printf("This is the file name: %s", inputFile);

         	


}





/**
Prints the syntax for the techrypt command	
*/
void printSyntax()
{

}
