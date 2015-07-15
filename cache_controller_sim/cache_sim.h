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


// this is a cache line. It includes a pointer to the history of that line
typedef struct {
	int 		valid_bit;
	int 		dirty_bit;
	int 		LRU_bits;
	int			tag;	
	struct hist_node	*head;
} cache_line;

typedef struct {
	cache_line	c_line;
	int			set;
	int			line;
	struct hist_node	*next;
} hist_node;

typedef struct {
	char command;
	u32	 address;
} mem_access;

//function prototype
void init_cache();
void cache_search(int, char*);
void add_cache_line(int, int, int, char*);
void replace_cache_line(int, int, char*);
void show_stats();


#endif