/* Jeff
ECE 587
	Portland State University Summer 2015
	Project 2: VMM simulator
	Version v0.1
	
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
#define ADDR_STRING_LEN 10
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



/*		Global statistics trackers					*/
u32	num_accesses;							
u32 num_writes;
u32 num_cycles_no_vmm;

PF_t * PDBR;
u32  used_pf   = 0;                         // tracking numbers of pf used

int total_cycles   = 0;

int num_PT     = 0;                          //number of PT and max number of PT
int max_num_PT = 0;                          //was used

int num_UP     = 0;                          //number of UP and max number of UP 
int max_num_UP = 0;                          //was used

int num_physical_frames = 0;

int num_swap_in = 0;                         //number of swap-in and out ops
int num_swap_out= 0;

int num_pure_replace = 0;

int num_PD_entry = 0;



/*		Function forward definitions		*/
void init_PD ();
void print_pf(PF_t *);
void get_command(FILE *);
void validate_pf_number(u32);
void validate_addr(char *);
void parse_addr(u32);
int is_PT_present(u16);
int is_UP_present(u16 pd_index, u16 pt_index);
void create_page_table(u16 pd_index, const char* rw);
void create_user_page(u16 pd_index, u16 pt_index,const char* rw);
void evict_page();
void print_outputs();
void dump_vmm();



int main(int argc, char *argv[]){
	
	FILE *fp;								// stimulus file handle
	char filename[50] = "stim.txt";			// stimulus file name, can be overwritten on cmdline
	 // if filename given on commandline use it otherwise use default filename
	if (argc == 2)	{strcpy(filename, argv[1]);}
	else			{strcpy(filename, "stim.txt");}
	
	int opt = 0;
	
	// parse the cmdline
	while ((opt = getopt(argc, argv, "i:p:")) != -1) {
		switch(opt) {
			case 'i':	strcpy(filename, optarg); break;
			case 'p':	available_pf = (u32) strtol(optarg,NULL,10);
						validate_pf_number(available_pf);
						break;
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
	//call function to allocate memory to PD
	init_PD();
	
	// loop through input file an feed commands to sim
	do {
		get_command(fp);
		if (strcmp(cmd, "-v") == MATCH) {
			printf("VMM Simulator Ver 0.1\n"); 
			exit(-1);
		}
		else if (strcmp(cmd, "w") == MATCH || strcmp(cmd, "r") == MATCH ) {
			parse_addr(addr);
			if ( t_flag == 1 ){
				printf(" %s 0x%08X\n", cmd, addr);
			}
			if (is_PT_present(working_addr.PD_index) == 1) {
			    //check the user page present
				if(is_UP_present(working_addr.PD_index, working_addr.PT_index) == 1)
				{
					total_cycles += 20;               //if the address exist, it considers memory access
					//printf("get here few time\n");
				}
				else  //the PT exists but not the user page
				{
				   if (available_pf == used_pf)       // no pf avail then evict one page
						evict_page();
				   create_user_page(working_addr.PD_index, working_addr.PT_index,cmd);
				}
			}
			else   // new PT and UP need to created
			{	
			    while (available_pf < (used_pf + 2))   // create 2 avail pf
					evict_page();
					
			    create_page_table(working_addr.PD_index, cmd);
			    create_user_page(working_addr.PD_index, working_addr.PT_index, cmd);
			}   
		}
		if (d_flag == 1){dump_vmm();}	
		
	} while (strcmp (cmd, "EOF"));
	
	num_physical_frames = 1 + num_PT + num_UP;
	print_outputs();
	
}


/*	
*	Function: init_PD
*	
*	Description: The function is to allocate memory for PD,
*       which is an array of 1024 items of PF struct.
*		The PDBR is the pointer to the PD
*	
*	Inputs: None
*
*   Outputs: None
*/
void init_PD(){
	
	int i;				//loop counter
	PDBR = (PF_t*) malloc (sizeof (PF_t) *1024);
	used_pf ++;

	for (i=0; i<PF_SIZE; i++) {
	    (PDBR + i)->address  = 0;
		(PDBR + i)->reserved = 0;
		(PDBR + i)->dirty    = 0;
		(PDBR + i)->present  = 0;
	}
}


/*	
*	Function: get_command
*	
*	Description:	Parses input file and return valid commands and 
*					and or address to global variables. validates data
*					before filling globals
*					
*	
*	Inputs: filepointer to infile
*
*   Outputs: None
*/
void get_command(FILE *fp){
	static int first_time = 1;
	static int eof_valid  = 0;
	static int need_num_pages = 0;
	static int need_addr = 0;
	static char token[11];

	static int rc;
	
	rc = fscanf(fp, "%s", token);
	if (rc == EOF && !eof_valid) {
		fprintf(stderr, "invalid EOF in input file; this is fatal\n");
		exit (-1);
	}
	else if (rc == EOF ) {
		//printf("rc = EOF\n");
		cmd = "EOF";
	}
	else if (first_time && strcmp(token, "-p")== MATCH) {  			//strcmp returns 0 if match
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
	else if (strcmp(token, "-v") == MATCH) {
		v_flag = 1;
		cmd = "-v";
	}
	else if (strcmp(token, "-t") == MATCH) {
		t_flag = 1;
		cmd = "-t";
	}
	else if (strcmp(token, "-d") == MATCH) {
		d_flag = 1;
		cmd = "-d";
	}
	else {
		fprintf(stderr, "Invalid instruction sequence: got %s\n", token);
		exit(-1);
	}
			
		
	first_time = 0;
}
/*	
*	Function: validate_pf_number
*	
*	Description: Validates user-supplied spec for number of page frames
*	
*	Inputs: u32 number to validate
*
*   Outputs: exits if invalid
*/
void validate_pf_number(u32 num_pf){
	if (num_pf < MIN_PF || num_pf > MAX_PF) {
		fprintf(stderr, "Invalid number of PF specified\n");
		fprintf(stderr, "Must be between %u and %u ", MIN_PF, MAX_PF);
		fprintf(stderr, "but %u was specified\n", num_pf);
		fprintf(stderr, "this is fatal: quitting\n");
		exit(-1);
	}
}


/*	
*	Function: validate_addr
*	
*	Description: Validates address from input file meets spec
*	
*	Inputs: pointer to token containg address to validate
*
*   Outputs: exits if invalid
*/
void validate_addr(char * token) {
	//printf("validating address\n");
	if ( (strncmp(token, "0x", 2) == MATCH) &&
		 (strlen(token) <= ADDR_STRING_LEN) ) {
			 
		addr = (u32) strtol(token,NULL,16);
	} else {
		fprintf(stderr, "illegal address specified\n");
		fprintf(stderr, "valid address is of the form 0xffffffff\n");
		fprintf(stderr, "%s was specified\n", token);
		exit(-1);
	}
}



/* 
*   Function: is_PT_present
*
*	Description: takes the supplied address for the inputfile
*   and parses it into the working_addr data
*   structure
*
*	Inputs:      32-bit addr from stimulus 
*
*   Outputs:     None
*
*/
	
void parse_addr(u32 addr) {
	working_addr.PD_index  = (addr & PD_MASK) >> (PT_BITS + OS_BITS);
	working_addr.PT_index  = (addr & PT_MASK) >> OS_BITS;
	working_addr.UP_offset = (addr & OS_MASK);
}

/* 
*   Function: is_PT_present
*
*	Description: This function is to check the Page Table(PT) exist
*      by checking the present bit in PD entry
*
*	Inputs:      Index to PD, 
*
*   Outputs:     1 - Exist
*                0 - None exist
*
*/

int is_PT_present(u16 index) 
{
	return ((PDBR + index)->present);
}

/* 
*   Function: is_UP_present
*
*	Description: This function is to check the User Page(UP) exist
*      by checking the present bit in PT entry.
*
*	Inputs:      Index to PD, 
*                Index to PT,
*
*   Outputs:     1 - Exist
*                0 - None exist
*
*/

int is_UP_present(u16 pd_index, u16 pt_index) 
{
	size_t page;
	int    result;
	
	//retrieve pointer to PT from PD entry
	page = (PDBR+pd_index)->address;
	
	//return present bit at PT entry
	result = ((PF_t*)((size_t)page + pt_index*sizeof(PF_t)))->present; 
	return result;
}


/* 
*   Function: create_page_table
*
*	Description: This function is to allocate memory for the new PT,
*      the PT address are stored in associated entry in PD, as well as
*      set up present and dirty bit. 
*
*	Inputs:      Index to PD, 
*                Index to PT,
*                RW cmd
*
*   Outputs:     None
*
*/

void create_page_table(u16 pd_index, const char *rw)
{
 	PF_t* page;
	
	//track number of PT and max PT
	num_PT ++;
	if (num_PT > max_num_PT)  max_num_PT = num_PT;
	
    //allocate memory for new PT
	page =  (PF_t*) malloc(sizeof(PF_t)*1024);
	
	used_pf ++; //tracking number of pf created
	
    //set the PT addr to PD entry, present bit
	(PDBR + pd_index)->address = ((size_t)page);
	(PDBR + pd_index)->present = 1;
	
	if (strcmp (rw, "w") == MATCH){
	   (PDBR + pd_index)->dirty = 1;
	   //printf(" get to dirty bit Pd\n");
    }
	num_PD_entry++;
    //printf ("New Page Table added to PD as entry %d\n", pd_index);   

}

/* 
*   Function: create_user_page
*
*	Description: This function is to set present and dirty bit in 
*      associated entry in PT for new user page. The user page address 
*      does not need to store in the entry because we not use access to
*      the UP.
*
*	Inputs:      Index to PD, 
*                Index to PT
*                RW cmd
*
*   Outputs:     None
*
*/

void create_user_page(u16 pd_index, u16 pt_index, const char *rw)
{
    size_t page;
	
	//track number of UP and max of UP
	num_UP++;                           
	if (num_UP > max_num_UP) max_num_UP = num_UP;
	
	used_pf ++;     //tracking number of used page frame
	
	page = (PDBR + pd_index)->address;  //retrieve the pointer to PT
	
	((PF_t*) (page + pt_index*8))->present = 1 ;
	
	if (strcmp (rw, "w")== MATCH){
		((PF_t*) (page + pt_index*8))->dirty = 1;
		 //printf(" get to dirty bit IN pt \n");
	}	
    
	//this is swap-in operation
	total_cycles += 50000;
	num_swap_in ++;
}

/* 
*   Function: evict_page
*
*	Description: This function is evicts a page. The evicted page is selected randomly
*      with constraint of avoiding swap-out policy.
*
*	Inputs:      None
*
*   Outputs:     None
*
*/

void evict_page()
{
   int evict_dirty_pages[PF_SIZE], evict_clean_pages[PF_SIZE];
	int num_dirty_pages, num_clean_pages;
	int evict_PD_entry;    //indicate which entry in PD is evicted
	int evict_PT_entry;    //indicate which entry in PT is evicted
	size_t page_addr;
	PF_t * tmp, tmp_UP;
	
	
	int i,d_index, c_index;
	
	START:
	num_dirty_pages = 0;
	num_clean_pages = 0;
	d_index = 0;
	c_index = 0;
	evict_PD_entry = 0;
	evict_PT_entry = 0;
	
	
	//printf ("evict page \n");
	
	
	//search in PD for present PT, categoried into dirty and clean
	for (i = 0; i < PF_SIZE; i++)
	{   
	    tmp = (PF_t *) (PDBR + i);
	    if (tmp->present == 1)
		{
			if (tmp->dirty == 1)
			{
		        evict_dirty_pages[d_index] = i;
				d_index ++;
				num_dirty_pages ++;
			}
			else {
			    evict_clean_pages[c_index] = i;
				c_index ++;
                num_clean_pages ++;
            }				
		}
	}  //end PD for loop search
	
	//select the evict pages randomly with no write back priority
	//evict number is an entry in PD
	//srand(1);
	if (num_clean_pages > 0) 
	{
		evict_PD_entry = evict_clean_pages[rand() % num_clean_pages];
	}
	else {
		evict_PD_entry = evict_dirty_pages[rand() % num_dirty_pages];
	}
	
    //retrieve pointer to PT
	page_addr =  ((PDBR+evict_PD_entry)->address);
	
	d_index = 0;
	c_index = 0;
	num_clean_pages = 0;
	num_dirty_pages = 0;
	 
	//loop thr PT to search for UP to evict
	for (i = 0; i < PF_SIZE ; i++)
	{  
	    if ( ((PF_t*)(page_addr+i*sizeof(PF_t)))->present == 1)
		{	
		    if (((PF_t*)(page_addr+i*sizeof(PF_t)))->dirty == 1)
		    {
			   evict_dirty_pages[d_index] = i;
			   d_index++;
		       num_dirty_pages++;
			}
			else 
			{
				evict_clean_pages[c_index] = i;
				c_index ++;
				num_clean_pages ++;
			}				
		}	
	}  //end PT for loop search
		
	//printf("number dirty page %d \n", num_dirty_pages);
	//printf("number clean page %d \n", num_clean_pages);
	
	if (num_clean_pages > 0)                         //evict clean page
	{
		evict_PT_entry = evict_clean_pages[rand() % num_clean_pages];
		((PF_t*)(page_addr + evict_PT_entry*sizeof(PF_t)))-> present = 0;    //clear the present bit in PT
		//printf("move the clean page \n");
		used_pf --;       		                      //track number of frames
		num_UP--;
		num_pure_replace++;
	}
	else if (num_dirty_pages >0)                     //Evict dirty page
	{
		evict_PT_entry = evict_dirty_pages[rand() % num_dirty_pages];
		((PF_t*)(page_addr + evict_PT_entry))-> present = 0;
		((PF_t*)(page_addr + evict_PT_entry))-> dirty   = 0;
		//printf("move the dirty page \n");
		used_pf --;                                  //track number of frames
		total_cycles +=50000;                        //swap-out cycles
		num_UP--;
		num_swap_out ++;
	} 
	else if (num_UP == 0)                            //only evict PT when there is no longer 
	                                                 //have any user page left
	{  //evic its own PT	
		//clear the entry in PD
		((PF_t*) (PD + evict_PD_entry))->present  = 0;
		((PF_t*) (PD + evict_PD_entry))->dirty    = 0;
		//printf ("move the pt\n");
		num_PD_entry--;                               //track number entry in PD
		used_pf --;                                   //track number of frames 
		num_PT --;                                    //PT removed
		num_pure_replace++;                           //remove PT as pure replacement
	}
	else  //keep looping the until all the user pages removed
		goto START;
}

/* 
*   Function: print_outputs
*
*	Description: This function is to print out the outputs
*
*	Inputs:      None
*
*   Outputs:     None
*
*/
void print_outputs()
{
	printf( " Number of PD Entries     : %d \n", num_PD_entry);
	printf( "\n");
	printf( " Number of Page Tables    : %d \n", num_PT);
	printf( " Max Number of Page Tables: %d \n", max_num_PT);
	printf( "\n");
	printf( " Number of User Pages     : %d \n", num_UP);
	printf( " Max Number of User Pages : %d \n", max_num_UP);
	printf( "\n");
	printf( " Number of Physical Frames: %d \n", num_physical_frames);
	printf( "\n");
	printf( " Number of swap-in        : %d \n", num_swap_in);
	printf( " Number of swap-out       : %d \n", num_swap_out);
	printf( " Number of Pure Replacement: %d\n", num_pure_replace);
	printf( "\n");
	printf( " Number of Execution Cycles: %d\n", total_cycles);
}


void dump_vmm() {
	int i,j;
	PF_t * PT_base;
	
	for (i=0; i < PF_SIZE; i++) {
		if ((PDBR + i)->present != 0) {
			printf("=========================================\n");
			printf("  PD Line #%04d", i);
			print_pf(PDBR + i);
			printf("=========================================\n");
			//printf("\n\n\n%d\n\n\n", (PDBR + i)->address);
			PT_base = (PF_t*)((PDBR + i)->address);

			for (j=0; j < PF_SIZE; j++) {
				if ( (PT_base + j)->present != 0 ) {
					printf("\tPT Line #%04d ",j);
					print_pf(PT_base + j);
				}
			}
			printf("\n\n");
		}
	}
	printf("\n\n\n\n\n");
}

/*	Simple print out of a page frame

	Takes a pointer to the PF
	 
*/
void print_pf(PF_t *pf){

	printf("d = %d p = %d\n", pf->dirty, pf->present);


}


