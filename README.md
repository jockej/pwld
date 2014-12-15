* pwld *
====

** The Per Window Layout Daemon **

`pwld` is a very simple X daemon which keeps track of the
keyboard layout per window. It does this by listening to
events on the root window. It attempts to be as simple as
possible. Simple Unix signals and files are used to
communicate with the daemon.

`pwld` relies on keyboard layouts having been set already.
A good way to do that is with `setxkbmap`.

`pwld` will write its `pid` to the file `/tmp/pwld.pid`.

There are two ways to change the layout for a window. To
set the next layout, send `SIGUSR1` to the daemon, like so:

```
kill -s SIGUSR1 `cat /tmp/pwld.pid`
```

The other way is to set a named layout by writing it to
the file `/tmp/pwld.in` and sending `SIGUSR2`, like so:

```
echo se > /tmp/pwld.in
kill -s SIGUSR2 `cat /tmp/pwld.pid`
```

This feature is mainly made to be used by other programs,
for example Emacs.

To make `pwld` exit cleanly and remove all its files,
send a `SIGTERM`.

** Example **









