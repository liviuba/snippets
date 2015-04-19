#include <unistd.h>
#include <stddef.h>
#include <string.h>	//TODO: replace memset call w/ an implementation
#include <stdio.h>

/** Map of free/taken memory chunks done as smallest <size> first -sorted linked list (makes more sense than array because of faster inserts in practice; following reference to next block is negligible, since size must be checked anyway, so struct gets cached)
 * best-fit approach
 * Provides the advantages of a linked-list(no "blocks" for heap memory), but stays localized in memory
 * TODO	: find a way to allocate extra chunks for the free_list , so that free list doesn't have a hardcoded limit
 * 			: merge adjacent free chunks
 * */
struct Empty_Chunk{
	size_t size;
	void* block_start;	//pointer to start of free block
	struct Empty_Chunk* next;
	struct Empty_Chunk* prev;
};

//TODO: make a proper estimation on size of the unused chunks vector(maybe based on process' heap hard-limit?) [Based on cache line size]
const unsigned MAX_CHUNK_NUMBER = 256;
unsigned ACTUAL_ALLOC_SIZE = 8;	//doesn't always call sbrk(); asks for 2^ACTUAL_ALLOC_SIZE bytes at once
struct Empty_Chunk* empty_chunk_list = NULL;
struct Empty_Chunk* empty_chunk_vector = NULL;
const int DEBUG = 1;
/**FIXME [hotfix works, but do something proper] 
 * First condition in like_malloc is supposed to act as a "constructor"
 *  free_list also becomes NULL when program runs out of allocated big block
 *  supposed to get a new big block w/ sbrk (only time when it's called), not initialize everything
 *  */
unsigned FREE_LIST_EMPTIED_OUT_HOTFIX = 0;

void* like_malloc(size_t size){
	struct Empty_Chunk* free_spot_finder = NULL;
	size_t* allocated_block;
	struct Empty_Chunk* i;
	int j;
	
	//first call, initialize everything
	if(empty_chunk_list == NULL && FREE_LIST_EMPTIED_OUT_HOTFIX==0){
		FREE_LIST_EMPTIED_OUT_HOTFIX = 1;

		if(DEBUG == 1) printf("Before any alloc : %p\n",sbrk(0));
		empty_chunk_vector = sbrk( MAX_CHUNK_NUMBER * sizeof(struct Empty_Chunk) );	//get memory for the list of free blocks
		empty_chunk_list = empty_chunk_vector;
		memset( empty_chunk_list, 0, MAX_CHUNK_NUMBER * sizeof(struct Empty_Chunk) );	//hotfix to mark free list items
		if(DEBUG == 1)printf("After putting chunk list on heap:%p\n",sbrk(0));

		//init list element for the first free block
		empty_chunk_list->size = 1 << ACTUAL_ALLOC_SIZE++;
		empty_chunk_list->block_start = sbrk( empty_chunk_list->size );	//get heap for first big block
		if(DEBUG == 1)printf("After allocating first big block :%p\n",sbrk(0));
		empty_chunk_list->next = NULL;
		empty_chunk_list->prev = NULL;
	}

	//find a free spot; one size_t as overhead, because free() only gets passed a pointer
	free_spot_finder = empty_chunk_list;
	while( free_spot_finder && (free_spot_finder->size + sizeof(size_t) < size) )
		free_spot_finder = free_spot_finder->next;

	//found a suitable free chunk
	if( free_spot_finder != NULL ){
		if(DEBUG == 1) printf("Found a suitable sized block of size %d\n",free_spot_finder->size);
		allocated_block = free_spot_finder->block_start;
		*allocated_block = size;	//add size as header of block
		allocated_block++;

		//restore list; if request fit perfectly, mark block unused; otherwise reuse current block to store remaining free space
		if(free_spot_finder->prev != NULL)
			free_spot_finder->prev->next = free_spot_finder->next;
		//free_spot_finder == null, so chunk is first
		else{
/*** FIXME
 * Something weird happens here; empty_chunk_list->prev becomes some weird value (and keeps being that weird value after null assignemnt
 * reproduce in gdb with main as is; found w/ 'watch empty_chunk_list->prev!=0'
 */
			empty_chunk_list = empty_chunk_list->next;
//			empty_chunk_list->prev = NULL;
		}
		
		//refresh info on free block; size 0 also marks unused slot in free list "vector"
		free_spot_finder->size -= (sizeof(size_t) + size);
		//if block didn't fit perfectly, reinsert leftover free space in list, keeping it sorted
		if(free_spot_finder->size){
			free_spot_finder->block_start += (sizeof(size_t) + size);	//"remove" used part of chunk
			for(i=empty_chunk_list; i && i->next!=NULL && i->size < free_spot_finder->size; i++);
			//empty_chunk list is either NULL, or something
			if( i==empty_chunk_list ){
				if(i==NULL){
					empty_chunk_list = free_spot_finder;
					empty_chunk_list->next = empty_chunk_list->prev = NULL;
				}
			}
			else{
				//insert before i (the first chunk larger than free_spot_finder OR end of list)
				if( i->next == NULL || i->size < free_spot_finder->size){//add at end of list
					i->next = free_spot_finder;
					free_spot_finder->prev = i;
					free_spot_finder->next = NULL;
				}
				else{//add somewhere in list (inluding head)
					i->prev->next = free_spot_finder;
					free_spot_finder->prev = i->prev;
					free_spot_finder->next = i;
					i->prev = free_spot_finder;
				}
			}
		}
	}
	/*** Didn't find a suitable sized free chunk; allocate big blocks until request gets filled **/
	else{
		//TODO: Is it worth the overhead appending to last chunk(if free?)
		if(DEBUG == 1) printf("Didn't find a suitable sized block\n");
		//get smallest 2^k big block that fits request
		while( (1<<ACTUAL_ALLOC_SIZE) < size ) ACTUAL_ALLOC_SIZE++;
		allocated_block = sbrk( 1<<ACTUAL_ALLOC_SIZE++ );
		*allocated_block = size;
		allocated_block++;

		//find first unused empty_chunk in the list, have it store the space space
		//TODO: Add some data structure to pop from empty_chunk_vector free spots for free_list in O(1)
		for(j=0; j< MAX_CHUNK_NUMBER && empty_chunk_vector[j].size; j++);
		empty_chunk_vector[j].size = (1<<(ACTUAL_ALLOC_SIZE-1)) - (size + sizeof(size_t));	//ACTUAL_ALLOC_SIZE has been previously incremented
		empty_chunk_vector[j].block_start = allocated_block + size;//FIXME +1?
		/** Insert into sorted empty chunk list **/
		for(i=empty_chunk_list; i && i->next != NULL && i->size < empty_chunk_vector[j].size; i = i->next); //find proper spot 

		//if list is empty
		if(i==NULL){
			empty_chunk_list = &empty_chunk_vector[j];
			empty_chunk_list->next = empty_chunk_list->prev = NULL;
		}
		else{
			//insert after i
			if( i->next == NULL && i->size < empty_chunk_vector[j].size){
				i->next = &empty_chunk_vector[j];
				empty_chunk_vector[j].prev = i;
				empty_chunk_vector[j].next = NULL;
			}
			//add somewhere in list (inluding head)
			else{
				i->prev->next = &empty_chunk_vector[j];
				empty_chunk_vector[j].prev = i->prev;
				empty_chunk_vector[j].next = i;
				i->prev = &empty_chunk_vector[j];
			}
		}
	}
	return allocated_block;
}

int main(){
	const unsigned BIG_VECTOR_SIZE = 256;	//less than 256B in allocated big block 
	const unsigned BIG_SERIAL_SIZE = 100;	//less than 512B in allocated big block
	const unsigned EXTRA_LARGE_WITH_HOT_SAUCE = 50;	//will do x3, so it will certainly empty out the allocated block
	int* a;
	int* a_vec;
	int i;
	int j_debug = 0;
	int test_ok;
	
	//pray it works
	a = (int*) like_malloc(sizeof(int));
	*a = 100;
	printf("First 'a' : %d\n\n",*a);

	//request a big chunk, forcing to allocate a new big block
	a_vec = like_malloc( BIG_VECTOR_SIZE * sizeof(int) );
	for( i=0; i<BIG_VECTOR_SIZE; i++)
		a_vec[i] = i;
	printf("Got space for a_vec\n\n");

	a = (int*) like_malloc(sizeof(int));
	*a = 10;
	printf("Second 'a' : %d\n\n",*a);
	//makes chunk list run out
	for(i=0;i<BIG_SERIAL_SIZE;i++){
		a = (int*) like_malloc(sizeof(int));
		*a = i;
	}
	for(i=0, test_ok=1; i<BIG_SERIAL_SIZE;i++){
		if(a[i] != i)
			test_ok = 0;
	}
	test_ok?printf("Everything went ok with BIG_SERIAL_SIZE alloc\n\n"):printf("Something went wrong w/ BIG_SERIAL_SIZE_ALLOC\n\n");
	//FIXME : There's something weird happening, allocating unneded blocks; look into it
	//
	//repeatedly request small chunks, w/ available_space % chunk_size != 0		(so that there's some spare little bit)
	printf("\n\n\n\n----------------------------------------------------\n\n\n");
	for(i=0; i<EXTRA_LARGE_WITH_HOT_SAUCE; i++){
		if( i == EXTRA_LARGE_WITH_HOT_SAUCE-10)	j_debug=1;
		a = (int*) like_malloc(7* sizeof(int));
		*a = i;
//		printf("%d'th a : %d\n\n",i,*a);
	}

	//request something in the small chunk leftover from before
	a = like_malloc(sizeof(char));
	*a = 4;
	printf("Reusing small leftover bit: %d\n",*a);

	return 0;
}
