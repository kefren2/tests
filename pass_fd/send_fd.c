#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

ssize_t
write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
    struct msghdr   msg;
    struct iovec    iov[1];

    union {
      struct cmsghdr    cm;
      char              control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr  *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmptr)) = sendfd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return(sendmsg(fd, &msg, 0));
}

int
get_unix_socket(char* filename)
{
  struct sockaddr_un addr;
  int fd;
  
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, filename, sizeof(addr.sun_path)-1);
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  listen(fd, 5);
  
  return fd;
}

/* connect to google.com */
int
get_new_tcp_socket()
{
  struct addrinfo hints, *res, *res0;
  int s;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  assert(getaddrinfo("www.google.com", "http", &hints, &res0) == 0);
  for (res = res0; res; res = res->ai_next) {
    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    assert(s>=0);
    if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
      close(s);
      continue;
    }
    freeaddrinfo(res0);
    return s;
  }
  freeaddrinfo(res0);
  assert(1 && "cannot connect to google");
  return -1;
}

int
main()
{
  int fd = get_unix_socket("passing"), x = 1, peer, h;
  char query[] = "GET http://www.google.com/ HTTP/1.1\nHost: www.google.com\n\n";
  
  peer = accept(fd, NULL, NULL);
  // push a query to google
  h = get_new_tcp_socket();
  write(h, query, sizeof(query));
  // and pass the fd to the client
  write_fd(peer, &x, sizeof(x), h);
  
  sleep(1);
  close(peer);
  close(fd);
  close(h);
  
  unlink("passing");
  return 0;
}