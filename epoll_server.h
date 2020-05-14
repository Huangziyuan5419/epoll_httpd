#include "wrap.h"
#include <sys/epoll.h>


int init_listen_fd(int, int);

void do_accept(int, int);

int get_line(int, char*, int);

void disconnect(int, int);

void do_read(int, int);

void send_respond_head(int, int, const char*, const char*, long);

void send_file(int, const char*);

void send_dir(int, const char*);

void http_request(const char*, int);

void epoll_run(int);
