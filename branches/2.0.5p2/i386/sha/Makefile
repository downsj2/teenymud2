# Generated automatically from Makefile.in by configure.
# sha library of routines used in TeenyMUD II.
#
#		       This file is part of TeenyMUD II.
#	    Copyright(C) 1995 by Jason Downs.  All rights reserved.
# 
# TeenyMUD II is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# TeenyMUD II is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program (see the file 'COPYING'); if not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA.
#
# NOTE: This code is good at breaking optimizing compilers.

srcdir = ../../sha
VPATH = ../../sha

SHELL = /bin/sh

CC=	gcc
RANLIB=	ranlib
CFLAGS=	${COPTS} -I.. -I${srcdir}/.. -I${srcdir}

OBJ=	sha.o sha_wrap.o

SRCS=	sha.c sha_wrap.c

HDRS=	${srcdir}/sha.h ${srcdir}/sha_wrap.h ../autoconf.h \
	${srcdir}/../config.h

all:	sha.a

clean:
	-rm -f *.o sha.a

real-clean:
	-rm -r *.o sha.a Makefile

sha.a:	${P} ${OBJ}
	rm -f sha.a
	ar q sha.a `lorder ${OBJ} | tsort`
#	ar q sha.a ${OBJ}
	${RANLIB} sha.a

sha.o: ${srcdir}/../config.h ${srcdir}/sha.h
sha_wrap.o: ${srcdir}/../config.h ${srcdir}/sha.h ${srcdir}/sha_wrap.h
${srcdir}/sha_wrap.h: ${srcdir}/../config.h
${srcdir}/../config.h: ../autoconf.h
