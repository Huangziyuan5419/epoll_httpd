#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>



void perr_exit(const char *);

int Accept(int, struct sockaddr *, socklen_t *);

int Bind(int, const struct sockaddr *, socklen_t );

int Connect(int, const struct sockaddr *, socklen_t );

int Listen(int, int);

int Socket(int, int, int);

ssize_t Read(int, void *, size_t);

ssize_t Write(int, void *, size_t);

int Close(int);

ssize_t Readn(int, void *, size_t);

ssize_t Writen(int, void *, size_t);