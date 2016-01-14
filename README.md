# pwld
====

## The Per Window Layout Daemon

`pwld` is a very simple X daemon which keeps track of the
keyboard layout per window. It does this by listening to
events on the root window. It attempts to be as simple as
possible. Unix signals and files are used to
communicate with the daemon.

It is a simple little program, written by a simple little man.
The error handling is practically non existent, the interface is
brittle (for example the layouts specified on the command line
need to be in the same order as when they were given to setxkbmap(1)),
the communication is done via files and signals instead of
for example dbus.
But it seems to work for my simple needs.

### How it works

`pwld` relies on keyboard layouts having been set already.
A good way to do that is with `setxkbmap(1)`. For example:

```
setxkbmap -layout "us,se"
```

You would the start `pwld` like so:

```
pwld -l us,se -d
```

When started `pwld` will write its `pid` to the file
`/tmp/pwld.pid`. 

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

## Example

In my `~/.xprofile` I have the following:

```
setxkbmap -option "" -layout "us,se" -option ctrl:swapcaps
pwld -l us,se -d
```

And in my `~/.i3/config` I have this:

```
bindsym $mod+m exec kill -s USR1 `cat /tmp/pwld.pid`
```

Lastly, in my `~/.conkyrc` I have this:

```
${execp cat /tmp/pwld.out}
```
## Building and installing

This program uses the autotools, so the build/install procedure is pretty much
the usual `./configure`, `make`, `make install`. Details follow.

### Arch Linux

On Arch Linux you can use the PKGBUILD file to make the package. If you don't
know how, read all about it on the Arch Wiki.

This program depends on the libbsd package, for the pidfile handling, so you
need to have that installed. The PKGBUILD file mentioned above will take care of
that for you.

If you already have it installed the usual:

```
$./configure --prefix=/usr
$make
\#make install
```

will do.

### OpenBSD

On OpenBSD you need to configure the package like this in order to find the X
libraries:

```
CPPFLAGS=-I/usr/X11R6/include LDFLAGS=-L/usr/X11R6/lib ./configure --prefix=/usr/local
```

Then it's just `make` and `make` install as usual.

On OpenBSD `pwld` will use my own implementation of the pidfilef functions form
FreeBSD, these are probably not very good, but they seem to work. (The reason
for this is that OpenBSD instead provides the `pidfile(3)` function. This
however puts the pidfile in `/var/run`, which is only writeable by root, so that
means it can't be used in a user program, which kinda sucks.)

### FreeBSD

I haven't actually tried to build this on FreeBSD, so I can't in good conscience
say it works. But I think it should work. YMMV.
