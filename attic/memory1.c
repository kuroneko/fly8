/* --------------------------------- memory.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* A general purpose memory manager.
 *
 * This one keeps no extra information on allocated blocks. Free blocks
 * are linked by size and address to allow fast access for both allocation
 * and freeing/merging of blocks. It needs to have the block size suppplied
 * in the free() call, so watch out if you STRdup() and then modify the 
 * string since it will confuse it.
 *
 * It uses less memory than the faster manager used now with Fly8. Only
 * one fixed table of 256 entries of BLOCKS (3 pointers each). So the usage
 * is fixed a 3kb.
*/

#include "fly.h"


#define SIZES		0
#define HEADS		1
#define TAILS		2

#define GRAIN		8

#define NBLOCKS		256

#define MAXBYTES	(NBLOCKS*GRAIN)

#define MINBYTES \
	(offsetof (BLOCK, size) + sizeof (((BLOCK *)NULL)->size))

#define CHUNKSIZE	(MAXBYTES*4)

#define CHUNKMIN	(CHUNKSIZE/4)

#define CHUNKPART	(CHUNKMIN/2)

#define BYTESINDEX(n) \
	((Uint)(((n) < MINBYTES ? MINBYTES : (n)) - 1) / GRAIN)

#define INDEXSIZE(i)	(((i) + 1) * GRAIN)

#define ROUNDBYTES(n)	INDEXSIZE (BYTESINDEX (n))

#define PTRINDEX(p) \
	(Uint)((13579UL * ((Uint)(p) ^ (Uint)((Ulong)(p) >> 16))) % NBLOCKS)

#define LISTREMOVE(b,list) \
	do { \
		if (T(b->prev[list]->next[list] = b->next[list])) \
			b->next[list]->prev[list] = b->prev[list]; \
	} while (0)

#define LISTADD(b,list,n) \
	do { \
		b->prev[list] = (BLOCK *)&blocks[n]; \
		if (T(b->next[list] = blocks[n].next[list])) \
			b->next[list]->prev[list] = b; \
		blocks[n].next[list] = b; \
	} while (0)

#define MEM_MINLOG	MAXBYTES+1		/* log nothing */

/* Each block has three doubly-linked lists and a size. Note that we cheat
 * and expect the next[3] array in BLOCKS to be used in place of the similar
 * field in BLOCK (see the cast above in LISTADD).
*/
typedef struct block	BLOCK;
struct block {
	BLOCK	*next[3];
	BLOCK	*prev[3];
	Ushort	size;
};

/* Free blocks are kept on a cached list.
*/
typedef struct blocks	BLOCKS;
struct blocks {
	BLOCK	*next[3];		/* MUST be same as BLOCK start */
#ifdef MEM_STATS
	short	nused;				/* stats: good alloc - free */
	Ulong	nalloc;				/* stats: requested alloc */
	Ulong	nomem;				/* stats: memory short count */
#endif
};

/* memory is acquired in large chunks to later be carved into allocated
 * blocks.
*/
typedef struct chunk	CHUNK;
struct chunk {
	CHUNK	*next;
	Uint	avail;				/* size  of free space */
	Uchar	*mem;				/* start of free space */
	Uchar	buff[CHUNKSIZE];
};

static BLOCKS	*blocks = 0;
static CHUNK	*chunks = 0;
static Uint	malloc_dead = 0;		/* malloc failed! */
static Uint	largestAvail = 0;
static Ulong	ngrains = 0;			/* stats total malloc size */
static Ulong	nReqs = 0;			/* stats: block free requests */
static Ulong	nProbes = 0;			/* stats: block search length */
static int	debugging = 0;
static int	logging = 0;

LOCAL_FUNC void NEAR
mem_assert (void)
{
	int	i, n;
	BLOCK	*p;
	long	totbytes = 0;

	totbytes = STATS_MEMALLOCED;
	for (i = 0; i < NBLOCKS; ++i) {
		n = INDEXSIZE (i);
		for (p = blocks[i].next[SIZES]; p; p = p->next[SIZES]) {
			if (p->size !=  (Uint)n)
				LogPrintf ("assert: Bad size %5d in list %d\n",
					p->size, n);
			totbytes += n;
		}
	}
	if (T(totbytes -= ngrains*GRAIN))
		LogPrintf ("assert: leakage %9ld\n", -totbytes);
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
		}
		Sys->Enable (flags);
	} else {
		++STATS_MEMLOW;
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
		}
		Sys->Enable (flags);
	} else {
		++STATS_MEMLOW;
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
LOCAL_FUNC void * NEAR
my_alloc (Uint bytes)
{
	CHUNK	*p, *c;
	void	*b;
	int	n;

	p = NULL;
	if (largestAvail >= (Ushort)bytes) {
		for (c = chunks; c; c = c->next) {
			if (c->avail >= bytes && (NULL == p
						  || p->avail > c->avail))
				p = c;
		}
	}
	if (F(p)) {
		if (!malloc_dead) {
			for (n = sizeof (*p); n > CHUNKMIN; n -= CHUNKPART) {
				if (T(p = (CHUNK *)malloc (n)))
					break;
			}
		}
		if (F(p)) {
			malloc_dead = 1;
			++STATS_MEMLOW;
		} else {
			p->next = chunks;
			chunks = p;
			p->avail = sizeof (p->buff) - (sizeof (*p) - n);
			p->mem = p->buff;
			if (p->avail > largestAvail)
				largestAvail = p->avail;
			if (p->avail < bytes)
				p = NULL;
		}
	}
	if (p) {
		b = p->mem;
		p->mem += bytes;
		p->avail -= bytes;
		largestAvail = p->avail;
		for (c = chunks; c; c = c->next) {
			if (c->avail > largestAvail)
				largestAvail = c->avail;
		}
	} else
		b = NULL;
	return (b);
}

/* Try to satisfy a memory request by splitting a larger size block from
 * the free memory list.
*/
LOCAL_FUNC BLOCK * NEAR
mem_split (Uint n)
{
	Uint	i;
	Uint	bytes;
	BLOCK	*p;
	BLOCK	*t;

	p = NULL;
	for (i = n + 1 + BYTESINDEX (MINBYTES); ++i < NBLOCKS;) {
		if (F(p = blocks[i].next[SIZES]))
			continue;

		t = p;					/* retained block */

/* remove p from all lists. heads stays.
*/
		LISTREMOVE (t, TAILS);
		LISTREMOVE (t, SIZES);

		bytes = INDEXSIZE (n);			/* returned bytes */
		t->size -= bytes;			/* retained bytes */
		p = (BLOCK *)(t->size + (char *)t);	/* returned block */
		i -= n + 1;				/* retained index */

/* add the leftover t to all lists.
*/
		n = PTRINDEX (p);			/* p is the tail */
		LISTADD (t, TAILS, n);
		LISTADD (t, SIZES, i);

		if (logging && debugging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("split %u -> %u + %u\n",
				p->size, bytes, t->size);
		break;
	}
	return (p);
}

extern void * FAR
mem_alloc (Uint bytes)
{
	BLOCK	*p;
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

	i = BYTESINDEX (bytes);
	flags = Sys->Disable ();

/* We get memory by first checking for a ready block, then trying to split
 * a larger block, then trying to acquire fresh memory.
*/
	if (T(p = blocks[i].next[SIZES])) {
		LISTREMOVE (p, HEADS);
		LISTREMOVE (p, TAILS);
		LISTREMOVE (p, SIZES);
	} else if (T(p = mem_split (i)))
		++STATS_MEMSPLIT;
	else if (T(p = (BLOCK *)my_alloc (INDEXSIZE (i))))
		STATS_MEMUSED = GRAIN * (ngrains += i + 1);
	else
		++STATS_MEMNO;

#ifdef MEM_STATS
	++blocks[i].nalloc;
	if (p)
		++blocks[i].nused;
	else
		++blocks[i].nomem;
#endif
	if (p)
		STATS_MEMALLOCED += INDEXSIZE (i);

	Sys->Enable (flags);
ret:
	if (p)
		memset (p, 0, bytes);
	return ((void *)p);
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
mem_free (void *bvoid, int bytes)
{
	BLOCK	*b;
	char	*tail;
	BLOCK	*p, *block = (BLOCK *)bvoid;
	int	nheads, ntails, nsizes;
	Uint	maxbytes;
	Ulong	flags;
	int	n;

	if (!block || bytes < 0)
		return (NULL);

	if (!bytes || bytes > MAXBYTES || !blocks)
		return (xfree (block));

	p = block->next[0];		/* for the return(p) */

	nsizes  = BYTESINDEX (bytes);
	bytes   = INDEXSIZE (nsizes);
	maxbytes = MAXBYTES - bytes;

	flags = Sys->Disable ();

	STATS_MEMALLOCED -= bytes;

#ifdef MEM_STATS
	--blocks[nsizes].nused;
	if (logging && !(st.flags1 & SF_ASYNC) && blocks[nsizes].nused < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#else
	if (logging && !(st.flags1 & SF_ASYNC) && STATS_MEMALLOCED < 0)
		LogPrintf ("bad free count (%d)\n", bytes);
#endif

/* Search for the head of this block in the tails list.
*/
	nheads = PTRINDEX (block);
	tail = bytes + (char *)block;
	ntails = PTRINDEX (tail);
retry:
	for (n = 0, b = blocks[nheads].next[TAILS]; b; b = b->next[TAILS]) {
		++n;
		if (b->size + (char *)b != (char *)block)
			continue;
		if (b->size > maxbytes)
			break;

		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("mem+ [%5d/%p] + %5d/%p -> %5d/%p\n",
				b->size, b, bytes, block, b->size+bytes, b);

		block = b;
		nheads = PTRINDEX (block);
		goto remove;
	}
	++nReqs;
	nProbes += n;

/* search for the tail of this block in the heads list.
*/
	for (n = 0, b = blocks[ntails].next[HEADS]; b; b = b->next[HEADS]) {
		++n;
		if (tail != (char *)b)
			continue;
		if (b->size > maxbytes)
			break;

		if (logging && !(st.flags1 & SF_ASYNC))
			LogPrintf ("mem+ %5d/%p + [%5d/%p] -> %5d/%p\n",
				bytes, block, b->size, b,
				b->size+bytes, block);

		tail = b->size + (char *)b;
		ntails = PTRINDEX (tail);
remove:
		++STATS_MEMMERGE;		/* keep stats */
		++nReqs;
		nProbes += n;

		LISTREMOVE (b, HEADS);		/* remove small b */
		LISTREMOVE (b, TAILS);
		LISTREMOVE (b, SIZES);

		bytes += b->size;		/* new size */
		nsizes = BYTESINDEX (bytes);
		maxbytes = MAXBYTES - bytes;
		goto retry;
	}
	++nReqs;
	nProbes += n;

/* This is a new block, add it to all lists.
*/
	LISTADD (block, HEADS, nheads);
	LISTADD (block, TAILS, ntails);
	LISTADD (block, SIZES, nsizes);
	block->size = (Ushort)bytes;

	Sys->Enable (flags);
	return (p);
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
	CHUNK	*p, *m;
	Ulong	flags;

#if 0
	mem_assert ();
#endif
	if (malloc_dead)
		return;

	for (m = p = chunks; p; p = p->next)
		if (p->avail > m->avail)
			m = p;

	if (!m || m->avail < CHUNKMIN) {
		flags = Sys->Disable ();
		if (T(p = (CHUNK *)malloc (sizeof (*p)))) {
			p->next = chunks;
			chunks = p;
			p->avail = sizeof (p->buff);
			p->mem = p->buff;
		} else
			malloc_dead = 1;
		Sys->Enable (flags);
	}
}

extern int FAR
mem_init (void)
{
	int	i;

	ngrains = 0;
	malloc_dead = 0;
	chunks = NULL;
	if (F(blocks = (BLOCKS *)xcalloc (NBLOCKS, sizeof (*blocks))))
		return (1);

	for (i = 0; i < NBLOCKS; ++i) {
		blocks[i].next[SIZES] = NULL;
		blocks[i].next[HEADS] = NULL;
		blocks[i].next[TAILS] = NULL;
#ifdef MEM_STATS
		blocks[i].nalloc = 0;
		blocks[i].nused = 0;
		blocks[i].nomem = 0;
#endif
	}

	mem_check ();

	return (0);
}

extern void FAR
mem_term (void)
{
	BLOCK	*p;
	CHUNK	*c, *c1;
	int	i, n;
	long	tt;
	long	avail;
	Ulong	totn;
	Ulong	totbytes;
#ifdef MEM_STATS
	Ulong	totalloc;
	Ulong	totnomem;
#endif

	if (!blocks)
		return;

	mem_assert ();

	totn = totbytes = 0;
#ifdef MEM_STATS
	totalloc = totnomem = 0;
#endif
	LogPrintf ("Memory usage summary:\n\n");
	LogPrintf ("Size  Count     Bytes   nAllocs   nFailed     nUsed\n");
	for (i = 0; i < NBLOCKS; ++i) {
		for (n = 0, p = blocks[i].next[SIZES]; p; p = p->next[SIZES])
			++n;
#ifdef MEM_STATS
		if (n || blocks[i].nalloc) {
			totalloc += blocks[i].nalloc;
			totnomem += blocks[i].nomem;
#else
		if (n) {
#endif
			totn += n;
			tt = INDEXSIZE(i) * (long)n;
			totbytes += tt;
			LogPrintf ("%4u %6u %9lu", INDEXSIZE(i), n, tt);
#ifdef MEM_STATS
			LogPrintf (" %9lu", blocks[i].nalloc);
			if (blocks[i].nomem || blocks[i].nused)
				LogPrintf (" %9lu %9d",
					blocks[i].nomem, blocks[i].nused);
#endif
			LogPrintf ("\n");
		}
	}
	LogPrintf (" tot %6lu %9lu", totn, totbytes);
#ifdef MEM_STATS
	LogPrintf (" %9lu %9lu", totalloc, totnomem);
#endif
	LogPrintf ("\nSize  Count     Bytes   nAllocs   nFailed     nUsed\n");

	n = 0;
	avail = 0;
	LogPrintf ("\nChunk    Unused\n");
	for (c1 = chunks; T(c = c1);) {
		++n;
		LogPrintf ("%5d %9u\n", n, c->avail);
		c1 = c->next;
		avail += c->avail;
		xfree (c);
	}
	LogPrintf ("total %9lu\n\n", avail);
	chunks = 0;
	LogPrintf ("Chunks   %9lu (%u)\n", n*(long)CHUNKSIZE, n);
	LogPrintf ("Unused   %9lu\n", avail);
	LogPrintf ("Alloc'ed %9lu\n", ngrains*GRAIN);
	LogPrintf ("Freed    %9lu\n", totbytes);
	LogPrintf ("Leakage  %9ld\n", ngrains*GRAIN - totbytes);
	LogPrintf ("nReqs    %9ld\n", nReqs);
	LogPrintf ("nProbes  %9ld\n", nProbes);

	blocks = xfree (blocks);
	ngrains = 0;
}

#undef SIZES
#undef HEADS
#undef TAILS
#undef GRAIN
#undef NBLOCKS
#undef MAXBYTES
#undef CHUNKSIZE
#undef CHUNKMIN
#undef CHUNKPART
#undef MEM_MINLOG
#undef MINBYTES
#undef BYTESINDEX
#undef INDEXSIZE
#undef ROUNDBYTES
#undef PTRINDEX
#undef LISTREMOVE
#undef LISTADD
