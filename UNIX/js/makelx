CFLAGS=	-m486 -O3 -fomit-frame-pointer -Wall # -DJS_V12=1
LDFLAGS= -s -N
CC=	gcc
PROGS=	js jscal
BINDIR=	/usr/local/bin
MODDIR=	/lib/modules
NEWVER=$(shell uname -a|cut -f 3 -d ' ')

# use this if you are using module vesions
#
MODVER= -DMODVERSIONS -include /usr/include/linux/modversions.h

all:	$(PROGS) joystick.o

js:	js.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ js.c

jscal:	jscal.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ jscal.c

joystick.o:	joystick.c /usr/include/linux/joystick.h
	$(CC) $(CFLAGS) -D__KERNEL__ -DMODULE $(MODVER) -c joystick.c

clean:
	rm -f *.o $(PROGS) *~

devs:
	-rm /dev/js0 /dev/js1
	mknod /dev/js0 c 15 0
	mknod /dev/js1 c 15 1

install:	all
	install -d /usr/local/bin /lib/modules/$(NEWVER)/misc
	install $(PROGS) /usr/local/bin
	install joystick.o /lib/modules/$(NEWVER)/misc

no_install: $(PROGS)
	@set -x ;for i in $(PROGS) ; \
	do \
		install -c $$i $(BINDIR) ; \
	done
