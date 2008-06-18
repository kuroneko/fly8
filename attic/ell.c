void symmetry(x,y)
int x,y;
{
	PUT_PIXEL(+x,+y);	/* This would obviously be inlined! */
	PUT_PIXEL(-x,+y);	/* and offset by a constant amount */
	PUT_PIXEL(-x,-y);
	PUT_PIXEL(+x,-y);
}

void
bresenham_ellipse_original (int a, int b)
{
	int	x, y, a2, b2, S, T;

	a2 = a*a;
	b2 = b*b;
	x  = 0;
	y  = b;
	S  = a2*(1-2*b) + 2*b2;
	T  = b2 - 2*a2*(2*b-1);

	symmetry (x, y);
	do {
		if (S < 0) {
			S += 2*b2*(2*x+3);
			T += 4*b2*(x+1);
			x++;
		} else if (T < 0) {
			S += 2*b2*(2*x+3) - 4*a2*(y-1);
			T += 4*b2*(x+1) - 2*a2*(2*y-3);
			x++;
			y--;
		} else {
			S -= 4*a2*(y-1);
			T -= 2*a2*(2*y-3);
			y--;
		}
		symmetry (x, y);
	} while (y > 0);
}

#define incx()		(x++, xx1 += dx, xx2 += dx)
#define incy()		(y--, yy1 -= dy, yy2 -= dy, yy -= _GrSizeX)

void
bresenham_ellipse_1 (int a, int b)
{
	int	x, y;
	long	S, T, xx1, xx2, yy1, yy2;
	long	a2, b2, dx, dy;			/* temps */
	long	yy;

	a2 = a*a;
	b2 = b*b;
	S  = a2*(1-2*b) + 2*b2;
	T  = b2 - 2*a2*(2*b-1);
	dx = 4*b2;
	dy = 4*a2;

	x = 0;
	y = b;
	xx2 = x*dx + dx;
	xx1 = xx2  + 2*b2;
	yy1 = y*dy - dy;
	yy2 = yy1  - 2*a2;

	yy = pixel address of (x, y);

	symmetry (x, y);
	do {
		if (S < 0) {
			S += xx1;
			T += xx2;
			incx ();
		} else if (T < 0) {
			S += xx1 - yy1;
			T += xx2 - yy2;
			incx ();
			incy ();
		} else {
			S -= yy1;
			T -= yy2;
			incy ();
		}
		symmetry (x, y);
	} while (y > 0);
}
