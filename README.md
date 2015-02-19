# pwld #
====

## The Per Window Layout Daemon ##

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

### How it works ###

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

## Example ##

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

