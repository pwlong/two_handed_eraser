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

PF_t * PTBR;
u32  used_pf   = 0;                         // tracking numbers of pf used


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
	//call function to allocate memory to PD
	init_PD();
	
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
			printf("working address\n");
			printf("\tPD %d\n", working_addr.PD_index );
			printf("\tPT %d\n", working_addr.PT_index );
			printf("\tOS %d\n", working_addr.UP_offset);
			
			if (is_PT_present(working_addr.PD_index) == 1) {
			    //check the user page present
				if(is_UP_present(working_addr.PD_index, working_addr.PT_index) == 1)
				{
					//Add cycle count
				}
				else  //the PT exist but not the user page
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
			
		
	} while (strcmp (cmd, "EOF"));
	//printf("Caught EOF, we are done here. G'night\n");
		
	
}


/*	
*	Function: init_PD
*	
*	Description: The function is to allocate memory for PD,
*       which is an array of 1024 items of PF struct.
*		The PTBR is the pointer to the PD
*	
*	Inputs: None
*
*   Outputs: None
*/
void init_PD(){
	
	int i;				//loop counter
	PTBR = (PF_t*) malloc (sizeof (PF_t) *1024);
	used_pf ++;

	for (i=0; i<PF_SIZE; i++) {
	    (PTBR + i)->address  = 0;
		(PTBR + i)->reserved = 0;
		(PTBR + i)->dirty    = 0;
		(PTBR + i)->present  = 0;
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
	return ((PTBR + index)->present);
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
	page = (PTBR+pd_index)->address;
	
	//return present bit at PT entry
	result = ((PF_t*)((size_t)page + pt_index))->present; 
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
    //allocate memory for new PT
	page =  (PF_t*) malloc(sizeof(PF_t)*1024);
	
	used_pf ++; //tracking number of pf created
	
    //set the PT addr to PD entry, present bit
	(PTBR + pd_index)->address = ((size_t)page);
	(PTBR + pd_index)->present = 1;
	
	if (strcmp (rw, "w") == MATCH)
	   (PTBR + pd_index)->dirty = 1;
   
    printf ("New Page Table added to PD as entry %d\n", pd_index);   
    //CALL swapin;
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
	
	page = (PTBR + pd_index)->address;  //retrieve the pointer to PT
	
	((PF_t*) (page + pt_index))->present = 1 ;
	
	if (strcmp (rw, "w")== MATCH){
		((PF_t*) (page + pt_index))->dirty = 1;
	}	
    used_pf ++;     //tracking number of used pf
	
	printf("Add user page at the entry %d of PT at %lx\n", pt_index, page);
	//CALL swap in
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
	PF_t * tmp;
	
	
	int i,d_index, c_index;
	num_dirty_pages = 0;
	num_clean_pages = 0;
	d_index = 0;
	c_index = 0;
	
	
	printf ("evict page \n");
	
	
	//search in PD for present PT, categoried into dirty and clean
	for (i = 0; i < PF_SIZE; i++)
	{   
	    tmp = (PF_t *) (PTBR + i);
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
	srand(1);
	if (num_clean_pages > 0) 
	{
		evict_PD_entry = evict_clean_pages[rand() % num_clean_pages];
	}
	else {
		evict_PD_entry = evict_dirty_pages[rand() % num_dirty_pages];
	}
	
	printf("number of clean page %d\n", num_clean_pages);
	printf("number of dirty page %d\n", num_dirty_pages);
	printf("the PT entry to evict %d\n",evict_PD_entry);
	printf("number of frame used before evict %d\n", used_pf);
	
	
    //retrieve pointer to PT
	page_addr =  ((PTBR+evict_PD_entry)->address);
	d_index = 0;
	c_index = 0;
	num_clean_pages = 0;
	num_dirty_pages = 0;
	 
	//loop thr PT to search for UP to evict
	for (i = 0; i < PF_SIZE ; i++)
	{
	    if ( ((PF_t*)(page_addr+i))->present == 1)
		{		
		   evict_clean_pages[c_index] = i;
		   c_index++;
		   num_clean_pages++;
		}
		else 
		{
		    evict_clean_pages[c_index] = i;
			c_index ++;
            num_clean_pages ++;
        }				
	}  //end PT for loop search
		   
	if (num_clean_pages > 0)                         //evict clean page
	{
		evict_PT_entry = evict_clean_pages[rand() % num_clean_pages];
		((PF_t*)page_addr + evict_PT_entry)-> present = 0;    //clear the present bit
		used_pf --;                                  //set number user PF
	}
	else if (num_dirty_pages >0)                     //Evict dirty page
	{
		evict_PT_entry = evict_dirty_pages[rand() % num_dirty_pages];
		((PF_t*)page_addr + evict_PT_entry)-> present = 0;
		((PF_t*)page_addr + evict_PT_entry)-> dirty   = 0;
		used_pf --;
		//CALL swap-out ; 
	} 
	else {  //evic its own PT	
		//clear the entry in PD
		((PF_t*) (PD + evict_PD_entry))->present  = 0;
		((PF_t*) (PD + evict_PD_entry))->dirty    = 0;
		used_pf --;
	}
	printf("number of frame used before evict %d\n", used_pf);
}