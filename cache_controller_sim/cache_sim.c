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
	FILE *trace_file_handle;
    
    cache_line *ptr;
	
	char inst[15];				//string read from command file
	char filename[100];			//command file's filename
	
	
	
	
	
    init_cache();
	
	
		
    // if filename given on commandline use it otherwise use default filename
	if (argc == 2)	{strcpy(filename, argv[1]);}
	else			{strcpy(filename, "my_text.txt");}
	
	fp = fopen (filename,"r");
	if (NULL == fp){
		fprintf (stderr, "Failed to open input file. This is fatal!\n");
		exit(-1);
	}
	
	// open a file for recording a tracefile
	trace_file_handle = fopen("trace_file.log", "w");
	if (NULL == trace_file_handle) {
		fprintf (stderr, "Failed to open command history file for writing\n");
		exit(-1);
	}

	
	//read instruction 
	while (fscanf(fp,"%s", inst) != EOF)
	{

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
			//printf("Detect debug command D - define the function later\n"); 
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
	
	show_stats();
	// TODO: Close the files we opened PWL
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
    unsigned int tag, set, i, hit;
    cache_line *tmp;	
	
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
			{
				if (tmp->tag   == tag)		//got hit
				{
					cycles++;
					hits++;
					if ((strcmp(cmd, "w") == MATCH)){write_hits++;}
					else 							 {read_hits++;}
					hit  = TRUE;
					// update LRU
					// update history
					// update stats
					break;
				}	
			}	
		}
		else								//when this statment executed then
		{									//the miss occurs, fill the line of set              
			add_cache_line(i,set,tag,cmd);	//with new address.
			hit  = TRUE;
			break;
		}	
    }
	
	if (hit == FALSE){						//miss and all lines are occupied
		misses++;
		//if ('w' == cmd) {write_misses++;}
		//else			{read_misses++;}
	    replace_cache_line(set,tag,cmd);	//call line eviction
	}
}

//add a cache line into cache
void add_cache_line(int line, int set, int tag, char * cmd)
{
    cache_line *ptr;
	int i;
	
	//printf("add a cache line with set %x and tag %x\n", set, tag);
	
	// adding a cache line so we must do a stream-in operation
	// update statistics
	cycles =+ 51;
	stream_ins ++;
	misses ++;
	
	
	ptr 	        = (cache_line *) malloc(sizeof(cache_line));
	ptr->tag        = tag;
	ptr->LRU_bits   = 0;
	ptr->valid_bit  = 1;
	
	//if write then set dirty bit
	if (strcmp(cmd,"w") == MATCH)	{ptr->dirty_bit = 1;}
	else							{ptr->dirty_bit = 0;}
	
	cache[set][line]= ptr;
	
	//inc other lines LRU
	for ( i= 0; i < line; i++)
	{
		ptr =  cache[set][i];
		ptr->LRU_bits++;
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
	printf("!!!EVICTION FROM CACHE!!!replace line occurs %d with LRU %d\n", i, tmp->LRU_bits); 
	
	//check dirty bit for write back
	//only for timing
	if (tmp->dirty_bit == 1){
		// must do a stream-out operation, update stats
		//printf("write back occurs\n");
		cycles += 50;
		stream_outs ++;
	}
	
	//replace the line with new tag, and update overhead bits
	
	tmp->tag      = tag;             
	tmp->LRU_bits = 0; 
	
	// must do a stream-in operation, update stats
	cycles += 50;
	stream_ins ++;
	
	if (strcmp(cmd,"w") == MATCH)    //set dirty bit in case of 'w'
		tmp->dirty_bit = 1;
	else
	    tmp->dirty_bit = 0;
	cache[set][i] = tmp;
	
	//inc other lines LRU
	for ( i= 0; i < LINES; i++)
	{
		tmp  =  cache[set][i];
		if (i != line_evict)
		{
		    tmp->LRU_bits++;
		}	
	}
}



// saves the command history to log file
int log_command(FILE *fh, char *cmd, int addr)
{
	int rc = 0;
	
	rc = fprintf(fh, "%s 0x%x", cmd, addr);
	if (0 > rc) {return 1;}
	else		{return 0;}	
}

void show_stats()
{
	printf("\n\n\t\t=========================================\n");
	printf("\t\t|     Final Simulation Statistics       |\n");
	printf("\t\t=========================================\n");
	printf("\t\t|  Total Mem Accesses:\t\t%d\t|\n", reads + writes);
	printf("\t\t|  Reads:\t\t\t%d\t|\n", reads);
	printf("\t\t|  Writes:\t\t\t%d\t|\n", writes);
	printf("\t\t|  Stream-In Operations:\t%d\t|\n", stream_ins);
	printf("\t\t|  Stream-Out Operations:\t%d\t|\n", stream_outs);
	printf("\t\t|  Cache Hits:\t\t\t%d\t|\n", hits);
	printf("\t\t|  Cache Misses:\t\t%d\t|\n", misses);
	printf("\t\t|  Read Hits:\t\t\t%d\t|\n", read_hits);
	printf("\t\t|  Write Hits:\t\t\t%d\t|\n", write_hits);
	printf("\t\t|  Total Cycles with  Cache:\t%d\t|\n", cycles);
	printf("\t\t|  Total Cycles if no Cache:\t%d\t|\n", (reads+writes)*50);
	printf("\t\t=========================================\n\n");
}
