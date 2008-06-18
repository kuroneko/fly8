/* --------------------------------- memory.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* A general purpose memory manager.
 *
 * This one keeps a small header in front of all blocks (both free and
 * allocated) to enable merging adjacent ones. A separate quick access
 * by size list is also maintained for the allocation part. It does not
 * use the size in the free() call since it has it's own.
 *
 * Memory usage is a 512 entries table of BLOCK (one pointer each) and two
 * shorts for each memory block (suppose we have a 1000 blocks, then we use
 * 4kb for block headers and 2kb for the fixed table).
*/

#include "fly.h"


#define GRAIN		8

#define NBLOCKS		512

#define MAXBLOCK	(NBLOCKS*GRAIN)

#define BLOCKHEADER	(2 * sizeof (SIZE (NULL)))

#define MAXBYTES	(MAXBLOCK - BLOCKHEADER)

#define MINBYTES	(BLOCKHEADER + 2 * sizeof (NEXT (NULL)))

#define BYTESINDEX(n) \
	((Uint)(((n) < MINBYTES ? MINBYTES : (n)) - 1) / GRAIN)

#define INDEXSIZE(i)	(((i) + 1) * GRAIN)

#define CHUNKHEADER	(offsetof (CHUNK, buff) + GRAIN)

#define CHUNKSIZE	(MAXBLOCK*1)

#define CHUNKMIN	(CHUNKSIZE/4)

#define CHUNKPART	(CHUNKMIN/2)

#define MEM_MINLOG	0			/* log nothing */

#define SIZE(p)		((short *)(p))[-1]
#define PSIZE(p)	((short *)(p))[-2]
#define NEXT(p)		((char **)(p))[0]
#define PREV(p)		((char **)(p))[1]

#define DLLADD(n,b) \
	do { \
		char	*z; \
		if (T(z = NEXT (b) = blocks[n].chain)) { \
			PREV (b) = PREV (z); \
			PREV (z) = b; \
		} else \
			PREV (b) = (char *)&blocks[n].chain; \
		blocks[n].chain = b; \
	} while (0)

#define DLLREMOVE(b) \
	do { \
		char	*z; \
		if (T(z = NEXT (b))) \
			PREV (z) = PREV (b); \
		NEXT (PREV (b)) = z; \
	} while (0)

#define DLLTRIMHEAD(n,b) \
	do { \
		char	*z; \
		if (T(z = blocks[n].chain = NEXT (b))) \
			PREV (z) = PREV (b); \
	} while (0)

/* memory is acquired in large chunks to later be carved into allocated
 * blocks.
 *
 * chuncks are stored as a list of blocks. The first one is pointed at
 * by chunk->chain, then each block follows on block->next. The last block
 * on a chunk list is always a dummy block with a size of zero (it only has
 * an header, no body).
*/
typedef struct chunk	CHUNK;
struct chunk {
	CHUNK	*next;
	Ushort	size;
	long	buff[1];			/* need to force alignment */
};

/* a block has a header with the block (net) size and the size of the previous
 * block (as the dll ptr). Free blocks are linked to another list (based on
 * the block size) using block->next;
*/

/* Free blocks are kept on a list by size. It is just a simple pointer which
 * may have some stats attached.
*/
typedef struct blocks	BLOCKS;
struct blocks {
	char	*chain;		/* note: sometimes used as NEXT(block)!!! */
#ifdef MEM_STATS
	short	nused;				/* stats: good alloc - free */
	Ulong	nalloc;				/* stats: requested alloc */
	Ulong	nomem;				/* stats: memory short count */
#endif
};

static BLOCKS	*blocks = 0;
static CHUNK	*chunks = 0;			/* used very little */
static Uint	malloc_dead = 0;		/* malloc failed! */

static int	debugging = 0;			/* internal, leave alone */
static int	logging = 0;			/* set to 1 for logging */

/* some debugging functions first
*/
LOCAL_FUNC void * NEAR
verify_address (char *p, char *title)
{
	CHUNK	*c;

	if ( SIZE (p) <= 0 ||  SIZE (p) > MAXBLOCK ||
	    PSIZE (p) <  0 || PSIZE (p) > MAXBLOCK) {
		LogPrintf ("verify_address(%s)> bad %d/%p P %d\n",
			title, SIZE (p), p, PSIZE (p));
		return (NULL);
	}
	for (c = chunks; c; c = c->next) {
		if (p            >= GRAIN           + (char *)c->buff &&
		    SIZE (p) + p <= GRAIN + c->size + (char *)c->buff)
			return (p);
	}
	LogPrintf ("verify_address(%s)> stray %d/%p P %d\n",
		title, SIZE (p), p, PSIZE (p));
	return (NULL);
}

LOCAL_FUNC void NEAR
mem_assert (char *title)
{
	int	i;
	int	n;
	int	t;
	char	*p;
	long	freebytes = 0;
	CHUNK	*c;

	if (!blocks)
		return;

	freebytes = STATS_MEMALLOCED;
	for (i = 0; i < NBLOCKS; ++i) {
		n = INDEXSIZE (i);
		for (p = blocks[i].chain; p; p = NEXT (p)) {
			if (SIZE (p) !=  (short)n)
LogPrintf ("assert(%s): %5d - Bad %d/%p P %d\n",
					title, n, SIZE (p), p, PSIZE (p));
			freebytes += n;
		}
	}
	if (T(freebytes -= STATS_MEMTOTAL))
		LogPrintf ("assert(%s): leakage %9ld\n", title, -freebytes);
	i = 0;
	for (c = chunks; c; c = c->next) {
		++i;
		t = 0;
		for (n = 0, p = GRAIN + (char *)c->buff;;) {
			if (PSIZE (p) != n) {
LogPrintf ("assert(%s): %d[%ld]: bad P %d (!= %d)\n",
					title, i, t, PSIZE (p), n);
				break;
			}
			if (0 == (n = SIZE (p)))
				break;
			if (n < 0)
				n = -n;
			p += n;
			t += n;
		}
		if ((Ushort)t != c->size) {
			LogPrintf ("assert(%s): %d: bad length %ld\n",
				title, i, t);
		}
	}
}

/* A set of safe memory management functions.
*/

extern void * FAR
xmalloc (Uint size)
{
	Ulong	flags;
	void	*p;

	if (!malloc_dead) {
		flags = Sys->Disable ();
		if (F(p = malloc (size))) {
			malloc_dead = 1;
			++STATS_MEMLOW;
			++STATS_MEMNO;
		}
		Sys->Enable (flags);
	} else {
		++STATS_MEMLOW;
		++STATS_MEMNO;
		p = NULL;
	}
	return (p);
}

extern void * FAR
xcalloc (Uint count, Uint size)
{
	Ulong	flags;
	void	*p;

	if (!malloc_dead) {
		flags = Sys->Disable ();
		if (F(p = calloc (count, size))) {
			malloc_dead = 1;
			++STATS_MEMLOW;
			++STATS_MEMNO;
		}
		Sys->Enable (flags);
	} else {
		++STATS_MEMLOW;
		++STATS_MEMNO;
		p = NULL;
	}
	return (p);
}

extern char * FAR
xstrdup (const char *s)
{
	char	*p;
	Ulong	flags;

	flags = Sys->Disable ();
	if (F(p = strdup (s))) {
		malloc_dead = 1;
		++STATS_MEMLOW;
		++STATS_MEMNO;
	}
	Sys->Enable (flags);
	return (p);
}

extern void * FAR
xfree (void *block)
{
	Ulong	flags;

	if (!block)
		return (0);

	flags = Sys->Disable ();
	free (block);
	Sys->Enable (flags);
	malloc_dead = 0;
	return (0);
}

/* We get here if the free blocks list cannot satisfy a memory allocation
 * request.
*/
LOCAL_FUNC int NEAR
mem_chunk (void)
{
	CHUNK	*c;
	char	*b;
	int	n;

	for (n = CHUNKSIZE; n > CHUNKMIN; n -= CHUNKPART) {
		if (T(c = (CHUNK *)malloc (CHUNKHEADER + GRAIN + n)))
			break;
	}

	if (F(c)) {
		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("chunk malloc dead\n");
		malloc_dead = 1;
		++STATS_MEMLOW;
	} else {

/* add new chunk to chunks list.
*/
		c->next = chunks;
		chunks = c;
		c->size = (Ushort)n;
		STATS_MEMTOTAL += n;

/* build EOL block
*/
		b = GRAIN + n + (char *)c->buff;
		SIZE (b) = 0;
		PSIZE (b) = (short)n;

		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("chunk EOL  is %d/%p\n", SIZE (b), b);

/* build root block.
*/
		b = GRAIN + (char *)c->buff;
		SIZE (b) = (short)n;
		PSIZE (b) = 0;

		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("chunk root is %d/%p\n", SIZE (b), b);

/* add root to list by size.
*/
		n = BYTESINDEX (n);
		DLLADD (n, b);
	}
	return (malloc_dead);
}

/* Try to satisfy a memory request by splitting a larger size block from
 * the free memory list.
*/
LOCAL_FUNC char * NEAR
mem_split (Uint n)
{
	Uint	i;		/* index for block to split */
	Uint	bytes;
	char	*t;		/* block allocated */
	char	*p;		/* block leftover */
	char	*q;		/* following block */

	p = NULL;
	for (i = n + 1 + BYTESINDEX (MINBYTES); ++i < NBLOCKS;) {
		if (F(p = blocks[i].chain))
			continue;

		if (logging && debugging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("split [%d] %d/%p", i, SIZE (p), p);

/* remove p from list by size.
*/
		DLLTRIMHEAD (i, p);

/* we modify three blocks now, establish addressability.
*/
		bytes = INDEXSIZE (n);			/* bytes allocated */
		t = p;					/* retained block */
		q = t + SIZE (t);			/* next block */
		p = q - bytes;				/* returned block */

/* update headers.
*/
		PSIZE (q) = SIZE (p) = (short)bytes;
		PSIZE (p) = (SIZE (t) -= (short)bytes);

/* add (leftover) t to list by size;
*/
		i -= n + 1;
		DLLADD (i, t);

		if (logging && debugging && !(st.flags1 & SF_ASYNC))
			LogPrintf (" -> [%d] %d/%p + %d/%p\n",
			i, SIZE (t), t, SIZE (p), p);
		break;
	}

	return (p);
}

extern void * FAR
mem_alloc (Uint bytes)
{
	char	*p;
	Uint	i;
	Ulong	flags;

	if (!bytes || !blocks) {
		p = NULL;
		goto ret;
	}

	if (bytes > MAXBYTES) {
		p = xmalloc (bytes);
		goto ret;
	}

	i = bytes + BLOCKHEADER;
	i = BYTESINDEX (i);
	flags = Sys->Disable ();

/* We get memory by first checking for a ready block, then trying to split
 * a larger block, then trying to acquire fresh memory.
 * The loop should terminate by either getting a block or my running out of
 * system memory (malloc_dead).
*/
	do {
		if (T(p = blocks[i].chain))
			DLLTRIMHEAD (i, p);
		else {
			++STATS_MEMLOW;
			if (T(p = mem_split (i)))
				++STATS_MEMSPLIT;
			else if (malloc_dead || mem_chunk ()) {
				++STATS_MEMNO;
				break;
			}
		}
	} while (F(p));

#ifdef MEM_STATS
	++blocks[i].nalloc;
	if (p)
		++blocks[i].nused;
	else
		++blocks[i].nomem;
#endif
	if (p) {
		STATS_MEMALLOCED += SIZE (p);
		if (STATS_MEMALLOCED > STATS_MEMMAXUSED)
			STATS_MEMMAXUSED = STATS_MEMALLOCED;
		SIZE (p) = -SIZE (p);			/* is now alloc'ed */
	}

	Sys->Enable (flags);
ret:
	if (p)
		memset (p, 0, bytes);
	return (p);
}

extern void * FAR
memd_alloc (Uint bytes, char *file, int lineno)
{
	void	*p;

	debugging = 1;
	p = mem_alloc (bytes);
	debugging = 0;

	if (logging && !(st.flags1 & SF_ASYNC) && bytes >= MEM_MINLOG)
		LogPrintf ("alloc %s(%d) %u/%p\n", file, lineno, bytes, p);

	return (p);
}

/* A fast memory manager. A freed block is merged with neighbouring blocks
 * rather than just get added to the pool.
*/
extern void * FAR
mem_free (void *block, int bytes)
{
	char	*p;
	char	*t;
	Ulong	flags;
	int	n;

	if (!block || bytes < 0)
		return (NULL);

	if (!bytes || bytes > MAXBYTES || !blocks)
		return (xfree (block));

	flags = Sys->Disable ();

	p = block;
	block = NEXT (block);			/* for the return(p) */

#ifdef CHECK_MEM
if (-SIZE (p) < bytes ||
    -SIZE (p) != INDEXSIZE (BYTESINDEX (BLOCKHEADER + bytes))) {
	LogPrintf ("mem_free> %d bad block %d/%p\n", bytes, SIZE (p), p);
	Sys->Enable (flags);
	return (NULL);
}
#endif

	SIZE (p) = -SIZE (p);			/* is now free */

	bytes = SIZE (p);

	STATS_MEMALLOCED -= bytes;

#ifdef MEM_STATS
	n = BYTESINDEX (bytes);
	--blocks[n].nused;
	if (logging && !(st.flags1 & SF_ASYNC) && blocks[n].nused < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#else
	if (logging && !(st.flags1 & SF_ASYNC) && STATS_MEMALLOCED < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#endif

/* merge p into preceding blocks.
*/
	while (PSIZE (p) && (t = p - PSIZE (p), SIZE (t) > 0)) {
		DLLREMOVE (t);
		SIZE (t) += SIZE (p);			/* merge p into t */
		p = t;
		++STATS_MEMMERGE;
	}

/* merge following blocks into p.
*/
	while (t = p + SIZE (p), SIZE (t) > 0) {
		DLLREMOVE (t);
		SIZE (p) += SIZE (t);			/* merge t into p */
		++STATS_MEMMERGE;
	}
	PSIZE (t) = SIZE (p);

/* add p to list by size.
*/
	n = BYTESINDEX (SIZE (p));
	DLLADD (n, p);

	Sys->Enable (flags);

	return (block);
}

extern void * FAR
memd_free (void *block, int bytes, char *file, int lineno)
{
	if (logging && !(st.flags1 & SF_ASYNC) && bytes >= MEM_MINLOG)
		LogPrintf ("free %s(%d) %u/%p\n",
			file, lineno, bytes, block);

	return (mem_free (block, bytes));
}

extern char * FAR
mem_strdup (const char *s)
{
	char	*p;
	int	len;

	if (s) {
		if (T(p = mem_alloc (len = strlen (s) + 1)))
			memcpy (p, s, len);
	} else
		p = NULL;
	return (p);
}

extern char * FAR
memd_strdup (const char *s, char *file, int lineno)
{
	char	*p;
	int	len;

	if (logging && !(st.flags1 & SF_ASYNC)
					&& (!s || strlen(s)+1 >= MEM_MINLOG))
		LogPrintf ("strdup %s(%d) \"%s\"\n", file, lineno,
			s ? s : "(null)");

	if (s) {
		if (T(p = memd_alloc (len = strlen (s) + 1, file, lineno)))
			memcpy (p, s, len);
	} else
		p = NULL;
	return (p);
}

extern void * FAR
mem_strfree (char *s)
{
	if (s)
		memory_free (s, strlen (s) + 1);
	return (NULL);
}

extern void * FAR
memd_strfree (char *s, char *file, int lineno)
{
	if (logging && !(st.flags1 & SF_ASYNC))
		LogPrintf ("strfree %s(%d) \"%s\"\n", file, lineno,
			s ? s : "(null)");

	if (s)
		memd_free (s, strlen (s) + 1, file, lineno);
	return (NULL);
}

/* Ensure we are not low on memory. Not yet used.
*/
extern void FAR
mem_check (void)
{
	Ulong	flags;

#if 0
	mem_assert ("mem_check");
#endif
	if (malloc_dead)
		return;

	if (STATS_MEMTOTAL - STATS_MEMMAXUSED < CHUNKSIZE) {
		flags = Sys->Disable ();
		mem_chunk ();
		Sys->Enable (flags);
	}
}

extern int FAR
mem_init (void)
{
	int	i;

	STATS_MEMTOTAL = 0;
	STATS_MEMMAXUSED = 0;
	malloc_dead = 0;
	chunks = NULL;
	if (F(blocks = (BLOCKS *)xcalloc (NBLOCKS, sizeof (*blocks))))
		return (1);

	for (i = 0; i < NBLOCKS; ++i) {
		blocks[i].chain = NULL;
#ifdef MEM_STATS
		blocks[i].nalloc = 0;
		blocks[i].nused = 0;
		blocks[i].nomem = 0;
#endif
	}

	return (0);
}

extern void FAR
mem_term (void)
{
	char	*p;
	CHUNK	*c, *c1;
	int	i, n;
	long	tt;
	Ulong	size;
	Ulong	totn;
	Ulong	freebytes;
#ifdef MEM_STATS
	Ulong	totalloc;
	Ulong	totnomem;
#endif

	if (!blocks)
		return;

	mem_assert ("mem_term");

	totn = freebytes = 0;
#ifdef MEM_STATS
	totalloc = totnomem = 0;
#endif
	LogPrintf ("Memory usage summary:\n\n");
	LogPrintf ("Size  Count     Bytes   nAllocs   nFailed     nUsed\n");
	for (i = 0; i < NBLOCKS; ++i) {
		for (n = 0, p = blocks[i].chain; p; p = NEXT (p))
			++n;
#ifdef MEM_STATS
		if (n || blocks[i].nalloc) {
			totalloc += blocks[i].nalloc;
			totnomem += blocks[i].nomem;
#else
		if (n) {
#endif
			totn += n;
			tt = INDEXSIZE (i) * (long)n;
			freebytes += tt;
			LogPrintf ("%4u %6u %9lu", INDEXSIZE (i), n, tt);
#ifdef MEM_STATS
			LogPrintf (" %9lu", blocks[i].nalloc);
			if (blocks[i].nomem || blocks[i].nused)
				LogPrintf (" %9lu %9d",
					blocks[i].nomem, blocks[i].nused);
#endif
			LogPrintf ("\n");
		}
	}
	LogPrintf (" tot %6lu %9lu", totn, freebytes);
#ifdef MEM_STATS
	LogPrintf (" %9lu %9lu", totalloc, totnomem);
#endif
	LogPrintf ("\nSize  Count     Bytes   nAllocs   nFailed     nUsed\n");

	n = 0;
	size = 0;
	LogPrintf ("\nChunk      Size Address range\n");
	for (c1 = chunks; T(c = c1);) {
		++n;
		LogPrintf ("%5d %9u %p-%p\n", n, c->size,
			GRAIN + (char *)c->buff, c->size + (char *)c->buff);
		c1 = c->next;
		size += c->size;
		xfree (c);
	}
	chunks = 0;
	LogPrintf ("total %9lu\n\n", size);

	LogPrintf ("Max used %9lu\n", STATS_MEMMAXUSED);
	LogPrintf ("Alloc'ed %9lu\n", STATS_MEMTOTAL);
	LogPrintf ("Free     %9lu\n", freebytes);
	LogPrintf ("Leakage  %9ld\n", STATS_MEMTOTAL - freebytes);

	blocks = xfree (blocks);
}

#undef GRAIN
#undef NBLOCKS
#undef MAXBLOCK
#undef BLOCKHEADER
#undef MAXBYTES
#undef MINBYTES
#undef BYTESINDEX
#undef INDEXSIZE
#undef CHUNKHEADER
#undef CHUNKSIZE
#undef CHUNKMIN
#undef CHUNKPART
#undef MEM_MINLOG
#undef SIZE
#undef PSIZE
#undef NEXT
#undef PREV
#undef DLLADD
#undef DLLREMOVE
#undef DLLTRIMHEAD
