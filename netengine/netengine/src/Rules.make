#rules of makefile 
#if this file is modified 
#please use 
#>make clean 
#>make
#to make all 

#debug flag 
DEBUG =y

#define uincode
UNICODE=n

#define user intel c compile
USE_INTEL_CC=n

#ifeq ($(OSTYPE),linux)
#	CFLAGS += -D__LINUX__
#elifeq ($(OSTYPE),linux-gnu)
#	CFLAGS += -D__LINUX__
#else
#	CFLAGS += -D__BSD__
#endif

CFLAGS += -c -D__LINUX__  -DSINGLE_THREAD_MOD
LFLAGS +=  -lpthread -lrt

ifeq ($(DEBUG),y)
	CFLAGS +=  -g -DDEBUG -DDEBUG_WITH_GDB -DNE_DEBUG -O0
#	LFLAGS += -
else
	CFLAGS += -DNEEBUG -O2
endif

ifeq ($(UNICODE),y)
	CFLAGS += -DNE_UNICODE
else
endif


TOPDIR = ../..
CURDIR = .
SRCDIR = $(CURDIR)/src
OBJDIR = $(CURDIR)/obj
OUTDIR = $(TOPDIR)/obj

CFLAGS += -I$(TOPDIR)/../testne/include

ifeq ($(DEBUG),y)
WORKDIR = $(TOPDIR)/bin
LIBDIR = $(TOPDIR)/lib
else
WORKDIR = $(TOPDIR)/bin
LIBDIR = $(TOPDIR)/lib
endif

ifeq ($(USE_INTEL_CC),y)
	CC = icc 
	CPP = i++
	AR = xiar rcs
	LFLAGS += /usr/intel/cc/lib/libimf.a 
else	
	CC = gcc 
	CPP = g++
	AR = ar  rv
	LFLAGS += -lm 
endif

