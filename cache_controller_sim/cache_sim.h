#ifndef MAIN_HEADER
#define MAIN_HEADER

#define SETS  1024
#define LINES 4
#define SET_MASK  0x00007FE0
#define SET_SHIFT 5
#define TAG_MASK  0xFFFF8000
#define TAG_SHIFT 15

#define MATCH 0
#define TRUE  1
#define FALSE 0

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
#ifdef __HAS_LONG_LONG__
typedef unsigned long long 	u64;
#endif



typedef struct cache_line cache_line;
typedef struct hist_node hist_node;

struct hist_node{
	int 		valid_bit;
	int 		dirty_bit;
	int 		LRU_bits;
	int			tag;
	char		cmd;
	hist_node	*next;
};

struct cache_line{
	int 		valid_bit;
	int 		dirty_bit;
	int 		LRU_bits;
	int			tag;	
	hist_node	*head;
};


/*
typedef struct {
	char command;
	u32	 address;
} mem_access;
*/

//function prototype
void init_cache();
void cache_search(int, char*);
void add_cache_line(int, int, int, char*);
void replace_cache_line(int, int, char*);
void show_stats();
void show_dump();
void add_history_node(cache_line*, char cmd);
void show_hist(int, int);


#endif