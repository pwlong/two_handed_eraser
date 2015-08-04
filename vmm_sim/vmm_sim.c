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

/*		Some convenient types				*/

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
#ifdef __HAS_LONG_LONG__
typedef unsigned long long 	u64;
#endif

/*		Function forward definitions		*/


/*		Global Data Structures				*/
typedef struct {
	u32	address;
	u16 reserved;
	u8  dirty;
	u8  present;
} PF_t;

int main(int argc, char *argv[]){
	
	FILE *fp;								// stimulus file handle
	char filename[100] = "stim.txt";		// stimulus file name
	
	int opt = 0;
	char *in_fname = NULL;
	char *out_fname = NULL;
	
	while ((opt = getopt(argc, argv, "i:p:")) != -1) {
		switch(opt) {
			case 'i':	strcpy(filename, optarg); break;
			case 'p':	break;
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
	
	
	
	
	
	
}





