#include<iostream>
#include<vector>
#include<list>
#include<cstdint>
#include<cstdlib>
#include<algorithm>
#include<climits>
#include<cstdio>
#include<assert.h>
#include<omp.h>
#include<sys/mman.h>
#include<unistd.h>
#include<cstring>
#include<iomanip>
#include<atomic>
#include<thread>
using namespace std;

namespace jemalloc{

	#define ll long long

	typedef uint8_t arena_chunk_map_t;

	size_t    bin_maxclass;

	/* VM page size. */
	static size_t pagesize;
	static size_t pagesize_mask;
	static size_t pagesize_2pow;

	unsigned ntbins; /* Number of (2^n)-spaced tiny bins. */
	unsigned nqbins; /* Number of quantum-spaced bins. */
	unsigned nsbins; /* Number of (2^n)-spaced sub-page bins. */
	size_t small_min;
	size_t small_max;

	/* Various quantum-related settings. */
	size_t quantum;
	size_t quantum_mask; /* (quantum - 1). */

	/* Various chunk-related settings. */
	size_t chunksize;
	size_t chunksize_mask; /* (chunksize - 1). */
	size_t chunk_npages;
	size_t arena_chunk_header_npages;
	size_t arena_maxclass; /* Max size class for arenas. */

	int arena_index;
	int num_arenas;
	bool aslr;

	#define	CHUNK_MAP_UNTOUCHED	0x80U
	#define	CHUNK_MAP_DIRTY		0x40U
	#define	CHUNK_MAP_LARGE		0x20U
	#define SIZEOF_INT_2POW	2
	#define	TINY_MIN_2POW		1
	#define	SMALL_MAX_2POW_DEFAULT	9
	#define	SMALL_MAX_DEFAULT	(1U << SMALL_MAX_2POW_DEFAULT)
	#define QUANTUM_2POW_MIN      4
	#define	CHUNK_2POW_DEFAULT	20

	/* Return the chunk address for allocation address a. */
	#define	CHUNK_ADDR2BASE(a)						\
		((void *)((uintptr_t)(a) & ~chunksize_mask))

	/* Return the chunk offset of address a. */
	#define	CHUNK_ADDR2OFFSET(a)						\
		((size_t)((uintptr_t)(a) & chunksize_mask))

	/* Return the smallest chunk multiple that is >= s. */
	#define	CHUNK_CEILING(s)						\
		(((s) + chunksize_mask) & ~chunksize_mask)

	/* Return the smallest cacheline multiple that is >= s. */
	#define	CACHELINE_CEILING(s)						\
		(((s) + (CACHELINE - 1)) & ~(CACHELINE - 1))

	/* Return the smallest quantum multiple that is >= a. */
	#define	QUANTUM_CEILING(a)						\
		(((a) + quantum_mask) & ~quantum_mask)

	/* Return the smallest pagesize multiple that is >= s. */
	#define	PAGE_CEILING(s)							\
		(((s) + pagesize_mask) & ~pagesize_mask)

	#define	RUN_MAX_SMALL_2POW	15
	#define	RUN_MAX_SMALL (1U << RUN_MAX_SMALL_2POW)

	/* Compute the smallest power of 2 that is >= x. */
	size_t pow2_ceil(size_t x){
		x--;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
	#if (SIZEOF_PTR == 8)
		x |= x >> 32;
	#endif
		x++;
		return (x);
	}

	int ctz(int x){
		if(x==0){
			return 0;
		}
		int t=1, r=0;
		while((x&t)==0){
			t=t<<1;
			r+=1;
		}
		return r;
	}

	int ffs(int x){
		/*
		implementation of find first set.
		Basically finds the position of the least significant bit set to 1.
		*/
		int i = ctz(x);
		if (i != 0)
			return (i + 1);

		return (0);
	}

	typedef pthread_mutex_t malloc_mutex_t;
	typedef pthread_mutex_t malloc_spinlock_t;

	/*Declaring all mutex objects*/
	static malloc_spinlock_t	huge_mtx;
	static size_t	opt_quantum_2pow = QUANTUM_2POW_MIN;
	static size_t	opt_small_max_2pow = SMALL_MAX_2POW_DEFAULT;
	static size_t	opt_chunk_2pow = CHUNK_2POW_DEFAULT;

	static inline void malloc_spin_lock(malloc_spinlock_t *lock){
			pthread_mutex_lock(lock);
	}

	static inline void malloc_spin_unlock(malloc_spinlock_t *lock){
			pthread_mutex_unlock(lock);
	}

	/* Forward Declarations */
	typedef struct arena_run_t arena_run_t;
	typedef struct arena_t arena_t;
	typedef struct arena_chunk_t arena_chunk_t;

	struct arena_bin_t{
		arena_run_t *runcurr;  // current run used to service allocations of this bin size
		vector<arena_run_t*> runs;  // list of all non-full runs
		size_t reg_size;  // Size of regions in a run for this bin's size class
		size_t run_size;  // Total size of a run for this bin's size class.
		uint32_t nregs;  // Total number of regions in a run for this bin's size class.
		uint32_t regs_mask_nelms;  // Number of elements in a run's regs_mask for this bin's size class.
		uint32_t reg0_offset;  // Offset of first region in a run for this bin's size class.
	};

	struct arena_run_t{
		arena_bin_t *bin; // bin that this run is associated with
		arena_chunk_t *chunk_base_addr;
		unsigned regs_minelm; // index of the first element that might have a free region
		unsigned nfree; // number of free regions in the run
		unsigned regs_mask[1]; // Bitmask of in-use regions (0: in use, 1: free). Dynamically sized.
	};

	struct arena_chunk_t{
		arena_t *arena;
		size_t pages_used;
		size_t ndirty;
		size_t size;
		uint8_t map[1];
	};

	struct arena_t {
		pthread_mutex_t	lock;
		vector<arena_chunk_t*> chunks;  // List of chunks that this arena manages
		arena_chunk_t *spare;  // Cache the most recently freed chunk
		size_t ndirty;
		vector<arena_run_t*> runs_available; // 	List of the available runs in the current arena. Runs are allocated using first-best-fit run allocation
		vector<arena_run_t*> runs_allocated; // 	List containing the allocated runs for this arena

		/*
		 * bins is used to store rings of free regions of the following sizes,
		 * assuming a 16-byte quantum, 4kB pagesize, and default MALLOC_OPTIONS.
		 *
		 *   bins[i] | size |
		 *   --------+------+
		 *        0  |    2 |
		 *        1  |    4 |
		 *        2  |    8 |
		 *   --------+------+
		 *        3  |   16 |
		 *        4  |   32 |
		 *        5  |   48 |
		 *        6  |   64 |
		 *           :      :
		 *           :      :
		 *       33  |  496 |
		 *       34  |  512 |
		 *   --------+------+
		 *       35  | 1024 |
		 *       36  | 2048 |
		 *   --------+------+
		 */
		vector<arena_bin_t*> bins;
	};

	vector<arena_t*> arenas;

	void insert_created_chunk_into_list(arena_t *arena, arena_chunk_t *chunk){
		// insert the chunk into arena->runs_allocated
		arena->chunks.push_back(chunk);
	}

	void insert_created_run_into_list(arena_t *arena, arena_run_t *run){
		// insert the chunk into arena->runs_allocated
		arena->runs_allocated.push_back(run);
	}

	void remove_run(arena_t* arena, vector<arena_run_t*> *runs, arena_run_t *run){
		// remove run from runs
		runs->erase(std::remove(runs->begin(), runs->end(), run), runs->end());
	}

	void* pages_map(void *addr, size_t size){
		void *ret;
		ret = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if(ret==MAP_FAILED){
			ret=NULL;
		}
		return ret;
	}

	void pages_unmap(void *addr, size_t size){
		munmap(addr, size);
	}

	// arena->runs_avail_szad
	arena_run_t* find_best_fit(arena_t *arena, size_t chunk_size){
		int size=arena->runs_available.size();
		for(int i=0;i<size;i++){
			arena_run_t *curr_run = arena->runs_available[i];
			// check if the size of the run is greater than the required size.
			// if it is find the smallest possible run
			if(curr_run->nfree > chunksize){
				return curr_run;
			}
		}
	  return NULL;
	}

	// From the given vector, return the first usable run
	arena_run_t* smallest_usable_run_larger_than_current(arena_t *arena, vector<arena_run_t*> *runs){
		int run_vec_size=runs->size();
		// return NULL;
		arena_run_t *curr_run=NULL, *run=NULL;
		for(int i=0;i<run_vec_size;i++){
			curr_run=runs->at(i);
			if(curr_run->nfree>0){
				run=curr_run;
				break;
			}
		}
		return run;
	}


	static size_t arena_bin_run_size_calc(arena_bin_t *bin, size_t min_run_size){
		size_t try_run_size, good_run_size;
		unsigned good_nregs, good_mask_nelms, good_reg0_offset;
		unsigned try_nregs, try_mask_nelms, try_reg0_offset;

		assert(min_run_size >= pagesize);
		assert(min_run_size <= arena_maxclass);
		assert(min_run_size <= RUN_MAX_SMALL);

		try_run_size = min_run_size;
		try_nregs = ((try_run_size - sizeof(arena_run_t)) / bin->reg_size) + 1;
		do {
			try_nregs--;
			try_mask_nelms = (try_nregs >> (SIZEOF_INT_2POW + 3)) +
			    ((try_nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1)) ? 1 : 0);
			try_reg0_offset = try_run_size - (try_nregs * bin->reg_size);
		} while (sizeof(arena_run_t) + (sizeof(unsigned) * (try_mask_nelms - 1)) > try_reg0_offset);

	    good_run_size = try_run_size;
	    good_nregs = try_nregs;
	    good_mask_nelms = try_mask_nelms;
	    good_reg0_offset = try_reg0_offset;

		assert(sizeof(arena_run_t) + (sizeof(unsigned) * (good_mask_nelms - 1))
		    <= good_reg0_offset);
		assert((good_mask_nelms << (SIZEOF_INT_2POW + 3)) >= good_nregs);

		/* Copy final settings. */
		bin->run_size = good_run_size;
		bin->nregs = good_nregs;
		bin->regs_mask_nelms = good_mask_nelms;
		bin->reg0_offset = good_reg0_offset;

		return (good_run_size);
	}


	void arena_run_split(arena_t *arena, arena_run_t *run, size_t size, bool small, bool zero){
		/*
		* Insert run into the data structure for the arena. The data structure is the list of allocated runs.
		* Set the size of the run.
		* The size of run must be smaller/equal to the chunk size
		*
		*/
	  arena_chunk_t *chunk, *allocated_chunk;
	  size_t run_index, total_pages, pages_required, remaining_pages, index, need_pages;

	  insert_created_run_into_list(arena, run);
	  chunk = run->chunk_base_addr;
	  arena_chunk_t* temp_chunk_addr = (arena_chunk_t *)CHUNK_ADDR2BASE(run);
	  run_index = (unsigned)(((uintptr_t)run - (uintptr_t)chunk) >> pagesize_2pow);
	  total_pages = chunk->size >> pagesize_2pow;
	  need_pages = (size >> pagesize_2pow);
	  assert(need_pages > 0);
	  assert(need_pages <= total_pages);
	  remaining_pages = total_pages - need_pages;

	  for(index=0;index<need_pages;index+=1){
	    if(zero){
	      if ((chunk->map[run_index + index] & CHUNK_MAP_UNTOUCHED) == 0) {
					memset((void *)((uintptr_t)chunk + ((run_index
					    + index) << pagesize_2pow)), 0, pagesize);
				}
			}
	  }
	  chunk->pages_used += need_pages;
	}

	static inline void* chunk_alloc_mmap(size_t size){
		void *ret;
		size_t offset;

		ret=pages_map(NULL, size);
		if(ret==NULL){
			return NULL;
		}
		return ret;
	}

	void* chunk_alloc(size_t size, bool zero){
		void *ret;
		assert(size>0);
		ret=chunk_alloc_mmap(size);
		if(ret==NULL){
			return NULL;
		}
		return ret;
	}

	arena_chunk_t *arena_chunk_alloc(arena_t* arena, size_t size){
		// Creates a new chunk in the given arena of size.
		arena_chunk_t *chunk;
		if(arena->spare!=NULL){
			chunk=arena->spare; // If there exists a spare arena available
			arena->spare=NULL;
		}
		else{
			chunk = (arena_chunk_t*) chunk_alloc(chunksize, true);
			if(chunk==NULL){
				return NULL;
			}
			chunk->arena=arena;
			insert_created_chunk_into_list(arena, chunk);
			chunk->pages_used = 0;
			chunk->ndirty = 0;
			chunk->size=size;
		}
		// insert into runs_avail_
		return chunk;
	}


	arena_run_t* arena_run_alloc(arena_t *arena, size_t size, bool small, bool zero){
	  arena_chunk_t *chunk;
	  arena_run_t *run, *temp_node;

	  assert(size <= (chunksize - (arena_chunk_header_npages << pagesize_2pow)));  // check if size requested is less than

	  // search arena's chunks for lowest best fit.
	  // check in arena->runs_avail_szad for available best fit runs

	  run = find_best_fit(arena, size); //  &arena->runs_avail_szad

	  if(run!=NULL){
	    arena_run_split(arena, run, size, small, zero);
	    return run;
	  }

	  // run is NULL. This means there are no available runs
	  // create a new chunk
	  chunk = arena_chunk_alloc(arena, size);
	  if(chunk==NULL){
	    return NULL;
	  }
		/*
		A new chunk of memory has been created.
		Using this new chunk, define a page run starting at the base address
		*/
	  	run = (arena_run_t *)((uintptr_t)chunk + (arena_chunk_header_npages << pagesize_2pow));
		/* Update page map. */
		run->chunk_base_addr=chunk;
		arena_run_split(arena, run, size, small, zero);
		return (run);
	}

	static inline void *arena_run_reg_alloc(arena_run_t *run, arena_bin_t *bin){
	  void *ret;

	  unsigned curr_index, mask, bit, regind;

	  assert(run->regs_minelm < bin->regs_mask_nelms);  // check if

	  curr_index=run->regs_minelm;
	  mask=run->regs_mask[1];

	  if(mask!=0){
	    // Usable allocation found

	    bit=ffs((int)mask) - 1;
	    // regind = ((curr_index << (SIZEOF_INT_2POW + 3)) + bit);
		ret = (void *) ((uintptr_t)run);
	    /* Clear bit. */
	    mask ^= (1U << bit);
	    run->regs_mask[curr_index] = mask;
	    return (ret);
	  }
	  // Perform the same operation in a loop
	  for (curr_index++; curr_index < bin->regs_mask_nelms; curr_index++) {
			mask = run->regs_mask[curr_index];
			if (mask != 0) {
				/* Usable allocation found. */
				bit = ffs((int)mask) - 1;

				regind = ((curr_index << (SIZEOF_INT_2POW + 3)) + bit);
				assert(regind < bin->nregs);
				ret = (void *)(((uintptr_t)run) + bin->reg0_offset + (bin->reg_size * regind));

				/* Clear bit. */
				mask ^= (1U << bit);
				run->regs_mask[curr_index] = mask;

				// Nothing before this element contains a free region.
				run->regs_minelm = curr_index; /* Low payoff: + (mask == 0); */
				return ret;
			}
		}

	  assert(0);
	  return NULL;
	}

	static inline arena_run_t* arena_bin_nonfull_run_get(arena_t *arena, arena_bin_t *bin){
	  arena_run_t *run;
	  unsigned i, remainder;

	  // Search for a usable run
	  run = smallest_usable_run_larger_than_current(arena, &bin->runs);
	  if(run!=NULL){
	    remove_run(arena, &bin->runs, run);  // remove the run from bins->runs
	    return run;
	  }
	  // Allocate new run
	  run = arena_run_alloc(arena, bin->run_size, true, false);
	  if(run==NULL){
	    return NULL;
	  }
	  // Initialize new run
	  run->bin = bin;
	  run->regs_minelm = 0;
	  run->nfree = bin->nregs;

		for (i = 0; i < bin->regs_mask_nelms; i++){
			run->regs_mask[i] = UINT_MAX;
		}
		remainder = bin->nregs & ((1U << (SIZEOF_INT_2POW + 3)) - 1);
		if(remainder != 0) {
			/* The last element has spare bits that need to be unset. */
			run->regs_mask[i] = (UINT_MAX >> ((1U << (SIZEOF_INT_2POW + 3)) - remainder));
		}

	  return run;
	}


	static inline void *arena_bin_malloc_easy(arena_t *arena, arena_bin_t *bin, arena_run_t *run){
	  void *ret; // return address pointer

	  assert(run->nfree > 0);

	  ret=arena_run_reg_alloc(run, bin);
	  assert(ret!=NULL);
	  run->nfree-=1;

	  return ret;
	}

	static inline void *arena_bin_malloc_hard(arena_t *arena, arena_bin_t *bin){
	  bin->runcurr = arena_bin_nonfull_run_get(arena, bin);
	  // bin->runcurr contains the best fit available bin
	  if(bin->runcurr==NULL){
	    return NULL;
	  }

	  assert(bin->runcurr->nfree>0);
	  return arena_bin_malloc_easy(arena, bin, bin->runcurr);
	}

	static inline void *arena_malloc_small(arena_t *arena, size_t size, bool zero){
		void *ret; // contains the return pointer
		arena_bin_t *bin; // bin information of the current arena
		arena_run_t *run; // run information of the current arena
		int value;
		if(size<small_min){
			// TINY ALLOCATION
			size=pow2_ceil(size);  // taken the ceil of the size
			value = ffs( (int)(size>>TINY_MIN_2POW+1) );
			bin = arena->bins[value];  // taken the bin size
		}

		else if(size<=small_max){
			// Quantum Allocation
			size=QUANTUM_CEILING(size);
			value = ntbins + (size >> opt_quantum_2pow) - 1;
			bin = arena->bins[value];
		}
		else{
			// Sub-Page
			size=pow2_ceil(size);
			value = (ffs((int)(size >> opt_small_max_2pow)) - 2);
			bin = arena->bins[ntbins + nqbins + value];
		}
		assert(size == bin->reg_size);  // size assertion

	  	malloc_spin_lock(&arena->lock);

		if((run = bin->runcurr)!=NULL && run->nfree > 0){
			ret=arena_bin_malloc_easy(arena, bin, run);
		}
		else{
			ret=arena_bin_malloc_hard(arena, bin);
		}

		if (ret==NULL) {
			malloc_spin_unlock(&arena->lock);
			return (NULL);
		}

		malloc_spin_unlock(&arena->lock);
		if(zero){
			memset(ret, 0, size);
		}
		return ret;
	}

	void remove_addr(void *addr, arena_run_t *run, size_t size){
		if(aslr){
			arena_chunk_t *chunk = run->chunk_base_addr;
			arena_t *arena = chunk->arena;
			remove_run(arena, &arena->runs_allocated, run);
			pages_unmap(addr, size);
		}
	}

	void *arena_malloc_large(arena_t *arena, size_t size, bool zero){
	  void *ret; // return pointer container the mapped address

	  // LARGE ALLOCATION
	  size=PAGE_CEILING(size);

	  malloc_spin_lock(&arena->lock);

	  ret=(void*) arena_run_alloc(arena, size, false, zero);
	  if(ret==NULL){
	    malloc_spin_unlock(&arena->lock);
	    return NULL;
	  }

	  malloc_spin_unlock(&arena->lock);
	  if(zero){
	    memset(ret, 0, size);
	  }
	  return ret;
	}


	static inline void* arena_malloc(arena_t *arena, size_t size, bool zero){
	  assert(arena!=NULL);
	  assert(size!=0);
		assert(QUANTUM_CEILING(size) <= arena_maxclass);

	  if(size<=bin_maxclass){
	    return arena_malloc_small(arena, size, zero);
	  }
	  else{
	    return arena_malloc_large(arena, size, zero);
	  }
	}

	void* huge_malloc(size_t size, bool zero){
	  void *ret;
	  size_t csize;

	  csize=CHUNK_CEILING(size);
	  if(csize==0){
	    return NULL;
	  }
		ret=chunk_alloc(csize, zero);  // allocating a chunk
		if(ret==NULL){
			return NULL;
		}

		// Insert the data into map/object store
		malloc_spin_lock(&huge_mtx);
	//	insert_into_ds();
		malloc_spin_unlock(&huge_mtx);

		if(zero){
			memset(ret, 0, csize);
		}
		return ret;
	}

	int num_threads;

	static inline arena_t* choose_arena_given_index(int i){
		// Allocates arena in a round robin manner
		arena_t *return_arena = arenas[i%num_threads%num_arenas];
		return return_arena;
	}


	static inline arena_t* choose_arena(){
		// Allocates arena in a round robin manner
		arena_index%=num_arenas;
		arena_t *return_arena = arenas[arena_index];
		arena_index+=1;
		return return_arena;
	}


	static inline void* mem_malloc(size_t size, int i){
		assert(size != 0);
    if (size <= arena_maxclass) {
      return arena_malloc(choose_arena_given_index(i), size, false);
    } else {
      return huge_malloc(size, false);
    }

	}

	static inline void* my_malloc1(size_t size){
	  assert(size!=0);

	  if(size<=arena_maxclass){
	    return arena_malloc(choose_arena(), size, false);
	  }
	  else{
	    return huge_malloc(size, false);
	  }
	}

	void mem_free(void *addr, size_t size){
		// remove the run mapping
		arena_run_t *run = (arena_run_t*) addr;
		remove_addr(addr, run, size);
		// arena_chunk_t *chunk = run->chunk_base_addr;
		// arena_t *arena = chunk->arena;
		// remove_run(arena, &arena->runs_allocated, run);
	}


	void arenas_create(arena_t *arena){
		arena->ndirty=0;
		arena->spare=NULL;

		arena_bin_t *bin;
		int i;

		size_t prev_run_size=pagesize;

		// Initialize chunks
		/*
		Initialize bins:
		1. Tiny spaced bins.
		2. Quantum bins.
		3. (2^n)-spaced sub-page bins.
		*/
		// Tiny spaced bins
		for(i=0;i<ntbins;i++){
			arena_bin_t *bin = new arena_bin_t;
			bin->runcurr=NULL;
			bin->reg_size = (1U << (TINY_MIN_2POW + i));
			bin->nregs=0;
			bin->regs_mask_nelms=0;
			bin->reg0_offset=0;
			arena->bins.push_back(bin);
			prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);
		}

		// Quantum bins
		while(i<ntbins+nqbins){
			arena_bin_t *bin = new arena_bin_t;
			bin->runcurr=NULL;
			bin->reg_size = quantum * (i - ntbins + 1);
			bin->nregs=0;
			bin->regs_mask_nelms=0;
			bin->reg0_offset=0;
			arena->bins.push_back(bin);
			prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);
			i+=1;
		}

		// Sub page bins
		while(i<ntbins+nqbins+nsbins){
			arena_bin_t *bin = new arena_bin_t;
			bin->runcurr=NULL;
			bin->reg_size = (small_max << (i - (ntbins + nqbins) + 1));
			bin->nregs=0;
			bin->regs_mask_nelms=0;
			bin->reg0_offset=0;
			arena->bins.push_back(bin);
			prev_run_size = arena_bin_run_size_calc(bin, prev_run_size);
			i+=1;
		}
	}

	void init(int count, int tc){
		num_arenas=count;
		num_threads=tc;
		pagesize_2pow=12;

		chunksize = (1LU << opt_chunk_2pow);
		chunksize_mask = chunksize - 1;
		chunk_npages = (chunksize >> pagesize_2pow);
		chunk_npages=256;

		size_t header_size = sizeof(arena_chunk_t) +
				(sizeof(arena_chunk_map_t) * (chunk_npages - 1)) +
				(sizeof(arena_chunk_t) * chunk_npages);
		// arena_chunk_header_npages=1000;

		long result = sysconf(_SC_PAGESIZE);
		assert(result != -1);
		aslr=false;

		pagesize_mask = result - 1;

		pagesize = (unsigned) result;
		int test_variable = header_size/4096;
		arena_chunk_header_npages = (header_size >> pagesize_2pow) +
				((header_size & pagesize_mask) != 0);
		arena_maxclass = chunksize - (arena_chunk_header_npages << pagesize_2pow);

		/* Set variables according to the value of opt_small_max_2pow. */
		if (opt_small_max_2pow < opt_quantum_2pow)
			opt_small_max_2pow = opt_quantum_2pow;
		small_max = (1U << opt_small_max_2pow);

		/* Set bin-related variables. */
		bin_maxclass = (pagesize >> 1);
		assert(opt_quantum_2pow >= TINY_MIN_2POW);
		ntbins = opt_quantum_2pow - TINY_MIN_2POW;
		assert(ntbins <= opt_quantum_2pow);
		nqbins = (small_max >> opt_quantum_2pow);
		nsbins = pagesize_2pow - opt_small_max_2pow - 1;

		/* Set variables according to the value of opt_quantum_2pow. */
		quantum = (1U << opt_quantum_2pow);
		quantum_mask = quantum - 1;
		if (ntbins > 0)
			small_min = (quantum >> 1) + 1;
		else
			small_min = 1;
		assert(small_min <= quantum);


		//Populate Arenas
		for(int i=0;i<count;i++){
			arena_t *arena = new arena_t;
			arenas_create(arena);
			arenas.push_back(arena);
		}
	}

}
