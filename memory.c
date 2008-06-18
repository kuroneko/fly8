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
 *
 * Debugging features:
 *
 * MEM_STATS	summary of memory usage will be done at program exit.
 * MEM_MAGIC	add magic number to block header for overwrite checks.
 * MEM_CHECK	do some extra checks to catch bad memory usage.
 * MEM_TRACE	log all memory activity to the Fly8 log.
*/

#include "fly.h"


#ifdef DEBUG_MEM
#define CHECK_MEM	1
#define MEM_TRACE	1
#endif

#ifdef CHECK_MEM
#define MEM_STATS	1
#define MEM_MAGIC	1
#define MEM_CHECK	1
#endif


#define GRAIN		8

#define NBLOCKS		512

#define MAXBLOCK	(NBLOCKS*GRAIN)

#define SIZE(p)		((short *)(p))[-1]
#define PSIZE(p)	((short *)(p))[-2]
#if MEM_MAGIC
#define MAGIC(p)	((short *)(p))[-3]
#define MAGIC_VAL	((short)0xf8f8)
#define BLOCKHEADER	(3 * sizeof (SIZE (NULL)))
#else
#define BLOCKHEADER	(2 * sizeof (SIZE (NULL)))
#endif

#define NEXT(p)		((char **)(p))[0]
#define PREV(p)		((char **)(p))[1]

#define MAXBYTES	(MAXBLOCK - BLOCKHEADER)

#define MINBYTES	(BLOCKHEADER + 2 * sizeof (NEXT (NULL)))

#define BYTESINDEX(n) \
	((Uint)(((n) < MINBYTES ? MINBYTES : (n)) - 1) / GRAIN)

#define INDEXSIZE(i)	(((i) + 1) * GRAIN)

#define CHUNKHEADER	(offsetof (CHUNK, buff) + GRAIN)

#define CHUNKSIZE	(MAXBLOCK*1)

#define CHUNKMIN	(CHUNKSIZE/4)

#define CHUNKPART	(CHUNKMIN/2)

#define MEM_MINLOG	0			/* log everything */

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

/* Memory is acquired in large chunks to later be carved into allocated
 * blocks.
 *
 * Each chunck is stored as a list of blocks. The first one is pointed at
 * by chunk->chain, then each block follows on block->next. The last block
 * on a chunk list is always a dummy block with a size of zero (it only has
 * a header, no body).
*/
typedef struct chunk	CHUNK;
struct chunk {
	CHUNK	*next;
	Ushort	size;
	long	buff[1];			/* need to force alignment */
};

/* A block has a header with the block (net) size and the size of the previous
 * block (as the dll ptr). Free blocks are linked to another list (based on
 * the block size) using block->next.
*/

/* Free blocks are kept on a list by size. It is just a simple pointer which
 * may have some stats attached.
*/
typedef struct blocks	BLOCKS;
struct blocks {
	char	*chain;		/* note: sometimes used as NEXT(block)!!! */
#if MEM_STATS
	short	nused;				/* stats: good alloc - free */
	Ulong	nalloc;				/* stats: requested alloc */
	Ulong	nomem;				/* stats: memory short count */
#endif
};

static BLOCKS	*blocks = 0;
static CHUNK	*chunks = 0;			/* used very little */
static Uint	malloc_dead = 0;		/* malloc failed! */

#if MEM_TRACE
static int	logging = 1;			/* set to 1 for logging */
static int	debugging = 0;			/* internal, leave alone */
#endif

#if 0
/* Some debugging functions first
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
#endif

/* It should be noted that it is resonable to see spurious assert failures.
 * These are the result of an a syncronous memory mgmt call done during the
 * check. The only way to avoid it is to lock for the full check, which is
 * too risky.
 *
 * Basically, if a problem shows and then goes away then it is OK. If it
 * stays then it is real.
*/
LOCAL_FUNC void NEAR
mem_assert (char *title)
{
	int	i;
	int	n;
	int	t;
	char	*p;
	long	freebytes = 0;
	CHUNK	*c;
	Ulong	flags;

	if (!blocks)
		return;

/* check the free blocks list
*/
	freebytes = STATS_MEMALLOCED;
	for (i = 0; i < NBLOCKS; ++i) {
		n = INDEXSIZE (i);
		flags = Sys->Disable ();
		for (p = blocks[i].chain; p; p = NEXT (p)) {
			if (SIZE (p) !=  (short)n) {
LogPrintf ("assert(%s): %5d - Bad %d/%p P %d\n",
					title, n, SIZE (p), p, PSIZE (p));
				break;
			}
			freebytes += n;
		}
		Sys->Enable (flags);
	}
	if (T(freebytes -= STATS_MEMTOTAL))
		LogPrintf ("assert(%s): leakage %9ld\n", title, -freebytes);

/* check the chunks structure (free and allocated).
*/
	i = 0;
	for (c = chunks; c; c = c->next) {
		++i;
		t = 0;
		flags = Sys->Disable ();
		for (n = 0, p = GRAIN + (char *)c->buff;;) {
#if MEM_MAGIC
			if (MAGIC (p) != MAGIC_VAL) {
LogPrintf ("assert(%s): %d[%ld]: bad magic 0x%hx (!= 0x%hx)\n",
					title, i, t, MAGIC (p), MAGIC_VAL);
				break;
			}
#endif
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
		Sys->Enable (flags);
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
mem_topup (void)
{
	CHUNK	*c;
	char	*b;
	int	n;

	if (malloc_dead)
		return (malloc_dead);

	c = NULL;
	for (n = CHUNKSIZE; n > CHUNKMIN; n -= CHUNKPART) {
		if (T(c = (CHUNK *)malloc (CHUNKHEADER + GRAIN + n)))
			break;
	}

	if (F(c)) {
#if MEM_TRACE
		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("malloc() dead\n");
#endif
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
#if MEM_MAGIC
		MAGIC (b) = MAGIC_VAL;
#endif

#if MEM_TRACE
		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("chunk EOL  is %d/%p\n", SIZE (b), b);
#endif

/* build root block.
*/
		b = GRAIN + (char *)c->buff;
		SIZE (b) = (short)n;
		PSIZE (b) = 0;
#if MEM_MAGIC
		MAGIC (b) = MAGIC_VAL;
#endif

#if MEM_TRACE
		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("chunk root is %d/%p\n", SIZE (b), b);
#endif

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
mem_reuse (Uint n)
{
	Uint	i;		/* running block size index */
	Uint	j;		/* first block size to split */
	Uint	bytes;
	char	*t;		/* block allocated */
	char	*p;		/* block leftover */
	char	*q;		/* following block */

/* j is the largest size that is too small to split. If available then it will
 * be returned as is.
*/
	j = n + 1 + BYTESINDEX (MINBYTES);
	if (j > NBLOCKS)
		j = NBLOCKS;

	for (i = n; i < j; ++i) {
		if (F(p = blocks[i].chain))
			continue;

		DLLTRIMHEAD (i, p);
		return (p);
	}

	p = NULL;
	for (; i < NBLOCKS; ++i) {
		if (F(p = blocks[i].chain))
			continue;

#if MEM_TRACE
		if (logging && debugging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("split [%d] %d/%p", i, SIZE (p), p);
#endif

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
#if MEM_MAGIC
		MAGIC (p) = MAGIC (q) = MAGIC_VAL;
#endif

/* add (leftover) t to list by size;
*/
		i -= n + 1;
		DLLADD (i, t);

#if MEM_TRACE
		if (logging && debugging && !(st.flags1 & SF_ASYNC))
			LogPrintf (" -> [%d] %d/%p + %d/%p\n",

#endif
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

/* We get memory by first checking for a reusable block, then trying to
 * acquire fresh memory.
 * The loop should terminate by either getting a block or by running out of
 * system memory.
*/
	while (F(p = mem_reuse (i)) && !mem_topup ())
		;

#if MEM_STATS
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
	} else
		++STATS_MEMNO;

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

#if MEM_TRACE
	debugging = 1;
#endif

	p = mem_alloc (bytes);

#if MEM_TRACE
	debugging = 0;
#endif

#if MEM_TRACE
	if (logging && !(st.flags1 & SF_ASYNC)
#if MEM_MINLOG
		&& bytes >= MEM_MINLOG
#endif
		)
		LogPrintf ("alloc %s(%d) %u/%p\n", file, lineno, bytes, p);
#endif

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

#if MEM_CHECK
#if MEM_MAGIC
if (MAGIC_VAL != MAGIC (p)) {
	LogPrintf ("mem_free> %p bad magic 0x%x != 0x%x\n",
		p, (int)MAGIC (p), (int)MAGIC_VAL);
	Sys->Enable (flags);
	return (NULL);
}
#endif
if (-SIZE (p) < INDEXSIZE (BYTESINDEX (BLOCKHEADER + bytes))) {
	LogPrintf ("mem_free> %p bad size %d (%d)\n",
		p, bytes, (int)SIZE (p));
	Sys->Enable (flags);
	return (NULL);
}
#endif

	SIZE (p) = -SIZE (p);			/* is now free */

	bytes = SIZE (p);

	STATS_MEMALLOCED -= bytes;

#if MEM_STATS
	n = BYTESINDEX (bytes);
	--blocks[n].nused;
#if MEM_CHECK
	if (!(st.flags1 & SF_ASYNC) && blocks[n].nused < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#endif
#else
#if MEM_CHECK
	if (!(st.flags1 & SF_ASYNC) && STATS_MEMALLOCED < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#endif
#endif

/* merge p into preceding blocks.
*/
	while (PSIZE (p) && (t = p - PSIZE (p), SIZE (t) > 0)) {
		DLLREMOVE (t);
		SIZE (t) += SIZE (p);			/* merge p into t */
		p = t;
	}

/* merge following blocks into p.
*/
	while (t = p + SIZE (p), SIZE (t) > 0) {
		DLLREMOVE (t);
		SIZE (p) += SIZE (t);			/* merge t into p */
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
#if MEM_TRACE
	if (logging && !(st.flags1 & SF_ASYNC) && bytes >= MEM_MINLOG)
		LogPrintf ("free %s(%d) %u/%p\n",
			file, lineno, bytes, block);
#endif

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

#if MEM_TRACE
	if (logging && !(st.flags1 & SF_ASYNC)
				&& (!s
#if MEM_MINLOG
					|| strlen(s)+1 >= MEM_MINLOG
#endif
					))
		LogPrintf ("strdup %s(%d) \"%s\"\n", file, lineno,
			s ? s : "(null)");
#endif

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
		mem_free (s, strlen (s) + 1);
	return (NULL);
}

extern void * FAR
memd_strfree (char *s, char *file, int lineno)
{
#if MEM_TRACE
	if (logging && !(st.flags1 & SF_ASYNC))
		LogPrintf ("strfree %s(%d) \"%s\"\n", file, lineno,
			s ? s : "(null)");
#endif

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

#if MEM_TRACE
	mem_assert ("mem_check");
#endif
	if (malloc_dead)
		return;

	if (STATS_MEMTOTAL - STATS_MEMMAXUSED < CHUNKSIZE) {
		flags = Sys->Disable ();
		mem_topup ();
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
#if MEM_STATS
		blocks[i].nalloc = 0;
		blocks[i].nused = 0;
		blocks[i].nomem = 0;
#endif
	}

	return (0);
}

extern void FAR
mem_stats (void)
{
	char	*p;
	CHUNK	*c, *c1;
	int	i, n;
	long	tt;
	Ulong	size;
	Ulong	totn;
	Ulong	freebytes;
#if MEM_STATS
	Ulong	totalloc;
	Ulong	totnomem;
#endif

	if (!blocks)
		return;

	mem_assert ("mem_term");

	totn = freebytes = 0;
#if MEM_STATS
	totalloc = totnomem = 0;
#endif

#if MEM_CHECK || MEM_TRACE || MEM_STATS
	LogPrintf ("Memory usage summary:\n\n");
	LogPrintf ("%s%s\n", "Size  Count     Bytes",
#if MEM_STATS
		"   nAllocs   nFailed     nUsed");
#else
		"");
#endif
#endif
	for (i = 0; i < NBLOCKS; ++i) {
		for (n = 0, p = blocks[i].chain; p; p = NEXT (p))
			++n;
#if MEM_STATS
		if (n || blocks[i].nalloc) {
			totalloc += blocks[i].nalloc;
			totnomem += blocks[i].nomem;
#else
		if (n) {
#endif
			totn += n;
			tt = INDEXSIZE (i) * (long)n;
			freebytes += tt;

#if MEM_CHECK || MEM_TRACE || MEM_STATS
			LogPrintf ("%4u %6u %9lu",
				INDEXSIZE (i), n, tt);
#if MEM_STATS
			LogPrintf (" %9lu", blocks[i].nalloc);
			if (blocks[i].nomem || blocks[i].nused)
				LogPrintf (" %9lu %9d",
					blocks[i].nomem,
					blocks[i].nused);
#endif
			LogPrintf ("\n");
#endif
		}
	}

#if MEM_CHECK || MEM_TRACE || MEM_STATS
	LogPrintf (" tot %6lu %9lu", totn, freebytes);
#if MEM_STATS
	LogPrintf (" %9lu %9lu", totalloc, totnomem);
#endif
	LogPrintf ("\n%s%s\n", "Size  Count     Bytes",
			"   nAllocs   nFailed     nUsed");
#endif

	n = 0;
	size = 0;

#if MEM_CHECK || MEM_TRACE || MEM_STATS
	LogPrintf ("\nChunk      Size Address range\n");
#endif
	for (c1 = chunks; T(c = c1);) {
		++n;

#if MEM_CHECK || MEM_TRACE || MEM_STATS
		LogPrintf ("%5d %9u %p-%p\n", n, c->size,
			GRAIN + (char *)c->buff,
			c->size + (char *)c->buff);
#endif
		c1 = c->next;
		size += c->size;
		xfree (c);
	}
	chunks = 0;

#if MEM_CHECK || MEM_TRACE || MEM_STATS
	LogPrintf ("total %9lu\n\n", size);
#endif

	LogPrintf ("Max mem  %9lu\n", STATS_MEMMAXUSED);
	LogPrintf ("Alloc'ed %9lu\n", STATS_MEMTOTAL);
	LogPrintf ("Free     %9lu\n", freebytes);
	LogPrintf ("Leakage  %9ld\n", STATS_MEMTOTAL - freebytes);
}

extern void FAR
mem_term (void)
{
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
