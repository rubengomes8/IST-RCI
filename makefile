#
# This is an example Makefile for a countwords program.  This
# program uses both the scanner module and a counter module.
# Typing 'make' or 'make count' will create the executable file.
#

# define some Makefile variables for the compiler and compiler flags
# to use Makefile variables later in the Makefile: $()
#
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#
# for C++ define  CC = g++
CC = gcc
CFLAGS  = -g -Wall

# typing 'make' will invoke the first target entry in the file
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: iamroot

# To create the executable file iamroot we need the object files
# iamroot.o, udp.o, tcp.o and utils.o:
#
iamroot:  iamroot.o udp.o tcp.o utils.o
	$(CC) $(CFLAGS) -o iamroot iamroot.o udp.o tcp.o utils.o

# To create the object file iamroot.o, we need the source
# files iamroot.c, udp.h, tcp.h and utils.h:
#
iamroot.o:  iamroot.c tcp.h udp.h utils.h
	$(CC) $(CFLAGS) -c iamroot.c

# To create the object file udp.o, we need the source files
# udp.c and udp.h:
#
udp.o:  udp.c udp.h
	$(CC) $(CFLAGS) -c udp.c

# To create the object file tcp.o, we need the source files
# tcp.c and tcp.h:
#
tcp.o:  tcp.c tcp.h
	$(CC) $(CFLAGS) -c tcp.c

# To create the object file utils.o, we need the source files
# utils.c and utils.h:
#
utils.o:  utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) count *.o *~

