/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))


/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    int lruCounter;
    int numHits;
    int numMisses;
    int numWB;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    cache.lruCounter = 0;
    cache.numHits = 0;
    cache.numMisses = 0;
    cache.numWB = 0;

    for (int i = 0; i < cache.numSets * cache.blocksPerSet; i++) {
        cache.blocks[i].tag = -1;
        cache.blocks[i].dirty = 0;
        cache.blocks[i].lruLabel = -1;
        for (int j = 0; j < cache.blockSize; j++) {
            cache.blocks[i].data[j] = 0; // Initialize data to zero
        }
    }

    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data)
{
    int blockIndex = (addr / cache.blockSize) % cache.numSets;
    int tag = addr / (cache.blockSize * cache.numSets);
    int offset = addr % cache.blockSize;
    int setBase = blockIndex * cache.blocksPerSet;
    int baseAddrOfBlock;

    blockStruct *set = &cache.blocks[setBase];
    int lruIndex = 0; // Index of the least recently used block
    int freeIndex = -1; // Index of a free block

    // Search for the tag in the cache
    for (int i = 0; i < cache.blocksPerSet; ++i) {
        if (set[i].tag == tag && set[i].lruLabel != -1) { // Cache hit
            cache.numHits++;
            set[i].lruLabel = ++cache.lruCounter; // Update LRU counter
            if (!write_flag) {
                printAction(addr, 1, cacheToProcessor);
                return set[i].data[offset]; // Returning the data for read
            } else {
                set[i].data[offset] = write_data; // Write data
                set[i].dirty = 1; // Mark block as dirty
                printAction(addr, 1, processorToCache);
                return 0; // Undefined return for write operations
            }
        }
        // Track the least recently used block and any free block
        if (set[i].lruLabel == -1 && freeIndex == -1) {
            freeIndex = i; // Found a free block
        } else if (set[lruIndex].lruLabel > set[i].lruLabel) {
            lruIndex = i; // Found a better LRU block
        }
    }

    // Cache miss
    cache.numMisses++;
    int targetIndex = (freeIndex != -1) ? freeIndex : lruIndex;

    // If the block is dirty and is being replaced, write it back to memory
    if (set[targetIndex].dirty) {
        cache.numWB++;
        int writeBackAddr = (set[targetIndex].tag * cache.numSets + blockIndex) * cache.blockSize;
        for (int i = 0; i < cache.blockSize; i++) {
            mem_access(writeBackAddr + i, 1, set[targetIndex].data[i]);
        }
        printAction(writeBackAddr, cache.blockSize, cacheToMemory);
    }
    else {
        // If not dirty and being evicted, it goes to nowhere
        if (set[targetIndex].tag != -1) { // Check if the block was previously used
            baseAddrOfBlock = (set[targetIndex].tag * cache.numSets + blockIndex) * cache.blockSize;
            printAction(baseAddrOfBlock, cache.blockSize, cacheToNowhere);
        }
    }

    // Load the block from memory
    for (int i = 0; i < cache.blockSize; i++) {
        set[targetIndex].data[i] = mem_access(addr - offset + i, 0, 0);
    }
    set[targetIndex].tag = tag;
    set[targetIndex].dirty = 0;
    set[targetIndex].lruLabel = ++cache.lruCounter;
    printAction(addr - offset, cache.blockSize, memoryToCache);

    // Handle the original operation
    if (!write_flag) {
        printAction(addr, 1, cacheToProcessor);
        return set[targetIndex].data[offset];
    } else {
        set[targetIndex].data[offset] = write_data;
        set[targetIndex].dirty = 1;
        printAction(addr, 1, processorToCache);
        return 0;
    }

    return mem_access(addr, write_flag, write_data);
}


/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    int dirtyBlocks = 0;
    
    // Iterate over all cache blocks to count the dirty ones
    for (int i = 0; i < cache.numSets * cache.blocksPerSet; i++) {
        if (cache.blocks[i].dirty) {
            dirtyBlocks++;
        }
    }

    printf("End of run statistics:\n");
    printf("hits %i, misses %i, writebacks %i\n", cache.numHits, cache.numMisses, cache.numWB);
    printf("%i dirty cache blocks left", dirtyBlocks);
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}