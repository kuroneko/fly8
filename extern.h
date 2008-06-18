/* --------------------------------- extern.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* mainly function prototypes.
*/

#ifndef FLY8_EXTERN
#define FLY8_EXTERN

#ifdef	DEBUG_MULDIV
#undef	USE_ASM
#endif

#ifdef	USE_ASM

#define	dithadj		dithadja
#define	VMmul		Vmula
#define	build_mat	matyxz
#define my_sin(a)	my_sina(a)
#define	fmul		fmula
#define	fdiv		fdiva
#define	muldiv		muldiva
#define	LL(x)		((int)(x))

#else

#define dithadj(x,d,t) \
	((int)(((x) * (long)(t) + ((x)>=0?(d):-(d))) / 1000))
#define VMmul(R,V,M) \
	(R[X]=VMRfmul(V,M,X),R[Y]=VMRfmul(V,M,Y),R[Z]=VMRfmul(V,M,Z))
#define build_mat cbuild_matyxz
#define my_sin(a)	my_sinc(a)

#ifdef	DEBUG_MULDIV

#define	fmul(x,y)	fmulchk ((x), (y), __FILE__, __LINE__)
#define	fdiv(x,y)	fdivchk ((x), (y), __FILE__, __LINE__)
#define	muldiv(x,y,z)	muldivchk ((x), (y), (z), __FILE__, __LINE__)
#define LL(x)		lcheck ((x), "LL", __FILE__, __LINE__)

#else

#define	fmul(x,y)	((int)(((long)(x) * (long)(y)) >> FSCALE))
#define	fdiv(x,y)	FnDIV (FSCALE, (x), (y))
#define muldiv(x,y,z)	((int)(((long)(x) * (long)(y)) / (long)(z)))
#define	LL(x)		((int)(x))

#endif
#endif

#define C_PI		3.1416
#define C_G		9.81

#define ALLOCATE(type)	(type *)xcalloc (sizeof (type), 1)

#define iabs(i)		((Uint)abs(i))

#define T(x)		(0 != (x))
#define F(x)		(0 == (x))

/* Here we trust 2's complement wraparound on overflow to handle the
 * case where our time wrapped around (every 49 days).
*/
#define TIMEDIFF(t, f)	((long)((t) - (f)))
#define TIMEOUT(f)	TIMEDIFF (st.present, (f))

/* alarms.c */
extern void	FAR alarm_set (int mode);
extern void	FAR hud_alarm (HUD *h, OBJECT *p, int color, int mode, int hon);

/* airdata.c */
extern void	FAR airdata (long height, int *srho, int *rrho, int *rho,
	int *sos);

/* altitude.c */
extern void	FAR show_altitude (HUD *h, OBJECT *p, int sx, int sy, int maxx,
	int maxy, int orgx, int orgy, int ttx, int tty, int tx, int ty, int ss,
	int shifty, int VVD[2]);

/* body.c */
extern BODY *	FAR bodies_new (ONAME name);
extern BODY *	FAR bodies_del (ONAME name);
extern void	FAR bodies_extent (ONAME name);
extern int	FAR bodies_init (void);
extern void	FAR bodies_term (void);
extern ONAME	FAR body_name (char *title);

/* buffers.c */
#ifdef INLINE
#define gr_1op(a)	GR_1OP(a)
#define GR_1OP(a) \
	do { \
		if (st.buf_avail >= 1 || buffer_new ()) { \
			*(st.buf_p)++ = a; \
			--st.buf_avail; \
		} \
	} while (0)
	
#define gr_2op(a,b)	GR_2OP(a,b)
#define GR_2OP(a,b) \
	do { \
		if (st.buf_avail >= 2 || buffer_new ()) { \
			*(st.buf_p)++ = a; \
			*(st.buf_p)++ = b; \
			st.buf_avail -= 2; \
		} \
	} while (0)
	
#define gr_4op(a,b,c,d)	GR_4OP(a,b,c,d)
#define GR_4OP(a,b,c,d) \
	do { \
		BUFLINE	*zp; \
		if (st.buf_avail >= 4 || buffer_new ()) { \
			zp = st.buf_p; \
			zp[0] = a; \
			zp[1] = b; \
			zp[2] = c; \
			zp[3] = d; \
			st.buf_p = zp + 4; \
			st.buf_avail -= 4; \
		} \
	} while (0)

#else
extern void	FAR FASTCALL gr_1op (Ushort a);
extern void	FAR FASTCALL gr_2op (Ushort a, Ushort b);
extern void	FAR FASTCALL gr_4op (Ushort a, Ushort b, Ushort c,
	Uxshort d);
#endif

extern BUFLINE *FAR FASTCALL gr_nop (int n);

#define gr_op(m)	gr_1op (T_OP      | (Ushort)(m))
#define gr_color(c) \
	gr_2op (T_COLOR | (Ushort)(0x0ff & (st.colors[c] >> 16)), \
		(Ushort)(st.colors[c]))
#define gr_move(x,y)	gr_2op (T_MOVE    | (Ushort)(x), (Ushort)(y))
#define gr_draw(x,y)	gr_2op (T_DRAW    | (Ushort)(x), (Ushort)(y))
#define gr_ellipse(x,y,rx,ry) \
			gr_4op (T_ELLIPSE | (Uint)(x), (Ushort)(y), \
				(Ushort)(rx), (Ushort)(ry))
#define gr_clear(x,y,sx,sy) \
			gr_4op (T_CLEAR   | (Uint)(x), (Ushort)(y), \
				(Ushort)(sx), (Ushort)(sy))
#define gr_line(x1,y1,x2,y2) \
	do { \
		gr_move ((x1), (y1)); \
		gr_draw ((x2), (y2)); \
	} while (0)

extern void	FAR buffer_show (BUFFER *b);
extern void	FAR buffers_show (int which);
extern void	FAR buffer_erase (BUFFER *b, Uint color);
extern void	FAR buffers_erase (int which);
extern void	FAR buffer_free (BUFFER *b);
extern void	FAR buffers_free (int which);
extern void	FAR buffers_term (void);
extern long	FAR buffers_size (int which);
extern void	FAR buffer_close (void);
extern BUFFER *	FAR buffer_new (void);


/* cockpit.c */
extern void	FAR show_inst (VIEW *view, OBJECT *obj);

/* colors.c */
extern char *	FAR color_name (int color);
extern char *	FAR color_rgb (int color);
extern int	FAR set_rrggbb (int letter, Ulong value);
extern void	FAR set_palette (void);
extern int	FAR color_assign (Ushort *item);
extern int	FAR menu_palette (void);
extern int	FAR menu_colors (void);

/* command.c */
extern Ushort	FAR set_lists (Ushort list);
extern int	FAR user_command (void);
extern int	FAR menu_cmd (void);

/* compass.c */
extern void	FAR show_compass (HUD *h, OBJECT *p, int mode);

/* debug.c */
extern int	FAR fmulchk (long x, long y, char *file, int line);
extern int	FAR fdivchk (long x, long y, char *file, int line);
extern int	FAR muldivchk (long x, long y, long z, char *file, int line);
extern long	FAR lcheck (long x, char *name, char *file, int line);

/* editstr.c */
extern int	FAR editset (
	void *	(FAR *s_malloc) (int n),
	void *	(FAR *s_free) (void *p, int n),
	void	(FAR *s_put) (int ch),
	int	(FAR *s_get) (void),
	void	(FAR *s_show) (int edit_mode),
	int	s_saved,
	int	m_recall);
extern int	FAR editget (char **str, int *len, int *pos, int *mode);
extern int	FAR editstr (char FAR* str, int len);

/* field.c */
extern int	FAR field_find (FILE *ifile, char *line);
extern int	FAR field_long (FILE *ifile, char *line, long *value);
extern int	FAR field_read (FILE *ifile, struct FldTab *fld, char *line);
#define READ_I		0x0100
#define READ_S		0x0200
#define READI(name)	{READ_I+sizeof (READFLD.name), &READFLD.name}
#define READS(name)	{READ_S, &READFLD.name}

/* fly8.c (this is where main() is) */
extern struct status	NEAR st;
extern int	FAR check_stack (int func);

/* fly8str.c */
extern int	FAR edit_init (void);
extern int	FAR edit_term (void);
extern int	FAR edit_str (char *prompt, char FAR* str, int len);
extern void	FAR edit_show (VIEW *view, int orgx, int orgy, int maxx,
	int maxy, int bss);

/* grmgr.c */
extern DEVICE *	FAR devices_select (char *dname);
extern void	FAR devices_release (void);
extern struct GrDriver NEAR* FAR devices_init (char *name);
extern void	FAR devices_term (void);

/* heading.c */
extern void	FAR show_heading (HUD *h, VIEW *view, OBJECT *p, int sx, int sy,
	int maxx, int maxy, int orgx, int orgy, int ttx, int tty, int tx,
	int ty, int ss, int shifty, int VVD[2]);

/* hud.c */
extern void	FAR show_hud (VIEW *view, OBJECT *pov, OBJECT *p,
	int orgx, int orgy, int maxx, int maxy, int mode);
extern int	FAR get_vv (OBJECT *p, VECT RR);
extern void	FAR show_num (int x, int y, long t, int s, int c, int orgx,
	int orgy, int maxx, int maxy, int shifty);
extern void	FAR add_segment (int x1, int y1, int x2, int y2,
	int orgx, int orgy, int sx, int sy, int shifty);
extern void	FAR add_dash (int x1, int y1, int x2, int y2, int ndash,
	int ratio, int orgx, int orgy, int sx, int sy);
extern void	FAR screen_coords (VIEW *view, VECT RR);
extern int	FAR clip_to_screen (int D[2], VECT RR, int maxx, int maxy,
	int clipx, int clipy, int shifty);
extern void	FAR clip_to_ether (HUD *h, int D[2], int x, int y);
extern int	FAR keep_inside (int *x, int *y, int xl, int xh, int yl, int yh,
	int orgx, int orgy, int clipx, int clipy, int shifty);
extern int	FAR is_in (HUD *h, int x, int y, int dx, int dy);

/* hudmenu.c */
extern int	FAR menu_hudtype (void);
extern int	FAR menu_hud (void);
extern void	FAR hud_setup (OBJECT *p);
extern void	FAR cc_setup (void);
extern void	FAR win_setup (void);

/* hudtrail.c */
extern void	FAR show_trail (HUD *h, VIEW *view, OBJECT *p);

/* ifuncs.c */
#if 1 /*was: ifdef INLINE*/
#define ihypot2d(x,y) \
	SQRT ((x)*(long)(x) + (y)*(long)(y))

#define ihypot3d(A) \
	SQRT (A[X]*(long)A[X] + A[Y]*(long)A[Y] + A[Z]*(long)A[Z])
#else
extern Uxshort	FAR FASTCALL ihypot2d (int x, int y);
extern Uxshort	FAR FASTCALL ihypot3d (VECT A);
#endif

#define	TANG(a)		((ANGLE)(0xffffU & (short)(a)))
#define	D180		0x8000
#define	D90		0x4000
#define	VD90		(D90/VONE)
#define	VD180		(VD90*2)
#define	VD360		(VD90*4)
#define	ANG2DEG(x)	muldiv ((x), 90, D90)
#define	ANG2DEG00(x)	muldiv ((x), 9000, D90)
#define DEG2ANG(x)	muldiv ((x), D90, 90)
#define	ANG2RAD(x)	fmul ((x), FCON(C_PI/2))
#define	RAD2ANG(x)	fmul ((x), FCON(2/C_PI))
#define DEG(x)		(TANG((x)*(long)D90/90))
#define	SIN(a)		((a) ? my_sin (a) : 0)
#define	COS(a)		((a) ? my_sin ((ANGLE)((a)+D90)) : FONE)
#define	ASIN(i)		my_asin(i)			/* [-90...+90] */
#define	ACOS(i)		(TANG(D90-my_asin(i)))		/* [0...180] */
							/* note: 180 is -ve! */
#define	ATAN(iy,ix)	my_atan((iy),(ix))
#define	SQRT(ul)	my_sqrt(ul)
#define FSQRT(i)	SQRT(FONE*(Ulong)i)

extern int	FAR funcs_init (void);
extern void	FAR funcs_term (void);
extern xshort	FAR FASTCALL lin2log (xshort linear);
extern xshort	FAR FASTCALL my_sinc (ANGLE angle);
extern ANGLE	FAR FASTCALL my_asin (int sin);
extern ANGLE	FAR FASTCALL my_atan (int y, int x);
extern Uxshort	FAR FASTCALL my_sqrt (Ulong x);
extern Ulong	FAR FASTCALL lhypot3d (LVECT A);
extern Ulong	FAR FASTCALL ldist3d (LVECT A, LVECT B);
extern Uxshort	FAR FASTCALL est_hyp (int x, int y, int z);
extern Uxshort	FAR FASTCALL est_dist (LVECT R1, LVECT R2);

/* info.c */
extern void	FAR stats_show (void);
extern void	FAR screen_info (VIEW *view, int orgx, int orgy, int maxx,
	int maxy, int bss, int mode);

/* init.c */
extern void	FAR initialize (char *argv[]);

/* kbdmgr.c */
extern struct KbdDriver NEAR* FAR kbrd_init (char *name);
extern void	FAR kbrd_term (void);

/* lamps.c */
extern void	FAR show_lamps (VIEW *view, OBJECT *p, int maxx, int maxy,
	int orgx, int orgy, int ss);

/* land.c */
extern int	FAR land_init (void);
extern void	FAR land_term (void);
extern int	FAR land_update (OBJECT *pov);

/* lnd.c */
extern int	FAR lnd_read (void);

/* log.c */
extern int	FAR log_init (void);
extern void	FAR log_term (void);
extern int	FAR log_flush (int now);
extern int	FAR LogPrintf (const char *fmt, ...);
extern void	FAR LogDumpHex (char *title, Uchar *h, int hlen);
extern int	FAR LogTrace (const char *file, int lineno);
#define	TRACE()	LogTrace (__FILE__, __LINE__)

/* loop.c */
extern void	FAR active_loop (Ulong t);
extern void	FAR screen_start (void);
extern void	FAR screen_empty (void);
extern void	FAR double_buffer (int mode);
extern void	FAR reset_page (int empty);
extern void	FAR repaint (void);
extern void	FAR pause_set (Ushort mode);

/* macros.c */
extern int	FAR mread (void);
extern int	FAR mgetch (void);
extern int	FAR mac_interpret (Ushort *keys, int len);
extern void	FAR mac_flush (void);
extern int	FAR mac_init (void);
extern void	FAR mac_term (void);

/* mat.c */
#if 1 /*was: ifdef INLINE*/
#define Mident(m) \
	(m[0][1] = m[0][2] = m[1][0] = m[1][2] = m[2][0] = m[2][1] = 0, \
	 m[0][0] = m[1][1] = m[2][2] = FONE)

#define Mxpose(m) \
	do { \
		xshort	zt; \
		zt = m[0][1];	m[0][1] = m[1][0];	m[1][0] = zt; \
		zt = m[0][2];	m[0][2] = m[2][0];	m[2][0] = zt; \
		zt = m[1][2];	m[1][2] = m[2][1];	m[2][1] = zt; \
	} while (0)
#else
extern void	FAR FASTCALL Mident (MAT m);
extern void	FAR FASTCALL Mxpose (MAT m);
#endif

#define	Mcopy(to,from)	memcpy (to, from, sizeof (MAT))
#define	Vcopy(to,from)	memcpy (to, from, sizeof (VECT))
#define	LVcopy(to,from)	memcpy (to, from, sizeof (LVECT))
#define	AVcopy(to,from)	memcpy (to, from, sizeof (AVECT))

extern void	FAR FASTCALL fMrotx (MAT m, xshort s, xshort c);
extern void	FAR FASTCALL fMroty (MAT m, xshort s, xshort c);
extern void	FAR FASTCALL fMrotz (MAT m, xshort s, xshort c);
#define Mrotx(m,d)	fMrotx (m, SIN(d), COS(d))
#define Mroty(m,d)	fMroty (m, SIN(d), COS(d))
#define Mrotz(m,d)	fMrotz (m, SIN(d), COS(d))

extern void	FAR Mobj (OBJECT *p);
extern void	FAR Myxz (MAT m, AVECT a);
extern void	FAR VxMmul (VECT R, VECT V, MAT M);
extern void	FAR Mmul (MAT m, MAT t);
extern void	FAR Vscale (VECT a, VECT b, int scalar);
extern void	FAR Vmuldiv (VECT a, VECT b, int m, int d);
extern void	FAR Mangles (OBJECT *p, MAT m, AVECT a, ANGLE dy);
extern void	FAR Euler (OBJECT *p);
extern void	FAR cbuild_matyxz (MAT T, int spitch, int cpitch, int sroll,
	int croll, int syaw, int cyaw);

/* some related macros */

#define	Vinc(to,from)	((to)[X]+=(from)[X], \
			 (to)[Y]+=(from)[Y], \
			 (to)[Z]+=(from)[Z])
#define	Vdec(to,from)	((to)[X]-=(from)[X], \
			 (to)[Y]-=(from)[Y], \
			 (to)[Z]-=(from)[Z])
#define	Vadd(to,a,b)	((to)[X]=(a)[X]+(b)[X], \
			 (to)[Y]=(a)[Y]+(b)[Y], \
			 (to)[Z]=(a)[Z]+(b)[Z])
#define	Vsub(to,a,b)	((to)[X]=(a)[X]-(b)[X], \
			 (to)[Y]=(a)[Y]-(b)[Y], \
			 (to)[Z]=(a)[Z]-(b)[Z])
#define VMRfmul(V,M,n) \
	(fmul(V[X],M[X][n]) + fmul(V[Y],M[Y][n]) + fmul(V[Z],M[Z][n]))

/* max.c */
extern int	FAR max_read (MACRO *Macros);

/* memory.c */
extern void *	FAR xmalloc (Uint size);
extern void *	FAR xcalloc (Uint size, Uint count);
extern char *	FAR xstrdup (const char *s);
extern void *	FAR xfree (void *block);
extern void *	FAR mem_alloc  (Uint bytes);
extern void *	FAR memd_alloc (Uint bytes, char *file, int lineno);
extern void *	FAR mem_free  (void *block, int bytes);
extern void *	FAR memd_free (void *block, int bytes, char *file, int lineno);
extern char *	FAR mem_strdup  (const char *s);
extern char *	FAR memd_strdup (const char *s, char *file, int lineno);
extern void *	FAR mem_strfree  (char *s);
extern void *	FAR memd_strfree (char *s, char *file, int lineno);
extern void	FAR mem_check (void);
extern int	FAR mem_init (void);
extern void	FAR mem_stats (void);
extern void	FAR mem_term (void);

#ifdef MEM_DEBUG

#define memory_alloc(n)		memd_alloc (n, __FILE__, __LINE__)
#define memory_free(b,n)	memd_free ((b), (n), __FILE__, __LINE__)
#define STRdup(s)		memd_strdup ((s), __FILE__, __LINE__)
#define STRfree(s)		memd_strfree ((s), __FILE__, __LINE__)

#else

#define memory_alloc(n)		mem_alloc (n)
#define memory_free(b,n)	mem_free ((b), (n))
#define STRdup(s)		mem_strdup (s)
#define STRfree(s)		mem_strfree (s)

#endif

#define memory_calloc(n,s)	memory_alloc ((n)*(s))
#define memory_cfree(b,n,s)	(memory_free ((b), (n)*(s)), (void *)NULL)

#define	NEW(p)	((p) = memory_alloc (sizeof (*(p))))
#define	DEL(p)	memory_free ((p), sizeof (*(p)))
#define	DEL0(p)	(DEL(p), (p) = 0)

/* menu.c */
extern char	FAR menuch[];
extern int	FAR menu_open (MENU *menu, int selected);
#define	MENU_ABORTED	-1
#define	MENU_FAILED	-2
extern void	FAR menu_close (void);
extern int	FAR hdd_menu (VIEW *view, int orgx, int orgy, int maxx,
	int maxy);
extern void	FAR show_menu (VIEW *view, int orgx, int orgy, int maxx,
	int maxy, int bss);
extern void	FAR get_num (char *name, void *value, int type, long vmin,
	long vmax, long vinc);
#define MT_CHAR		1
#define MT_SHORT	2
#define MT_INT		3
#define MT_LONG		4

#ifdef SHORT_IS_INT
#define MT_XSHORT	MT_INT
#else
#define MT_XSHORT	MT_SHORT
#endif

extern void	FAR SetOption (Ushort *i, Ushort mask);

/* menus.c */
extern int	FAR menu_top (void);
extern void	FAR set_screen (int sizex, int sizey);

/* message.c */
extern HMSG *	FAR msg_del (const HMSG *p);
extern HMSG *	FAR MsgPrintf (int ttl, const char *fmt, ...);
extern HMSG *	FAR MsgEPrintf (int ttl, const char *fmt, ...);
extern HMSG *	FAR MsgWPrintf (int ttl, const char *fmt, ...);
extern void	FAR msg_show (int orgx, int orgy, int maxx, int maxy, int bss);
extern void	FAR msg_clear (int hard);
extern int	FAR msg_init (void);
extern void	FAR msg_term (void);

/* msubs.asm */
extern int	FAR AFASTCALL fmula (int x, int y);
extern int	FAR AFASTCALL fdiva (int x, int y);
extern int	FAR AFASTCALL muldiva (int x, int y, int z);
extern int	FAR AFASTCALL dithadja (int x, int dither, int interval);
extern void	FAR Vmula (VECT R, VECT V, MAT M);
extern void	FAR matyxz (MAT T, int spitch, int cpitch, int sroll, int croll,
	int syaw, int cyaw);
extern xshort	FAR AFASTCALL my_sina (ANGLE angle);

/* nav.c */
extern int	FAR nav_init (void);
extern void	FAR nav_term (void);
extern int	FAR nav_find (char *name);
extern int	FAR ils_get (OBJECT *p);
extern void	FAR show_ils (HUD *h, OBJECT *p, int sx, int sy, int orgx,
	int orgy, int ss, int shifty);
extern int	FAR menu_ils (void);
extern struct ils FAR *ils;


/* need.c */
#ifdef NEED_STRICMP
extern int	FAR stricmp (const char *a, const char *b);
#endif
#ifdef NEED_STRNICMP
extern int	FAR strnicmp (const char *a, const char *b, size_t n);
#endif
#ifdef NEED_STRDUP
extern char *	FAR strdup (register const char *s);
#endif
#ifdef NEED_STRERROR
extern char *	FAR strerror (int n);
#endif
#ifdef NEED_LABS
extern long	FAR labs (long x);
#endif

/* netmgr.c */
extern int	FAR netports_init (void);
extern void	FAR netports_term (void);
extern void	FAR netports_receive (void);
extern void	FAR netports_poll (void);
extern void	FAR netport_count (PLAYER *pl, int delta);
extern int	FAR packet_deliver (PACKET *pack);
extern int	FAR packet_send (PACKET *pack, int mode);
extern PACKET *	FAR packet_new (short size, short hlen);
extern PACKET *	FAR packet_del (PACKET *pack);
extern void	FAR PlName (PLAYER *pl);
extern char *	FAR netport_name (int port);
extern char *	FAR netport_addr (int port, Uchar *addr);

/* nogr.c */
extern void	FAR NoEllipse (Uint x1, Uint y1, Uint rx, Uint ry, Uint color);
extern void	FAR NoClear (Uint x, Uint y, Uint sx, Uint sy, Uint color);
extern void	FAR NoPolygon (int npoints, BUFLINE *points, Uint color);

/* nokbd.c */
extern struct KbdDriver NEAR KbdNone;

/* nosound.c */
extern struct SndDriver NEAR SndNone;

/* nosystem.c */
extern struct SysDriver NEAR SysNone;

/* notimer.c */
extern struct TmDriver NEAR TmNone;

/* obasic.c */
extern void	FAR dynamics_basic (OBJECT *p, int action);

/* object.c */
extern OBJECT *	FAR create_object (ONAME name, int init);
extern OBJECT *	FAR create_land (ONAME name);
extern OBJECT *	FAR get_owner (OBJECT *object);
extern OBJECT *	FAR delete_object (OBJECT *object);
extern OBJECT *	FAR delete_land (OBJECT *object);
extern void	FAR list_clear (OBJECT *list[]);
extern void *	FAR shape_free (VERTEX *vx);
extern int	FAR shape_read (SHAPE *shape, char *VxFileName);

/* objects.c */
extern void	FAR object_rand (OBJECT *obj, int speed, int extent, int noise);
extern int	FAR object_break (int n, int speed, int extent, int noise);
extern void	FAR object_hit (OBJECT *obj, int seed, int speed, int extent,
	int damaging);
extern void	FAR object_update (OBJECT *p);
extern void	FAR object_dynamics (OBJECT *p);
extern int	FAR objects_dynamics (void);

/* obox.c */
extern BODY	FAR BoBox;

/* obroken.c */
extern BODY	FAR BoBroken;

/* ocar.c */
extern BODY	FAR BoCar;

/* ochute.c */
extern BODY	FAR BoChute;

/* oclassic.c */
extern void	FAR dynamics_classic (OBJECT *p, int action);

/* ocrater.c */
extern BODY	FAR BoCrater;

/* ofplane.c */
extern void	FAR dynamics_fplane (OBJECT *p, int action);

/* ogen.c */
extern int	FAR gen_init (BODY *b);
extern int	FAR gen_read (BODY *b);
extern void	FAR gen_term (BODY *b);
extern int	FAR gen_create   (OBJECT *p);
extern int	FAR gen_nocreate (OBJECT *p);
extern void	FAR gen_delete (OBJECT *p);
extern void	FAR gen_dynamics   (OBJECT *p);
extern void	FAR gen_nodynamics (OBJECT *p);
extern void	FAR gen_hit   (OBJECT *p, int speed, int extent, int damaging);
extern void	FAR gen_nohit (OBJECT *p, int speed, int extent, int damaging);

/* oground.c */
extern BODY	FAR BoGround;

/* ogtarget.c */
extern BODY	FAR BoGtarget;

/* ohouse.c */
extern BODY	FAR BoHouse;

/* olow.c */
extern BODY	FAR BoLow;

/* om61.c */
extern BODY	FAR BoM61;

/* omk82.c */
extern BODY	FAR BoMK82;

/* opaddoc.c */
extern int	FAR paddoc_init (BODY *b);

/* oplane.c */
extern void	FAR eject (OBJECT *obj);
extern void	FAR shoot (OBJECT *p, int weapon, int n, int seed,
	int interval);
extern void	FAR plane_smoke (OBJECT *p);
extern void	FAR place_plane (OBJECT *p, short home);
extern void	FAR emit_drone (void);
extern char *	FAR get_wname (int w);
extern void	FAR plane_xfer (OBJECT *p);
extern BODY	FAR BoPlane;

/* orunway.c */
extern BODY	FAR BoRunway;

/* osmoke.c */
extern BODY	FAR BoSmoke;

/* otarget.c */
extern BODY	FAR BoTarget;

/* otower.c */
extern BODY	FAR BoTower;

/* oviewer.c */
extern BODY	FAR BoViewer;

/* oxplane.c */
extern void	FAR dynamics_xplane (OBJECT *p, int action);

/* oyplane.c */
extern void	FAR dynamics_yplane (OBJECT *p, int action);

/* panel.c */
extern void	FAR show_panel (VIEW *view, OBJECT *p, int maxx, int maxy,
	int orgx, int orgy, int ss);

/* pid.c */
extern long	FAR pid_calc (F8PID *pid, long P, int interval);

/* piper.c */
extern void	FAR show_piper (HUD *h, OBJECT *obj, OBJECT *target, int x,
	int y, int dx, int dy, int ds, int mode, int off_screen, int dist,
	int tti, int closure, int orgx, int orgy, int clipx, int clipy,
	int hbottom, int hleft, int ss, int shifty);

/* pitch.c */
extern void	FAR get_cue (OBJECT *p);
extern void	FAR show_pitch (HUD *h, VIEW *view, OBJECT *p, int sx, int sy,
	int maxx, int maxy, int orgx, int orgy, int ttx, int tty, int tx,
	int ty, int ss, int shifty, int mode, int VVD[2]);

/* player.c */
extern int	FAR players_init (void);
extern void	FAR players_term (void);
extern void	FAR players_delete (void);
extern void	FAR players_remove (PLAYER *ptype);
extern void	FAR players_purge (void);
extern void	FAR players_flush (void);

extern PLAYER *	FAR player_add (PACKET *pack);
extern PLAYER *	FAR player_delete (PLAYER *player);
extern PLAYER *	FAR player_find (PACKET *pack);
extern PLAYER *	FAR player_next (PLAYER *player);
extern PLAYER *	FAR player_active (PACKET *pack);
extern void	FAR player_remove (PLAYER *player);

/* ptrmgr.c */
extern int	FAR pointers_init (void);
extern void	FAR pointers_term (void);
extern POINTER * FAR pointer_select (char *pname);
extern POINTER * FAR pointer_release (POINTER *ptr);
extern int	FAR pointer_read (POINTER *ptr, int transfer);
extern void	FAR std_key (POINTER *p, int key);
extern int	FAR menu_ptrs (void);
extern void	FAR do_btns (POINTER *p, char *btn, int size);
extern void	FAR do_bchar (POINTER *p, int bchar, int state);
extern int	FAR menu_btn (void);

/* radar.c */
extern void	FAR show_radar (HUD *h, VIEW *view, OBJECT *p, OBJECT *pov,
	int orgx, int orgy, int maxx, int maxy, int tx, int ty, int ss,
	int clipx, int clipy, int sx, int sy, int shifty, int VVD[2],
	int mode, int hon);
extern void	FAR show_data (OBJECT *obj, int datax, int datay, int detail,
	int knots, int dist, int closure, char *name, int tti, int mode,
	int ss, Uint color);
extern char *	FAR get_name (OBJECT *obj, OBJECT *target, int mode);
extern int	FAR get_center (OBJECT *p, int orgy, int sy, int VVD[2]);

/* remote.c */

#define RC_SENDOK	0x00
#define RC_PACKED	0xfe
#define RC_SENDFAIL	0xff

extern int	FAR remote_urhit (OBJECT *p, int speed, int extent,
	int damaging);
extern int	FAR remote_imhit (OBJECT *p, int seed, int speed, int extent,
	int damaging);
extern int	FAR crc (PACKET *pack);
extern int	FAR remote_init (void);
extern void	FAR remote_term (void);
extern int	FAR send_obj (OBJECT *p, PLAYER *player);
extern void	FAR remote_receive (OBJECT *p);
extern void	FAR remote_ping (void);
extern void	FAR remote_request (PLAYER *pl);
extern void	FAR remote_noplay (PLAYER *pl);
extern void	FAR remote_reply (PLAYER *pl, int reply);
extern void	FAR remote_shoot (OBJECT *p, int weapon, int n, int seed,
	int interval);
extern void	FAR remote_msg (char *text, PLAYER *pl);
extern void	FAR remote_time (PLAYER *pl);
extern void	FAR remote_refresh (void);

/* show.c */
extern void	FAR objects_show (int mode, VIEW *view, OBJECT *pov, LVECT OR,
	VECT RR);

/* sky.c */
extern int	FAR sky_init (void);
extern void	FAR sky_term (void);
extern void	FAR show_sky (VIEW *view, OBJECT *p);

/* sndmgr.c */
extern struct SndDriver NEAR* FAR sound_init (char *name);
extern void	FAR sound_term (void);

/* speed.c */
extern void	FAR show_speed (HUD *h, OBJECT *p, int sx, int sy, int maxx,
	int maxy, int orgx, int orgy, int ttx, int tty, int tx, int ty, int ss,
	int shifty, int VVD[2]);

/* stfont1.c */
extern char NEAR* NEAR StFont1[];

/* stfont2.c */
extern char NEAR* NEAR StFont2[];

/* stores.c */
extern void	FAR show_stores (VIEW *view, OBJECT *p, int maxx, int maxy,
	int orgx, int orgy, int ss);

/* stroke.c */
extern int	FAR font_set (int font);
extern void	FAR stroke_angle (ANGLE angle);
extern int	FAR stroke_decimal (int c);
extern int	FAR num_size (long num, int ss);
extern void	FAR num_extent (long num, int ss, int *exs, int *exc,
	int *eys, int *eyc);
extern int	FAR char_size (int Char, int size);
extern int	FAR stroke_size (char *s, int size);
extern int	FAR stroke_char (int x, int y, int digit, int size, int color);
extern int	FAR stroke_str (int x, int y, char *p, int size, int color);
extern int	FAR stroke_num (int x, int y, long num, int size, int color);
extern int	FAR frac_size (long num, int digits, int frac, int ss);
extern int	FAR stroke_frac (int x, int y, long num, int digits, int frac,
	int size, int color);

/* symbols.c */
extern void	FAR show_w (int x, int y, int tx, int ty, int color);
extern void	FAR show_x (int x, int y, int tx, int ty, int color);
extern void	FAR show_rect (int x, int y, int tx, int ty, int color,
	int showx);
extern void	FAR show_brect (int x, int y, int tx, int ty, int ratio,
	int color, int showx);
extern void	FAR show_diamond (int x, int y, int tx, int ty, int color,
	int showx);
extern void	FAR show_plus (int x, int y, int tx, int ty, int color);
extern void	FAR show_bplus (int x, int y, int tx, int ty, int ratio,
	int color);
extern void	FAR show_fpm (int x, int y, int rx, int ry, int tx, int ty,
	int color, int round);
extern void	FAR show_dir (int x, int y, int rx, int ry, int tx, int ty,
	int color);
extern void	FAR show_dir1 (int x, int y, int rx, int ry, int tx, int ty,
	int sa, int ca, int color,
	int orgx, int orgy, int sx, int sy, int shifty);
extern void	FAR show_ptr (int x, int y, int rx, int ry, int tx, int ty,
	int color, int round);
extern void	FAR show_trig (int x, int y, int dx, int dy, int color);

/* system.c */
extern void	FAR sim_set (void);
extern void	FAR sim_reset (void);
extern void	FAR sys_poll (int id);
extern void	FAR iefbr14 (void);

/* term.c */
extern void	FAR terminate (int StackUsed);
extern void	FAR die (void);

/* tunes.c */
extern int	FAR TnHello[];
extern int	FAR TnGone[];
extern int	FAR TnHit[];
extern int	FAR TnEngine[];
extern int	FAR TnAlarm[];
extern int	FAR TnWarn[];
extern int	FAR TnNotice[];
extern int	FAR TnGear[];
extern int	FAR TnMsg[];
extern int	FAR TnDamage[];
extern int	FAR TnBlues[];

/* util.c */
extern int	FAR opt36 (int c);
extern int	FAR get_long (char *p, long *lp);
extern int	FAR get_int (char *p, int *li);
extern char *	FAR get_parg (char *options, char *argname);
extern char *	FAR get_piarg (char *options, int argno);
extern int	FAR get_narg (char *options, char *argname, long *lp);
extern int	FAR get_niarg (char *options, int argno, long *lp);
extern char *	FAR get_sarg (char *options, char *argname);
extern char *	FAR get_siarg (char *options, int argno);
extern int	FAR looping (int i);
extern int	FAR Frand (void);
extern void	FAR Fsrand (Uint seed);
extern void	FAR Frandomize (void);
extern Uchar *	FAR show_l (long n);
extern Uchar *	FAR show_ul (Ulong n);
extern Uchar *	FAR show_ulf (Ulong n, Ulong f);
extern Uchar *	FAR show_time (char *title, Ulong tt);
extern Uint	FAR ComGBw (Uchar *p);
extern Uint	FAR ComGLw (Uchar *p);
extern void	FAR ComPBw (Uchar *p, Uint w);
extern void	FAR ComPBl (Uchar *p, Ulong l);
extern void	FAR ComPLw (Uchar *p, Uint w);

/* version.c */
extern char *	FAR welcome (int log);

/* views.c */
extern void	FAR get_area (VIEW *view, int *orgx, int *orgy, int *maxx,
	int *maxy);
extern void	FAR get_square (VIEW *view, int maxx, int maxy, int *sizex,
	int *sizey);
extern int	FAR scenery (int type);
extern OBJECT *	FAR get_viewer (int type);
extern void	FAR update_viewer (void);
extern void	FAR get_viewport (OBJECT *p);
extern void	FAR save_viewport (OBJECT *p);
extern void	FAR swap_extview (void);
extern void	FAR render_picture (void);
extern int	FAR menu_view (int type);
extern int	FAR hdd_menu (VIEW *view, int orgx, int orgy, int maxx,
	int maxy);

/* vmodes.c */
extern int	FAR vm_read (void);
extern void	FAR vm_free (void);

/* vv.c */
extern void	FAR show_vv (HUD *h, VIEW *view, OBJECT *p, int color);
extern void	FAR show_wl (HUD *h, OBJECT *p, int color);

/* waypoint.c */
extern void	FAR show_waypoint (HUD *h, VIEW *view, OBJECT *p);

/* window.c */
extern void	FAR windows_term (void);
extern void	FAR windows_set (void);
extern void	FAR set_screen (int sizex, int sizey);
extern void	FAR set_main (void);
extern void	FAR set_small_frame (void);
extern void	FAR set_large_frame (void);
extern void	FAR zoom (VIEWPORT *vp, int zoom);
extern int	FAR window_select (void);
extern int	FAR menu_windows (void);

/* $(SYSTEM)/ *.c
 * These must be supplied by the system dependent subdirectory.
*/
extern struct GrDriver	NEAR *FAR GrDrivers[];
extern struct SndDriver	NEAR *FAR SndDrivers[];
extern struct PtrDriver	NEAR *FAR PtrDrivers[];
extern struct KbdDriver	NEAR *FAR KbdDrivers[];
extern struct NetDriver	NEAR *FAR NetDrivers[];
extern struct SysDriver	NEAR SysDriver;
extern struct TmDriver	NEAR TmDriver;

/* Fixed point math macros. 'n' is number of fraction bits.
*/

#define	FnMUL(n,x,y)	((int)(((x) * (long)(y)) >> (n)))
#define	FnDIV(n,x,y)	((int)(((long)(x) << (n)) / (y)))
#define	FnCON(n,c)	((int)((1 << (n)) * (double)(c)))
#define	FnONE(n)	FnCON(n,1)

#define FSCALE		14		/* fraction bits in sine/cos etc. */
#define	FCON(c)		FnCON (FSCALE, (c))
#define	FONE		FnONE(FSCALE)
#define	fuscale(x)	((x) >> FSCALE)

#define VSCALE		4		/* fraction bits in v (speed) */
#define	VONE		FnONE(VSCALE)
#define DV(x)		((x)>>VSCALE)
#define	VMAX		0x7fff
#define	vscale(x)	((int)((x) * (long)VONE))
#define	vuscale(x)	((x) >> VSCALE)

#define	TADJ(x)		dithadj ((x), st.dither, st.interval)

#ifndef offsetof
#define offsetof(t,m)	((char *)&(((t *)0)->m) - (char *)0)
#endif

#endif /* ifndef FLY8_EXTERN */

