/* Copyright (C) 1986 by Manx Software Systems */
/*
 * char *lmalloc(long)
 * char *malloc(unsigned)
 * char *calloc(unsigned, unsigned)
 * char *_alloc(long)
 */
#if sizeof(char *) == 4
typedef long heap_t;
#else
typedef unsigned int heap_t;
#endif

typedef struct freelist {
	heap_t	f_size;
	struct freelist	*f_chain;
} FREE;

static FREE	head, *last;
static FREE save;

#define NULL	(char *)0
#define GRAIN	(256*sizeof(FREE))

extern char *sbrk();

char *
_alloc(size)
heap_t size;
{
	register FREE *tp, *prev;
	int times;

	size = (size+1) & ~1;
	if (size < sizeof(FREE))
		size = sizeof(FREE);
	if ((prev = last) == NULL)
		last = head.f_chain = prev = &head;

	times = 0;
	for (tp = prev->f_chain ; ; prev = tp, tp = tp->f_chain) {
		/* merge blocks adjacent to the current loc */
		while (tp != tp->f_chain &&
				(FREE *)((char *)tp+tp->f_size) == tp->f_chain) {
			tp->f_size += tp->f_chain->f_size;
			tp->f_chain = tp->f_chain->f_chain;
		}

		if (tp->f_size >= size) {	/* if we have a big enough block */
			/* check to see if leftover is too small */
			if (tp->f_size < size+sizeof(FREE))
				/* if too small, give'm all of it */
				prev->f_chain = tp->f_chain;
			else { /* otherwise split the block */
				last = (FREE *)((char *)tp + size);
				prev->f_chain = last;
				save = *last;
				last->f_chain = tp->f_chain;
				last->f_size = tp->f_size - size;
				tp->f_size = size;
				tp->f_chain = 0; /* for free */
			}
			last = prev;
			return (char *)tp;
		} 

		/* else, we don't have a big enough block, yet */
		/* have we looked through all the free list? */
		if (tp >= tp->f_chain && ++times > 1) {
			if ((tp = (FREE *)sbrk(GRAIN)) == (FREE *)-1)
#ifdef DEBUG
{ printf("sbrk failed!\n");
  return (char *)NULL; }
#else
				return (char *)NULL;
#endif
			tp->f_size = GRAIN;
			tp->f_chain = 0; /* for free */
			free((char *)tp+sizeof(FREE));
			tp = &head;	/* start at beginning to merge new with old */
		}
	}
}

char *
malloc(size)
unsigned size;
{
	register char *tp;

	if ((tp = _alloc((heap_t )size+sizeof(FREE))) == 0)
		return 0;
	return tp+sizeof(FREE);
}

#ifdef MPU68000
char *
lmalloc(size)
long size;
{
	register char *tp;

	if ((tp = _alloc(size+sizeof(FREE))) == 0)
		return 0;
	return tp+sizeof(FREE);
}
#endif

char *
calloc(nelem, size)
unsigned nelem, size;
{
	register unsigned i = nelem*size;
	register char *cp;

	if ((cp = malloc(i)) != (char *)0)
		setmem(cp, i, 0);
	return cp;
}

free(area)
char *area;
{
	register FREE *tp, *hole;

	if (area == 0)
		return -1;
	hole = (FREE *)(area - sizeof(FREE));
	if (hole->f_chain != 0) 
		return -1;

	for (tp = last ; tp > hole || hole > tp->f_chain ; tp = tp->f_chain) {
		if (tp >= tp->f_chain && (hole > tp || hole < tp->f_chain))
			break;
	}

	hole->f_chain = tp->f_chain;
	tp->f_chain = hole;
	last = tp;
	return 0;
}

char *
realloc(area, size)
register char *area; 
unsigned size;
{
	register char *cp, *end;
	heap_t osize;

	end = (char *)((FREE *)area-1);
	if ((osize = end - area) > size) {
		osize = size;
		end = (char *)(area + osize);
	}
	free(area);
	if ((cp = malloc(size)) != 0 && cp != area) {
		movmem(area, cp, osize);
		if ((char *)last >= area && (char *)last < end) {
			*(FREE *)(cp + ((char *)last - area)) = save;
		}
	}
	return cp;
}

#ifdef MEMTEST
prtlist()
{
	register FREE *ptr;
	register int i;

	if (last == 0) {
		printf("List is empty\n");
		return;
	}

	ptr = &head;
	i = 0;
	do {
		printf("%c%08lx  ", ptr==last ?'*':' ', ptr);
		if (++i == 10) {
			putchar('\n');
			i = 0;
		}
		ptr = ptr->f_chain;
	} while (ptr != &head);
	if (i)
		putchar('\n');
}
#endif
