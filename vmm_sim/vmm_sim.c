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
#define PF_SIZE			1024				// 1k pageframes
#define ADDR_SIZE_BITS	32
//#define ADDR_STRING_LEN	((ADDR_SIZE_BITS >> 3) + 2)
#define ADDR_STRING_LEN 10
//#define MAX_PF			((1<<ADDR_SIZE_BITS)/PF_SIZE)
#define MAX_PF			4194304
#define MIN_PF			3
#define MATCH			0
#define PD_MASK			0xFFC00000
#define PT_MASK			0x3FF000
#define OS_MASK			0xFFF
#define	PD_BITS			10
#define	PT_BITS			10
#define	OS_BITS			12



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

typedef struct {
	u16	PD_index;
	u16	PT_index;
	u16 UP_offset;
} VA_t;


/*		Global variables					*/
char *cmd ;									// command being simulated
u32  addr;									// address being simulated
u32	 available_pf = 10;						// user specified number of pageframes (default = 10)
PF_t PD[PF_SIZE];							// Page Directory
VA_t working_addr;							// virtual address that is currently getting manipulated
u8	 t_flag;								// various debug flags
u8	 d_flag;
u8	 v_flag;



/*		Function forward definitions		*/
void init_pf (PF_t *);
void print_pf(PF_t *);
void get_command(FILE *);
void validate_pf_number(u32);
void validate_addr(char *);
void parse_addr(u32);
int is_present(u16, const char *);



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
	
	// open the input file
	fp = fopen (filename,"r");
	if (NULL == fp){
		fprintf (stderr, "Failed to open input file. This is fatal!\n");
		exit(0);
	}
	// setup a PD
	init_pf(PD);
	
	// loop through input file an feed commands to sim
	//int count=1;
	do {
		//printf("getting command #%d \n",count++);
		get_command(fp);
		if (strcmp(cmd, "-v") == MATCH) {
			printf("VMM Simulator Ver 0.1\n"); 
			exit(-1);
		}
		else if (strcmp(cmd, "w") == MATCH || strcmp(cmd, "r") == MATCH ) {
			parse_addr(addr);
			//printf("working address\n");
			//printf("\tPD %d\n", working_addr.PD_index );
			//printf("\tPT %d\n", working_addr.PT_index );
			//printf("\tOS %d\n", working_addr.UP_offset);
			if (is_present(working_addr.PD_index, "PD") == 1) {
				if (is_present(working_addr.PT_index, "PT") == 1) {
					//This is a hit
					//update r/w counters
				}
			}
		}
			
		
	} while (strcmp (cmd, "EOF"));
	//printf("Caught EOF, we are done here. G'night\n");
		
	
}


/*	Simple initialization of a page frame

	Takes pointer to the PF
	Iterates through every line of the PF
	Sets all members in the struct = 0
*/
void init_pf(PF_t *pf){
	int i;				//loop counter

	for (i=0; i<PF_SIZE; i++) {
			pf[i].address	= 0;
			pf[i].reserved	= 0;
			pf[i].dirty 	= 0;
			pf[i].present	= 0;
	}
}


/*	Simple print out of a page frame

	Takes a pointer to the PF
	Interates through each line of the PF
	Prints out the all members of the struct 
*/
void print_pf(PF_t *pf){
	int i;
	printf("=========================================\n");
	printf("      current page frame contents\n");
	printf("=========================================\n");
	
	for (i=0; i<PF_SIZE; i++) {
		printf("%4d\taddress = %08X d = %d p = %d\n",\
			    i, pf[i].address, pf[i].dirty, pf[i].present);
	}
	printf("\n\n");
}

void get_command(FILE *fp){
	static int first_time = 1;
	static int eof_valid  = 0;
	static int need_num_pages = 0;
	static int need_addr = 0;
	static char token[11];

	static int rc;
	
	rc = fscanf(fp, "%s", token);
	//printf("just after fscanf, token = %8s \teof_valid = %d\n", token, eof_valid);
	if (rc == EOF && !eof_valid) {
		fprintf(stderr, "invalid EOF in input file; this is fatal\n");
		exit (-1);	//PWL SHOULD DO CLEANUP BEFORE EXIT!!!!!!!!!!!!!!!!
	}
	else if (rc == EOF ) {
		printf("rc = EOF\n");
		cmd = "EOF";
	}
	else if (first_time && strcmp(token, "-p")== MATCH) {  			//strcmp returns 0 if match
		//printf("first_time = %d token = %s\n", first_time, token);
		cmd = "p";
		first_time = 0;						// clear first time flag cause we have been here already
		need_num_pages = 1;					// set flag to require a pagenumber next in input file
		get_command(fp);
	}
	else if (need_num_pages) {
		available_pf = (u32) strtol(token, NULL, 10);
		validate_pf_number(available_pf);
		eof_valid = 1;
		need_num_pages = 0;
	}
	else if (need_addr) {
		validate_addr(token);
		need_addr = 0;
		eof_valid = 1;
	}
	else if ( strcmp(token, "w") == MATCH ||
			  strcmp(token, "W") == MATCH ) {
			  
		cmd = "w";
		need_addr = 1;
		get_command(fp);
	}
	else if ( strcmp(token, "r") == MATCH ||
			  strcmp(token, "R") == MATCH ) {
		cmd = "r";
		need_addr= 1;
		get_command(fp);
	}
	else if (strcmp(token, "-v") == MATCH) {v_flag = 1;}
	else if (strcmp(token, "-t") == MATCH) {t_flag = 1;}
	else if (strcmp(token, "-d") == MATCH) {d_flag = 1;}
	else {
		fprintf(stderr, "Invalid instruction sequence: got %s\n", token);
		exit(-1);
	}
			
		
	first_time = 0;
}

void validate_pf_number(u32 num_pf){
	if (num_pf < MIN_PF || num_pf > MAX_PF) {
		fprintf(stderr, "Invalid number of PF specified\n");
		fprintf(stderr, "Must be between %u and %u ", MIN_PF, MAX_PF);
		fprintf(stderr, "but %u was specified\n", num_pf);
		fprintf(stderr, "this is fatal: quitting\n");
		exit(-1);//PWL DO CLEANUP BEFORE EXIT!!!!!!!!!!!!!!!!
	}
}

void validate_addr(char * token) {
	printf("validating address\n");
	if ( (strncmp(token, "0x", 2) == MATCH) &&
		 (strlen(token) <= ADDR_STRING_LEN) ) {
			 
		addr = (u32) strtol(token,NULL,16);
	} else {
		fprintf(stderr, "illegal address specified\n");
		fprintf(stderr, "valid address is of the form 0xffffffff\n");
		fprintf(stderr, "%s was specified\n", token);
		exit(-1);//PWL DO CLEANUP BEFORE EXIT!!!!!!!!!!!!!!!!
	}
}

/*	takes the supplied address for the inputfile
	and parses it into the working_addr data
	structure
	*/
	
void parse_addr(u32 addr) {
	working_addr.PD_index  = (addr & PD_MASK) >> (PT_BITS + OS_BITS);
	working_addr.PT_index  = (addr & PT_MASK) >> OS_BITS;
	working_addr.UP_offset = (addr & OS_MASK);
}

int is_resident(u16 index, const char* PF_type) {
	// increment machine cycle counters here
	if ( strcmp(PF_type, "PD") == MATCH ) {
		return PD[index].present;
	} else if (strcmp(PF_type, "PD") == MATCH){
		return PT[index].present;
	} else {
		//error case
		exit(-1);
	}
}