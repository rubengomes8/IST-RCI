#ifndef RCI_INTERFACE_H
#define RCI_INTERFACE_H

#include "root_server.h"

void interface(int fd_rs, struct addrinfo *res, char* streamID, int is_root, char * ipaddr, char* uport, char* tport,
        int tcp_sessions, int tcp_occupied, int fd_udp);
int read_terminal(int fd_rs, struct addrinfo *res_rs, char *streamID, int is_root, char *ipaddr, char* uport, char* tport,
                   int tcp_sessions, int tcp_occupied);

#endif //RCI_INTERFACE_H
