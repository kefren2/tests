#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ssize_t
read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{
    struct msghdr   msg;
    struct iovec    iov[1];
    ssize_t         n;
    int             newfd;

	assert(fd > 1 && "invalid fd");

    union {
      struct cmsghdr    cm;
      char              control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr  *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if ( (n = recvmsg(fd, &msg, 0)) <= 0)
        return n;

    if ( (cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
        cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
        assert(cmptr->cmsg_level == SOL_SOCKET && "control level != SOL_SOCKET");
        // it's triggered on OSX
        // assert(cmptr->cmsg_type != SCM_RIGHTS && "control type != SCM_RIGHTS");
        *recvfd = *((int *) CMSG_DATA(cmptr));
    } else
        *recvfd = -1;       /* descriptor was not passed */
    return n;
}

int
get_unix_socket(char *filename)
{
  struct sockaddr_un addr;
  int fd;
  
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, filename, sizeof(addr.sun_path)-1);
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  assert(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != -1 && "Cannot connect");
  
  return fd;
}

int
main()
{
  int fd = get_unix_socket("passing"), h, x;
  ssize_t bytes;
  char buf[2048];
  
  assert(read_fd(fd, &x, sizeof(x), &h) > 0 && "Couldn't read the socket");

  bytes = read(h, buf, sizeof(buf) - 1);
  buf[bytes] = '\0';
  printf("Read %zd bytes\n===========\n%s", bytes, buf);
    
  close(fd);
  close(h);
}
