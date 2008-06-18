/* --------------------------------- buffers.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Display list management.
*/

#include "fly.h"


#undef gr_1op
extern void FAR FASTCALL
gr_1op (Ushort a)
{
	if (st.buf_avail >= 1 || buffer_new ()) {
		*(st.buf_p)++ = a;
		--st.buf_avail;
	}
}

#undef gr_2op
extern void FAR FASTCALL
gr_2op (Ushort a, Ushort b)
{
	if (st.buf_avail >= 2 || buffer_new ()) {
		*(st.buf_p)++ = a;
		*(st.buf_p)++ = b;
		st.buf_avail -= 2;
	}
}

#undef gr_4op
extern void FAR FASTCALL
gr_4op (Ushort a, Ushort b, Ushort c, Ushort d)
{
	BUFLINE	*p;

	if (st.buf_avail >= 4 || buffer_new ()) {
		p = st.buf_p;
		p[0] = a;
		p[1] = b;
		p[2] = c;
		p[3] = d;
		st.buf_p = p + 4;
		st.buf_avail -= 4;
	}
}

#undef gr_nop
extern BUFLINE *FAR FASTCALL
gr_nop (int n)
{
	BUFLINE	*p;

	if (st.buf_avail >= n || buffer_new ()) {
		p = st.buf_p;
		st.buf_p += n;
		st.buf_avail -= n;
	} else
		p = NULL;
	return (p);
}

static Uint	WriteMode = T_MSET;
static Uint	wmstack[10];
static int	wmstacki = 0;

LOCAL_FUNC void NEAR FASTCALL
show_buf (BUFFER *b)
{
	BUFLINE		*p;
	BUFLINE		*end;
	Uint		op;
	Uint		x;
#ifdef CHECK_GR
	Uint		minx, maxx, miny, maxy;
#endif
	static Uint	color = 0;
	void		(FAR* MoveTo) (Uint x1, Uint y1);
	void		(FAR* DrawTo) (Uint x2, Uint y2, Uint c);
	void		(FAR* Ellipse) (Uint x, Uint y, Uint rx, Uint ry,
					Uint c);
	void		(FAR* Polygon) (int npoints, BUFLINE *points,
					Uint color);
	void		(FAR* Clear) (Uint x, Uint y, Uint sx, Uint sy,
					Uint c);

	MoveTo  = Gr->MoveTo;
	DrawTo  = Gr->DrawTo;
	Ellipse = Gr->Ellipse ? Gr->Ellipse : NoEllipse;
	Polygon = Gr->Polygon ? Gr->Polygon : NoPolygon;
	Clear   = Gr->Clear   ? Gr->Clear   : NoClear;

#ifdef CHECK_GR
	minx = (Uint)CS->left;
	maxx = (Uint)CS->left + CS->sizex;
	miny = (Uint)CS->top;
	maxy = (Uint)CS->top  + CS->sizey;
#endif

	p = b->first;
	end = b->p;
	for (; p < end;) {
		x = *p++;
		op = x & T_MASK;
		x &=    ~T_MASK;
		switch (op) {
		case T_COLOR:
			color = (Uint)(((Ulong)(0x0ff & x) << 16) | *p++);
			break;
		case T_DRAW:
#ifdef CHECK_GR
if (x  < minx || x  > maxx || p[0] < miny || p[0] > maxy) {
    	LogPrintf ("Draw(%d,%d) [%d,%d]\n", x, p[0], maxx, maxy);
    	break;
}
#endif
			DrawTo (x, *p++, color);
			++STATS_DRAWCOUNT;
			break;
		case T_MOVE:
#ifdef CHECK_GR
if (x  < minx || x  > maxx || p[0] < miny || p[0] > maxy) {
    	LogPrintf ("Move(%d,%d) [%d,%d]\n", x, p[0], maxx, maxy);
    	break;
}
#endif
			MoveTo (x, *p++);
			++STATS_MOVECOUNT;
			break;
		case T_OP:
			switch (x) {
			case T_NOP:
			case T_ERASE:
			case T_NOERASE:
				break;
			case T_MSET:
			case T_MXOR:
			case T_MOR:
				WriteMode = x;
				if (Gr->SetWriteMode)
					Gr->SetWriteMode (x);
				break;
			case T_MPUSH:
				if (wmstacki < rangeof (wmstack))
					wmstack[wmstacki++] = WriteMode;
				break;
			case T_MPOP:
				if (wmstacki > 0) {
					WriteMode = wmstack[--wmstacki];
					if (Gr->SetWriteMode)
						Gr->SetWriteMode (WriteMode);
				}
				break;
			default:		/* should not happen! */
				++STATS_ERRBUFLINE;
				break;
			}
			break;
		case T_ELLIPSE:
#ifdef CHECK_GR
if (x-p[1] < minx || x+p[1]  >= maxx || p[0]-p[2] < miny || p[0]+p[2] >= maxy) {
    	LogPrintf ("Ellipse(%d,%d,%d,%d) [%d,%d]\n",
		x, p[0], p[1], p[2], maxx, maxy);
    	break;
}
#endif
			Ellipse (x, p[0], p[1], p[2], color);
			p += 3;
			break;
		case T_POLYGON:
			sys_poll (3);
			Polygon (x, p, color);
			sys_poll (3);
			p += x * 2;
			break;
		case T_CLEAR:
#ifdef CHECK_GR
if (x < minx || x+p[1]  > maxx || p[0] < miny || p[0]+p[2] > maxy) {
    	LogPrintf ("Clear(%d,%d,%d,%d) [%d,%d]\n",
		x, p[0], p[1], p[2], maxx, maxy);
    	break;
}
#endif
			sys_poll (3);
			Clear (x, p[0], p[1], p[2], color);
			sys_poll (3);
			p += 3;
			break;
		default:		/* should not happen! */
			++STATS_ERRBUFLINE;
			break;
		}
	}
	return;
}

extern void FAR
buffer_show (BUFFER *b)
{
	wmstacki = 0;
	WriteMode = T_MSET;

	if (Gr->SetWriteMode)
		Gr->SetWriteMode (T_MSET);		/* initial mode */

	for (; b; b = b->next) {
		show_buf (b);
		sys_poll (3);
	}
}

extern void FAR
buffers_show (int which)
{
	int	i;

	buffer_show (st.bufs[which]);

	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;
		buffer_show (st.hdd[i].bufs[which]);
	}
}

LOCAL_FUNC void NEAR FASTCALL
erase_buf (BUFFER *b, Uint color)
{
	BUFLINE		*p;
	BUFLINE		*end;
	Uint		op;
	Uint		x;
	int		erase;
#ifdef CHECK_GR
	Uint		minx, maxx, miny, maxy;
#endif
	void		(FAR* MoveTo) (Uint x1, Uint y1);
	void		(FAR* DrawTo) (Uint x2, Uint y2, Uint c);
	void		(FAR* Ellipse) (Uint x, Uint y, Uint rx, Uint ry,
					Uint c);
	void		(FAR* Polygon) (int npoints, BUFLINE *points,
					Uint color);

	color   = st.colors[color];
	MoveTo 	= Gr->MoveTo;
	DrawTo 	= Gr->DrawTo;
	Ellipse = Gr->Ellipse ? Gr->Ellipse : NoEllipse;
	Polygon = Gr->Polygon ? Gr->Polygon : NoPolygon;

#ifdef CHECK_GR
	minx = (Uint)CS->left;
	maxx = (Uint)CS->left + CS->sizex;
	miny = (Uint)CS->top;
	maxy = (Uint)CS->top  + CS->sizey;
#endif

	erase = 1;
	p = b->first;
	end = b->p;
	for (; p < end;) {
		x = *p++;
		op = x & T_MASK;
		x &=    ~T_MASK;
		switch (op) {
		case T_COLOR:
			++p;
			break;
		case T_DRAW:
#ifdef CHECK_GR
if (x  < minx || x  > maxx || p[0] < miny || p[0] > maxy) {
    	LogPrintf ("Draw(%d,%d) [%d,%d]\n", x, p[0], maxx, maxy);
    	break;
}
#endif
			if (erase)
				DrawTo (x, *p++, color);
			++STATS_DRAWCOUNT;
			break;
		case T_MOVE:
#ifdef CHECK_GR
if (x  < minx || x  > maxx || *p < miny || *p > maxy) {
    	LogPrintf ("Move(%d,%d) [%d,%d]\n", x, *p, maxx, maxy);
    	break;
}
#endif
			if (erase)
				MoveTo (x, *p++);
			++STATS_MOVECOUNT;
			break;
		case T_ELLIPSE:
#ifdef CHECK_GR
if (x-p[1] < minx || x+p[1]  >= maxx || p[0]-p[2] < miny || p[0]+p[2] >= maxy) {
    	LogPrintf ("Ellipse(%d,%d,%d,%d) [%d,%d]\n",
		x, p[0], p[1], p[2], maxx, maxy);
    	break;
}
#endif
			if (erase)
				Ellipse (x, p[0], p[1], p[2], color);
			p += 3;
			break;
		case T_POLYGON:
			if (erase) {
				sys_poll (4);
				Polygon (x, p, color);
				sys_poll (4);
			}
			p += x * 2;
			break;
		case T_CLEAR:
			p += 3;
			break;
		case T_OP:
			switch (x) {
			case T_NOP:
			case T_MSET:
			case T_MXOR:
			case T_MOR:
			case T_MPUSH:
			case T_MPOP:
				break;
			case T_NOERASE:
				erase = 0;
				break;
			case T_ERASE:
				erase = 1;
				break;
			default:
				++STATS_ERRBUFLINE;
				break;
			}
			break;
		default:
			++STATS_ERRBUFLINE;
			break;
		}
	}
	return;
}

extern void FAR
buffer_erase (BUFFER *b, Uint color)
{
	wmstacki = 0;
	WriteMode = T_MSET;

	if (Gr->SetWriteMode)
		Gr->SetWriteMode (T_MSET);		/* initial mode */

	for (; b; b = b->next) {
		erase_buf (b, color);
		sys_poll (4);
	}
}

extern void FAR
buffers_erase (int which)
{
	int	i;
	int	solid;

	solid = (st.flags & SF_SKY) && (st.flags1 & SF_SOLIDSKY);

	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;
		if (!solid || !scenery (st.hdd[i].type))
			buffer_erase (st.hdd[i].bufs[which], st.hdd[i].BgColor);
	}
}

extern void FAR
buffer_free (BUFFER *b)
{
	BUFFER	*next;

	for (; b; b = next) {
		next = b->next;
		memory_free (b,
			sizeof (*b) + (BUFLEN - 1) * sizeof (*b->first));
		--st.nbuffers;
	}
}

extern void FAR
buffers_free (int which)
{
	int	i;

	buffer_free (st.bufs[which]);
	st.bufs[which] = 0;

	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;
		buffer_free (st.hdd[i].bufs[which]);
		st.hdd[i].bufs[which] = 0;
	}
}

extern void FAR
buffers_term (void)
{
	int	i;

	for (i = 0; i < NBUFS; ++i)
		buffers_free (i);

	buffer_free (st.buf[HEAD]);
	st.buf[HEAD] = st.buf[TAIL] = NULL;
	st.buf_p = NULL;
}

LOCAL_FUNC long NEAR FASTCALL
buffer_size (BUFFER *b)
{
	long	l;

	for (l = 0; b; b = b->next)
		l += b->p - b->first;
	return (l);
}

extern long FAR
buffers_size (int which)
{
	int	i;
	long	l;

	l = buffer_size (st.bufs[which]);
	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;
		l += buffer_size (st.hdd[i].bufs[which]);
	}
	return (l);
}

extern void FAR
buffer_close (void)
{
	if (st.buf[TAIL])
		st.buf[TAIL]->p = st.buf_p;
	st.buf_p     = NULL;
	st.buf_avail = 0;
}

extern BUFFER * FAR
buffer_new (void)
{
	BUFFER	*b;

	if (st.buf_p)
		buffer_close ();

	if (st.nbuffers >= st.maxbuffers)
		return (NULL);

	b = (BUFFER *) memory_alloc (sizeof (*b) +
					(BUFLEN - 1) * sizeof (*b->first));
	if (!b)
		return (NULL);

	++st.nbuffers;
	b->next = 0;
	b->p = b->first;
	if (st.buf[TAIL])
		st.buf[TAIL]->next = b;
	else
		st.buf[HEAD] = b;
	st.buf[TAIL] = b;

/* redundant copies for speed.
*/
	st.buf_p     = b->p;
	st.buf_avail = BUFLEN;

	return (b);
}
