/*
	ECE 587
	Portland State University Summer 2015
	Project 2: VMM simulator
	
	Jeff Nguyen <jqn@pdx.edu>
	Paul Long	<paul@thelongs.ws>
	
	
	
	Default output is directed to stdout, redirect on commanline if desired.
	
*/




#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>



/*		Constants							*/
#define PF_SIZE 1024						// 1k pageframes

/*		Some convenient types				*/

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
#ifdef __HAS_LONG_LONG__
typedef unsigned long long 	u64;
#endif



/*		Global Data Structures				*/
typedef struct {
	u32	address;
	u16 reserved;
	u8  dirty;
	u8  present;
} PF_t;


/*		Global variables					*/
char *cmd;									// command being simulated
PF_t PD[PF_SIZE];							// static Page directory



/*		Function forward definitions		*/
void init_pf (PF_t *);
void print_pf(PF_t *);



int main(int argc, char *argv[]){
	
	FILE *fp;								// stimulus file handle
	char filename[50] = "stim.txt";			// stimulus file name, can be overwritten on cmdline
	
	int opt = 0;
	
	// parse the cmdline
	while ((opt = getopt(argc, argv, "i:p:")) != -1) {
		switch(opt) {
			case 'i':	strcpy(filename, optarg); break;
			case 'p':	break;// PWL THIS DOESNT DO ANYTHING YET!!!!!!!!!!!!!!!!!!!!!!!!
			case '?': 
						if (optopt == 'i'){
							printf("USAGE: -i <filename>\n\n");
							return -1;
						} else if (optopt == 'p') {
							printf("USAGE: -p <numpages>\n\n");
							return -1;
						} else {
							printf("Valid switches are:\n");
							printf("\t -i <input_filename>\n\t -p <numpages>\n\n");
							return -1;
						break;
						}
		}
	}
	
	
	fp = fopen (filename,"r");
	if (NULL == fp){
		fprintf (stderr, "Failed to open input file. This is fatal!\n");
		exit;
	}
	
	init_pf(PD);
	print_pf(PD);
	
	
	
	
}



void init_pf(PF_t *pf){
	int i;				//loop counter

	for (i=0; i<PF_SIZE; i++) {
			pf[i].address	= 0;
			pf[i].reserved	= 0;
			pf[i].dirty 	= 0;
			pf[i].present	= 0;
	}
}

void print_pf(PF_t *pf){
	int i;
	printf("=========================================\n");
	printf("      current page frame contents\n");
	printf("=========================================\n");
	
	for (i=0; i<PF_SIZE; i++) {
		printf("%4d\taddress = %08X d = %d p = %d\n", i, pf[i].address, pf[i].dirty, pf[i].present);
	}
	printf("\n\n");
}

