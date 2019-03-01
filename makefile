CC=gcc
CFLAGS = -g -Wall

iamroot: tcp.c udp.c utils.c
	$(CC) -o iamroot iamroot.c

