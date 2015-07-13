#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETS  1024
#define LINES 4
#define SET_MASK  0x00007FE0
#define SET_SHIFT 5
#define TAG_MASK  0xFFFF8000
#define TAG_SHIFT 15

#define MATCH 0
#define TRUE  1
#define FALSE 0
typedef struct {
    int valid_bit;
	int dirty_bit;
	int LRU_bits;
	int tag;
} cache_line;

//function prototype
void init_cache();
void cache_search(int, char*);
void add_cache_line(int, int,int, char*);
void replace_cache_line(int,int,char*);

cache_line* cache[SETS][LINES];
char *cmd;
int  addr;
int  addr_need;                 //toggle variable between r/w and addr

//declare timing variable
int total_exec_cycle;


void main()
{   
    FILE *fp;
    
    cache_line* ptr;
	
	char *inst;                 //pointer of string that read from command file
	
    init_cache();
		
    //NEED TO FIGURE OUT THE WAY TO ENTER THE PATH AND NAME FROM STDIN
	fp = fopen ("my_text.txt","r");
	
	//read instruction 
	while (fscanf(fp,"%s", inst) != EOF)
	{
        // the following codes is to parse the command file
    	if ((strcmp(inst, "w") == MATCH) ||      //detect w instruction
		    (strcmp(inst, "W") == MATCH) )
		{	
		    if (addr_need == TRUE) 
			{   
			    printf("There is invalid instruction sequence\n");
				exit(-1);
			}	
			addr_need = TRUE;
			cmd       =  "w";
		}
		//detect Read instruction	
		else if ((strcmp(inst,"r") == MATCH) ||  //detect r instruction
		         (strcmp(inst,"R") == MATCH) )
		{		 
		    if (addr_need == TRUE)             //check for correct cmd sequence
			{   
			    printf("There is invalid instruction sequence\n");
				exit(-1);
			}
			addr_need  = TRUE;
			cmd        = "r";
		}
		else if ((strcmp (inst, "-v") == MATCH) ||  //detect the debug command-v
                 (strcmp (inst, "-V") == MATCH)) 		
		{
		    if (addr_need == TRUE)                 //debug must not occurs
			{
			    printf("The memory address must occurs here \n");
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
			    printf("The memory address must occurs here \n");
				exit(-1);
			}
			printf("Detect debug command T - Not know what to do with it\n"); 
			
		}
		else if ((strcmp (inst, "-d") == MATCH) ||  //detect the debug command-T
                 (strcmp (inst, "-D") == MATCH)) 		
		{
			if (addr_need == TRUE)                  //check for correct cmd sequence
			{   
			    printf("There is invalid instruction sequence\n");
				exit(-1);
			}
			printf("Detect debug command D - define the function later\n"); 
			
		}
		else if (strncmp (inst,"0x",2) == MATCH)    //detect address
		{
		    if (strlen(inst) > 10)                  //address with > 32 bits
			{                                       //print out error and exits
				printf ("The memory address is invalid %s\n", inst);
				exit(-1);
			}
            
			if (addr_need == TRUE)                  //mem address followed the r/w cmd
			{
				addr_need = FALSE;
				addr      = (int) strtol(inst,NULL,16);
				printf (" addr : 0x%x", addr);
				cache_search(addr,cmd);
			}
			else  
			{
				printf("There is invalid instruction sequence \n");
			    exit(-1);
			}
		}
		else 
		{
			printf("There is invalid instruction \n");
			exit (-1);
		}
	}			
	//int h = (int) strtol(str2,NULL,16);
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
	
	
	printf(" Execute the cmd:  %s with addr 0x%x\n", cmd, addr);
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
				if (tmp->tag   == tag)          //got hit
				{
					total_exec_cycle++;
					hit  = TRUE;
					break;
				}	
			}	
		}
		else                               //when this statment executed then
		{                                  //the miss occurs, fill the line of set              
			add_cache_line(i,set,tag,cmd); //with new address.
			hit  = TRUE;
			break;
		}	
    }
	
	if (hit == FALSE)                  //miss and all lines are occupied
	    replace_cache_line(set,tag,cmd);   //call line eviction
	
}

//add a cache line into cache
void add_cache_line(int line, int set, int tag, char * cmd)
{
    cache_line* ptr;
	int i;
	
	printf("add a cache line with set %x and tag %x\n", set, tag);
	
	total_exec_cycle = total_exec_cycle + 51;
	
	ptr 	        = (cache_line *) malloc(sizeof(cache_line));
	ptr->tag        = tag;
	ptr->LRU_bits   = 0;
	ptr->valid_bit  = 1;
	
	if (strcmp(cmd,"w") == MATCH)     //if write then set dirty bit 
		ptr->dirty_bit = 1;
	else
	    ptr->dirty_bit = 0;
	
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
	
	total_exec_cycle = total_exec_cycle + 1;
	
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
	printf("replace line occurs %d with LRU %d\n", i, tmp->LRU_bits); 
	
	//check dirty bit for write back
	//only for timing
	if (tmp->dirty_bit == 1){
		total_exec_cycle = total_exec_cycle + 50;
		printf("write back occurs\n");}
	
	//replace the line with new tag, and update overhead bits
	tmp->tag      = tag;             
	tmp->LRU_bits = 0; 
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


