/*
    pwld, the per window layout daemon
    Copyright (C) 2014, 2015 Joakim Jalap

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// I don't even remember what this was about...
#ifdef __Linux__
#define _XOPEN_SOURCE
#endif

#include <cstdio>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <vector>
#include <csignal>
#include <string>
#include <string.h>
#include <X11/XKBlib.h>
#include <map>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

#ifdef HAVE_FREEBSD_PIDFILE
#ifdef __Linux__
#include <bsd/libutil.h>
#elif defined(__FreeBSD__)
#include <util.h>
#endif
#else
#include "pidfile.h"
#endif

#ifdef __OpenBSD__
#include <errno.h>
#endif

#include "../config.h"

#define INFILE      "/tmp/pwld.in"
#define OUTFILE     "/tmp/pwld.out"
#define PIDFILE     "/tmp/pwld.pid"

#define MAX_GRP_LEN 4  // the max length of a group including ending newline
#define MIN_GRP_LEN 3  // the min length of a group including ending newline

using namespace std;

int               curgrp    = 5; // current group
Window            curwin;       // current window
map<Window, int> *wingrp;       // map win -> group
unsigned          ngrp;         // number of groups
vector<string*>  *grpnames;     // names of groups
Display          *dsp;          // display
Atom              aw;           // _NET_ACTIVE_WINDOW atom
Atom              cl;           // _NET_CLIENT_LIST atom
Window            root;         // the root window
int               thesig;       // memory for the signal caught
bool              daemonize = false;
FILE             *outfile;      // the current layout is in this file
int logmask;
int x_fd;                       // The X servers fd
int sig_fds[2];                 // fds for the signal handler
struct pidfh     *pfh;


/*!
 * Logs \a msg to the syslog. If \a quit is true
 * calls exit(3);
 */
static void errlog(const char *msg, bool quit) {
  syslog(LOG_DEBUG, msg);
  if (quit) {
    printf("exiting\n");
    syslog(LOG_DEBUG, "Can't deal with that, exiting");
    exit(EXIT_FAILURE);
  }
}

/*!
 * Catch signal \a sig and write it to the write
 * end of sig_fds.
 */
static void catchsig(int sig) {
  thesig = sig;
  write(sig_fds[1], &thesig, sizeof(int));
}

/*!
 * Initializes grpnames with values from the
 * command line. Returns true on success.
 */
static bool parse_layout(char *layouts) {
  char *l;
  grpnames = new vector<string*>();
  for (l = strtok(layouts, ","); l; l = strtok(NULL, ","))
    grpnames->push_back(new string(l));
  return (ngrp = grpnames->size()) > 1;
}

static void print_version() {
  printf("The Per Window Layout Daemon, version " VERSION ".\n"\
         "Please report any bugs to " PACKAGE_BUGREPORT ".\n");
  exit(EXIT_SUCCESS);
}

/*!
 * Parse the command line options.
 */
static void parse_cmdl(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "vdl:")) != -1) {
    switch(opt) {
    case 'v':
      print_version();
    case 'd':
      daemonize = true;
      break;
    case 'l':
      if (parse_layout(optarg)) break;
    default:
      goto fail;
    }
  }
  if (grpnames) return;
 fail:
  fprintf(stderr, "Usage: %s -l layouts [-d]\n", argv[0]);
  exit(EXIT_FAILURE);
}

/*!
 * Sets the current layout to \a grp, and
 * writes its name to OUTFILE.
 */
static void setgrp(int grp) {
  if (grp == curgrp) return;
  string *name = grpnames->at(grp);
  size_t len = name->size();
  curgrp = grp;
  if (!XkbLockGroup(dsp, XkbUseCoreKbd, grp)) {
    errlog("failed to send lock group request.\n", false);
  }
  ftruncate(fileno(outfile), len);
  fwrite(name->c_str(), 1, len, outfile);
  fwrite("\n", 1, 1, outfile);
  rewind(outfile);
}

/*!
 * Closes all files.
 */
static void clean() {
  syslog(LOG_DEBUG, "Cleaning up and exiting");
  closelog();
  unlink(INFILE);
  fclose(outfile);
  unlink(OUTFILE);
  pidfile_remove(pfh);
}

/*!
 * Initializes all the resources necessary.
 * This includes getting a connection to the
 * X display, setting up the signal handler,
 * allocating memory and opening files.
 */
static void init() {
  // init the x stuff
  int a,b,c,d,e;
  c = XkbMajorVersion;
  d = XkbMinorVersion;
  dsp = XkbOpenDisplay(NULL, &a, &b, &c, &d, &e);
  if (!dsp) errlog("Could not get display", true);
  root = XDefaultRootWindow(dsp);
  XSelectInput(dsp, root, PropertyChangeMask);
  aw = XInternAtom(dsp, "_NET_ACTIVE_WINDOW", True);
  cl = XInternAtom(dsp, "_NET_CLIENT_LIST", True);
  x_fd = ConnectionNumber(dsp);

  wingrp = new map<Window, int>();

  // file descriptors
  pipe(sig_fds);

  // signals
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGUSR2);
  struct sigaction sa;
  sa.sa_handler = catchsig;
  sa.sa_mask = ss;
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);

  // logging
  openlog("pwld", logmask, LOG_USER);

  atexit(clean);
  pfh = pidfile_open(PIDFILE, 0600, NULL);
  outfile = fopen(OUTFILE, "w+");
}

/*!
 * Set the next layout as the active one.
 */
static void setnext(void) {
  int newgrp = (curgrp + 1) % ngrp;
  wingrp->at(curwin) = newgrp;
  setgrp(newgrp);
}

/*!
 * Sets the active layout to the one
 * stored for \a actwin.
 */
static void setcur(Window actwin) {
  if (actwin == curwin) return;
  curwin = actwin;
  map<Window, int>::iterator it;
  if ((it = wingrp->find(curwin)) != wingrp->end()) {
    setgrp(it->second);
  } else {
    wingrp->insert(make_pair(curwin, 0));
    setgrp(0);
  }
}

/*!
 * Gets the active window.
 */
static Window get_aw() {
  Atom ret;
  int format;
  unsigned long int n, bytes;
  unsigned char *buf;

  if (XGetWindowProperty(dsp, root, aw, 0, ~0,
                     False, AnyPropertyType,
                     &ret, &format, &n,
                         &bytes, &buf) != Success
      || n == 0) errlog("Couldn't get active window\n", false);
  Window w = *(Window*) buf;
  XFree(buf);
  return w;
}

/*!
 * Gets the client list from the root window and
 * synchronizes the wingrp map with it.
 */
static void cl_change() {
  Atom ret;
  int format;
  unsigned long int n, bytes;
  unsigned char *buf;

  if (XGetWindowProperty(dsp, root, cl, 0, ~0,
                     False, AnyPropertyType,
                     &ret, &format, &n,
                         &bytes, &buf) != Success
      || n == 0) errlog("Couldn't get client list\n", false);
  Window *list = (Window*) buf;
  sort(list, list + n);
  auto it = wingrp->begin();
  unsigned long i = 0;
  for (; i < n && it != wingrp->end(); i++, ++it) {
    if (it->first != list[i]) {
      it = wingrp->insert(it, make_pair(list[i], 0));
      ++it;
    }
  }
  if (i == n) wingrp->erase(it, wingrp->end());
  else for (; i < n; i++) wingrp->insert(make_pair(list[i], 0));
  XFree(buf);
}

// Class to see if a string is the grpnames array.
class isgroup {
  const char * cmp;
public:
  isgroup(const char * const cmp) : cmp(cmp) {}
  inline bool operator()(string*& str) const {
    return str->compare(cmp) == 0;
  }
};

/*!
 * Reads the name of a layout from INFILE and sets that as the
 * current layout.
 */
static void readnext() {
  char buf[MAX_GRP_LEN];
  int idx, n, infile;
  truncate(INFILE, MAX_GRP_LEN);
  if ((infile = open(INFILE, O_RDONLY)) == -1)
    errlog("Couldn't open INFILE\n", false);
  if ((n = read(infile, &buf, MAX_GRP_LEN)) < MIN_GRP_LEN) return;
  close(infile);
  char* b;
  for (b = buf; b - buf < MAX_GRP_LEN && *b != '\n'; b++);
  *b = '\0';
  auto it = find_if(grpnames->begin(), grpnames->end(), isgroup(buf));
  if (it == grpnames->end()) return;
  idx = it - grpnames->begin();
  if (n != -1) setgrp(idx);
}

/*!
 * Reads what kind of event \a ev is, and takes appropriate
 * action.
 */
static void handle_xevents(XEvent *ev) {
  Window actwin;
  if (ev->xany.window != root) return;
  if (ev->xproperty.atom == cl) {
    cl_change();
  } else if (ev->xproperty.atom == aw) {
    actwin = get_aw();
    setcur(actwin);
  }
}

/*!
 * The main loop that listens for an event on either
 * X_fd or the read end of the sig_fds pipe. The reason
 * we need to do this is that we cannot call X functions
 * from a signal handler, but we need to be able to
 * interrupt XWindowEvent to set the new layout.
 * So what we do is "extract" the fd from X, so that
 * we can listen on _both_ that and a fd of our own.
 * That way we can know which signal interrupted
 * XWindowEvent.
 */
static void main_loop() {
  int nfds = max(x_fd, sig_fds[0]) + 1;
  fd_set fds;
  while (true) {
    XEvent ev;
    FD_ZERO(&fds);
    FD_SET(x_fd, &fds);
    FD_SET(sig_fds[0], &fds);
    // take care of any xevents which are not waiting on x_fd
    // but are queued in memory.
    if (XPending(dsp)) {
      // XWindowEvent really should work, since we did XSelectInput
      // with the root window as argument, but, well, things aren't
      // always the way you want them to be, so we take all XEvents
      // and throw away those which did not originat from root.
      // XWindowEvent(dsp, root, PropertyChangeMask, &ev);
      XNextEvent(dsp, &ev);
      handle_xevents(&ev);
    } else {
      if (select(nfds, &fds, NULL, NULL, NULL) == -1 && errno != EINTR)
        errlog("select failed\n", true);
      if (FD_ISSET(sig_fds[0], &fds)) {
        int sig;
        read(sig_fds[0], &sig, sizeof(int));
        switch(sig) {
        case SIGUSR1:
          setnext(); break;
        case SIGUSR2:
          readnext(); break;
        case SIGTERM:
          exit(EXIT_SUCCESS);
        default: errlog("caught unknown signal\n", true);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  parse_cmdl(argc, argv);
  logmask = daemonize ? LOG_PID : LOG_PID | LOG_PERROR;
  init();
  setgrp(0);
  cl_change();
  curwin = get_aw();
  if (daemonize && daemon(1, 0) != 0) errlog("Failed to daemonize\n", true);
  pidfile_write(pfh);
  main_loop();
  return 0;
}
