/*
	ECE 587
	Portland State University Summer 2015
	Project 1: Cache simulator
	
	Jeff Nguyen <jqn@pdx.edu>
	Paul Long	<paul@thelongs.ws>
	
	Simulates a blocking L1 data cache for a 32bit byte-addressable system with:
		- cyclical distribution
		- 1k sets
		- 4-way (set associative)
		- 4-byte integer words
		- 32-bit cache line (and memory paragraph) 
		- LRU replacement policy
		- allocate on write
		- memory accesses assumed to be 50 cycles
		
	Default input file is to be named "my_text.txt" this is used if no input file
	is specified on the commandline. Default my be overridden by supplying an
	input file on the commandline.
	
	Default output is directed to stdout, redirect on commanline if desired.
	
*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache_sim.h"





cache_line *cache[SETS][LINES];
char *cmd;
int  addr;
int  addr_need;                 //toggle variable between r/w and addr


// some flags that might get set in response to debug instructions in the input stream
u8	d_flag = 0;
u8  t_flag = 0;
	

// statistics counters
int reads, writes, stream_ins, stream_outs, hits, misses,\
	read_hits, write_hits, cycles;


void main(int argc, char *argv[])
{   
    FILE *fp;
    
    cache_line *ptr;
	
	char inst[15];				// string read from command file
	char filename[100];			// command file's filename
	
    init_cache();
	
	
		
    // if filename given on commandline use it otherwise use default filename
	if (argc == 2)	{strcpy(filename, argv[1]);}
	else			{strcpy(filename, "my_text.txt");}
	
	fp = fopen (filename,"r");
	if (NULL == fp){
		fprintf (stderr, "Failed to open input file. This is fatal!\n");
		exit;
	}
	

	//read instruction 
	while (fscanf(fp,"%s", inst) != EOF)
	{
		//printf("in while loop\n");
        // the following codes is to parse the command file
    	if ((strcmp(inst, "w") == MATCH) ||      //detect w instruction
		    (strcmp(inst, "W") == MATCH) )
		{
		    if (addr_need == TRUE) 
			{   
			    fprintf(stderr, "Invalid instruction sequence read from input file:");
				fprintf(stderr, "expected address, got %s\n", inst);
				exit(-1);
			}	
			addr_need = TRUE;
			writes++;							// update stats
			cmd       =  "w";
		}
		//detect Read instruction	
		else if ((strcmp(inst,"r") == MATCH) ||  //detect r instruction
		         (strcmp(inst,"R") == MATCH) )
		{		 
		    if (addr_need == TRUE)             //check for correct cmd sequence
			{   
			    fprintf(stderr, "Invalid instruction sequence read from input file:");
				fprintf(stderr, "expected address, got %s\n", inst);
				exit(-1);
			}
			addr_need  = TRUE;
			reads++;							// update stats
			cmd        = "r";
		}
		else if ((strcmp (inst, "-v") == MATCH) ||  //detect the debug command-v
                 (strcmp (inst, "-V") == MATCH)) 		
		{
		    if (addr_need == TRUE)                 //debug must not occurs
			{
			    fprintf(stderr, "Invalid instruction sequence read from input file:");
				fprintf(stderr, "expected address, got %s\n", inst);
				exit(-1);
			}
			printf("Cache Simulator Ver 0.1\n"); 
			exit(-1);
		}
		else if ((strcmp (inst, "-t") == MATCH) ||  //detect the debug command-T
                 (strcmp (inst, "-T") == MATCH)) 		
		{
			if (addr_need == TRUE)                 //debug must not occurs
			{
			    fprintf(stderr, "Invalid instruction sequence read from input file:");
				fprintf(stderr, "expected address, got %s\n", inst);
				exit(-1);
			}
			//printf("Detect debug command T - Not know what to do with it\n"); 
			if (!t_flag) {t_flag = 1;}
		}
		else if ((strcmp (inst, "-d") == MATCH) ||  //detect the debug command-T
                 (strcmp (inst, "-D") == MATCH)) 		
		{
			if (addr_need == TRUE)                  //check for correct cmd sequence
			{   
			    fprintf(stderr, "Invalid instruction sequence read from input file:");
				fprintf(stderr, "expected address, got %s\n", inst);
				exit(-1);
			}
			// conditional check not really needed, we could just set it
			// but spec said "ignore if already set" so putting this in
			if (!d_flag) {d_flag = 1;}
			
		}
		else if (strncmp (inst,"0x",2) == MATCH)    //detect address
		{
		    if (strlen(inst) > 10)                  //address with > 32 bits
			{                                       //print out error and exits
				fprintf (stderr, "The memory address is invalid: got %s\n", inst);
				exit(-1);
			}
            
			if (addr_need == TRUE)                  //mem address followed the r/w cmd
			{
				addr_need = FALSE;
				addr      = (int) strtol(inst,NULL,16);
				if(t_flag) {fprintf (stdout, "%s 0x%x\n", cmd, addr);}
				cache_search(addr,cmd);
			}
			else  
			{
				fprintf(stderr, "Invalid instruction sequence: got %s\n", inst);
			    exit(-1);
			}
		}
		
	}		
	
	//int h = (int) strtol(str2,NULL,16);
	
	show_stats(filename);
	if (d_flag){show_dump();}
	free_mem();
	fclose(fp);
	
	// TODO: input file parsing is slightly broken: it allows extra characters
}

//the function is to initial cache to NULL
void init_cache ()
{   
	int i, j;
	for (i = 0; i < SETS; i++)
	    for (j = 0; j < LINES; j++)
			cache[i][j] = NULL;
}

//the function to search for hit/miss
void cache_search( int address, char *cmd)
{
    unsigned int tag, set, i, j, hit;
    cache_line *tmp, * tmp_LRU;	
	
	set = (address & SET_MASK) >> SET_SHIFT;
	tag = (address & TAG_MASK) >> TAG_SHIFT;
	hit = FALSE;
	
	//search for hit or miss
	for (i = 0; i < LINES; i++)
	{   
		tmp  =  cache[set][i];
		if (tmp != NULL) 
		{
		    
		    if (tmp->valid_bit == TRUE)  
			{   //hit cases
				if (tmp->tag   == tag)		
				{
					cycles++;
					hits++;
					if ((strcmp(cmd, "w") == MATCH)){write_hits++;}
					else 							{read_hits++;}
					hit  = TRUE;
					
					//update LRU
					for (j = 0; j < LINES; j++) {
						if (i != j){                                // avoid the hit line in set
							tmp_LRU = cache[set][j];
							
						if ((tmp_LRU != NULL) &&
							(tmp_LRU->LRU_bits < tmp->LRU_bits) )   // incr only LRUbits of the lines that
							{                                       // is smaller then hit line LRU.
								tmp_LRU->LRU_bits++; 
								add_history_node(tmp_LRU, *cmd);
							}
						}
					}
					tmp->LRU_bits = 0;  // change LRU in hit line to zero
					if (cmd == "w") tmp->dirty_bit = 1;
					add_history_node(tmp, *cmd);
					break;
				}	
			}	
		}
		else								// when this statment executed then
		{									// the miss occurs, fill the line of set              
			add_cache_line(i,set,tag,cmd);	// with new address.
			hit  = TRUE;
			break;
		}	
    }
	
	if (hit == FALSE){						// miss and all lines are occupied
		misses++;
		replace_cache_line(set,tag,cmd);	// call line eviction
	}
}

//add a cache line into cache
void add_cache_line(int line, int set, int tag, char * cmd)
{
    cache_line *ptr;
	int i;
	
	// adding a cache line so we must do a stream-in operation
	// update statistics
	cycles += MEM_ACC_CYCLE+1;
	stream_ins ++;
	misses ++;
	
	ptr 	        = (cache_line *) malloc(sizeof(cache_line));
	ptr->tag        = tag;
	ptr->LRU_bits   = 0;
	ptr->valid_bit  = 1;
	ptr->head		= NULL;
	
	//if write then set dirty bit
	if (strcmp(cmd,"w") == MATCH)	{ptr->dirty_bit = 1;}
	else							{ptr->dirty_bit = 0;}
	
	// record history and save to cache
	add_history_node(ptr, *cmd);
	cache[set][line]= ptr;
	
	//inc other lines LRU
	for ( i= 0; i < line; i++)
	{
		ptr =  cache[set][i];
		ptr->LRU_bits++;
		add_history_node(ptr, *cmd);
	}
}

//replace a line in cache
void replace_cache_line (int set, int tag, char *cmd)
{    int i, line_evict;
     cache_line * tmp;
	
	cycles += 1;
	
    //search for the LRU bit 
	for ( i= 0; i < LINES; i++)
	{
		tmp  =  cache[set][i];
		if (tmp->LRU_bits == 3)
		{
		    line_evict = i;
			break;
		}	
	}
	
	//check dirty bit for write back
	//only for timing
	if (tmp->dirty_bit == 1){
		// must do a stream-out operation, update stats
		cycles += MEM_ACC_CYCLE;
		stream_outs ++;
	}
	
	//replace the line with new tag, and update overhead bits
	tmp->tag      = tag;             
	tmp->LRU_bits = 0; 
	
	// must do a stream-in operation, update stats
	cycles += MEM_ACC_CYCLE;
	stream_ins ++;
	
	//set dirty bit in case of 'w'
	if (strcmp(cmd,"w") == MATCH)    
		tmp->dirty_bit = 1;
	else
	    tmp->dirty_bit = 0;
	
	// record history and save to cache
	add_history_node(tmp, *cmd);
	cache[set][i] = tmp;
	
	//inc other lines LRU
	for ( i= 0; i < LINES; i++)
	{
		tmp  =  cache[set][i];
		if (i != line_evict)
		{
		    tmp->LRU_bits++;
			add_history_node(tmp, *cmd);
		}	
	}
}

void free_mem(){
	int i,j;
	hist_node *current, *head;
	
	for (i=0; i<SETS; i++){
		for (j=0; j<LINES; j++){
			if (NULL == cache[i][j]){
				free(cache[i][j]);
			}else{
				head = cache[i][j]->head;			// http://stackoverflow.com/questions/7025328
				while ((current = head) != NULL) {	// set curr to head, stop if list empty.
					head = head->next;				// advance head to next element.
					free (current);					// delete saved pointer.
				}
			free(cache[i][j]);
			}
		}
	}
}

void show_dump() {
	int i, j;
	
	for (i=0; i<SETS; i++) {
		if (NULL == cache[i][0]) {
			printf("Set %4d is untouched\n", i);
		} else {
			printf("\nSet %4d has the following history:\n", i);
			for (j=0; j<LINES; j++) {
				if (NULL != cache[i][j]){
					printf("  LINE #%2d:\n", j);
					show_hist(i,j);
				}else{
					printf("  LINE #%2d is untouched\n",j);
				}
			}
			printf("\n");
		}
	}
}

void show_hist(int set, int line) {
	hist_node *head;
	cache_line* current_line;
	head = cache[set][line]->head;
	current_line = cache[set][line];
	//to print out the current states of the cache line and its history
	
	while (NULL != head) {
		printf("History State: \tV: %d\tD: %d\tLRU: %d\tTag: %d\tCMD: %c\n",\
				head->valid_bit,\
				head->dirty_bit,\
				head->LRU_bits,\
				head->tag,\
				head->cmd);
				// should figure out how to show the cmd and address
		// go to next record in llist
		head = head->next;
	}
	printf("\n");
}
	
void show_stats(char *filename) {
	printf("\n\n\t\t=================================================\n");
	printf("\t\t| Final Simulation Statistics       \t\t|\n");
	printf("\t\t| Input File: %22s\t|\n", filename);
	printf("\t\t=================================================\n");
	printf("\t\t|    Total Mem Accesses:\t%10d\t|\n", reads + writes);
	printf("\t\t|    Reads:\t\t\t%10d\t|\n", reads);
	printf("\t\t|    Writes:\t\t\t%10d\t|\n", writes);
	printf("\t\t|    Stream-In Operations:\t%10d\t|\n", stream_ins);
	printf("\t\t|    Stream-Out Operations:\t%10d\t|\n", stream_outs);
	printf("\t\t|    Cache Hits:\t\t%10d\t|\n", hits);
	printf("\t\t|    Cache Misses:\t\t%10d\t|\n", misses);
	printf("\t\t|    Read Hits:\t\t\t%10d\t|\n", read_hits);
	printf("\t\t|    Write Hits:\t\t%10d\t|\n", write_hits);
	printf("\t\t|    Total Cycles with  Cache:\t%10d\t|\n", cycles);
	printf("\t\t|    Total Cycles if no Cache:\t%10d\t|\n", (reads+writes)*50);
	printf("\t\t=================================================\n\n");
}

void add_history_node (cache_line *c_line, char cmd) {
	hist_node *node;
	
	// create a new hist node and populate with current data
	node			= (hist_node *)malloc(sizeof(hist_node));
	node->valid_bit = c_line->valid_bit;
	node->dirty_bit	= c_line->dirty_bit;
	node->LRU_bits  = c_line->LRU_bits;
	node->tag       = c_line->tag;
	node->cmd		= cmd;
	
	// update head/next pointers to put this node at beginning of list
	// i.e. LIFO order for history linked list
	if (NULL == c_line->head) {
		c_line->head	= node;
		node->next		= NULL;
	} else {
		node->next		= c_line->head;
		c_line->head	= node;
	}		
}