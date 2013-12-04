# Generated automatically from Makefile.in by configure.
# Hash library makefile, for TeenyMUD 2.x
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

srcdir = ../../hash
VPATH = ../../hash

SHELL = /bin/sh

CC=	gcc
RANLIB=	ranlib
CFLAGS=	${COPTS} -I.. -I${srcdir}/.. -I${srcdir}

OBJ=	Hash.o HashCSrch.o Hash_CEntry.o Hash_DEntry.o \
	Hash_DTable.o Hash_EFirst.o Hash_ENext.o \
	Hash_FEntry.o Hash_InitTab.o Hash_PStat.o List_Init.o \
	List_Insert.o List_LInst.o List_Move.o List_Remove.o \
	List_Verify.o

SRCS=	Hash.c HashCSrch.c Hash_CEntry.c Hash_DEntry.c \
	Hash_DTable.c Hash_EFirst.c Hash_ENext.c \
	Hash_FEntry.c Hash_InitTab.c Hash_PStat.c List_Init.c \
	List_Insert.c List_LInst.c List_Move.c List_Remove.c \
	List_Verify.c

HDRS=	${srcdir}/hash.h ${srcdir}/list.h ../autoconf.h ${srcdir}/../config.h

all:	hash.a

clean:
	-rm -f *.o hash.a

real-clean:
	-rm -r *.o hash.a Makefile

hash.a:	${P} ${OBJ}
	rm -f hash.a
	ar q hash.a `lorder ${OBJ} | tsort`
#	ar q hash.a ${OBJ}
	${RANLIB} hash.a

Hash.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
HashCSrch.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_CEntry.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_DEntry.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_DTable.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_EFirst.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_ENext.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_FEntry.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_InitTab.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
Hash_PStat.o: ${srcdir}/../config.h ${srcdir}/hash.h ${srcdir}/list.h
List_Init.o: ${srcdir}/../config.h ${srcdir}/list.h
List_Insert.o: ${srcdir}/../config.h ${srcdir}/list.h
List_LInst.o: ${srcdir}/../config.h ${srcdir}/list.h
List_Move.o: ${srcdir}/../config.h ${srcdir}/list.h
List_Remove.o: ${srcdir}/../config.h ${srcdir}/list.h
List_Verify.o: ${srcdir}/../config.h ${srcdir}/list.h
${srcdir}/hash.h: ${srcdir}/list.h
${srcdir}/../config.h: ../autoconf.h
