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
# iamroot.o, udp.o, tcp.o, utils.o and interface.o:
#
iamroot:  iamroot.o udp.o tcp.o utils.o root_server.o interface.o queue.o
	$(CC) $(CFLAGS) -o iamroot iamroot.o udp.o tcp.o utils.o root_server.o interface.o queue.o

# To create the object file iamroot.o, we need the source
# files iamroot.c, udp.h, tcp.h, utils.h and interface,h:
#
iamroot.o:  iamroot.c tcp.h udp.h utils.h interface.h queue.h
	$(CC) $(CFLAGS) -c iamroot.c

# To create the object file udp.o, we need the source files
# udp.c and udp.h:
#
udp.o:  udp.c udp.h
	$(CC) $(CFLAGS) -c udp.c

# To create the object file tcp.o, we need the source files
# tcp.c and tcp.h:
#
tcp.o:  tcp.c tcp.h utils.h
	$(CC) $(CFLAGS) -c tcp.c

# To create the object file utils.o, we need the source files
# utils.c and utils.h:
#
utils.o:  utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# To create the object file root_server.o, we need the source files
# root_server.c and root_server.h:
#
root_server.o:  root_server.c root_server.h udp.h
	$(CC) $(CFLAGS) -c root_server.c

# To create the object file queue.o, we need the source files
# queue.c and queue.h:
#
queue.o:  queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

# To create the object file interface.o, we need the source files
# interface.c and interface.h:
#
interface.o:  interface.c interface.h utils.h queue.h
	$(CC) $(CFLAGS) -c interface.c

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean:
	$(RM) iamroot *.o *~

