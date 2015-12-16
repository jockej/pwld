

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

struct pidfh {
  int fd;
  const char* path;
};

struct pidfh *pidfile_open(const char* name, int mode, void *whatever) {
  int fd;
  struct pidfh * ret;
  fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (fd == -1) return NULL;

  ret = (struct pidfh*)malloc(sizeof(struct pidfh));
  if (!ret) return NULL;
  ret->fd = fd;
  ret->path = name;
  return ret;
}

#define PID_SIZE 10
void pidfile_write(struct pidfh *f) {
  pid_t pid = getpid();
  char buf[PID_SIZE];

  memset(buf, '\0', PID_SIZE);
  snprintf(buf, PID_SIZE, "%d", pid);
  size_t s = strnlen(buf, PID_SIZE);
  write(f->fd, buf, s);
  close(f->fd);
}

void pidfile_remove(struct pidfh * f) {
  close(f->fd);
  unlink(f->path);
}
  







