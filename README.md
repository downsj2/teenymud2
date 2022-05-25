# Readme

NOTE: This is version of 2.0.9 of TeenyMUD, an update to the 2.0.5 releases.
It is intended to make TeenyMUD compile and run on modern systems.

It will almost certainly not work on any of the ancient systems it was
originally written on.

You will *not* be able to read in databases from an ancient system or
version 2.0.5; you *must* load your database from a text dump.  Failure to
do so will result in a non-functioning database.

My apologies for any bugs which remain.  I do believe that I've gotten the
bulk of them.

--Jason Downs (downsj@downsj.com), November/December 2013 & May 2022.

# Building

This release of TeenyMUD uses GNU Autoconf.  The configure script
will detect most aspects of the target system needed to compile the package.
To configure it in the source directory, simply type `./configure`.  To
configure and build in another directory, run the `configure` script from
that directory; this will create all the needed subdirectories and files.

After `configure` is done, you may want to take a look at the top
level Makefile, and make changes by hand.  Especially make sure `configure`
found the correct libraries for your system; sometimes it misses them,
and other times it finds ones you don't really want to use.

You can now type `make` (or the name of the make program you wish
to use) in order to build the base programs.

You will need to have the appropriate development package for `libgdbm`
installed in order to build the software.  For instance, you should run
`sudo apt install libgdbm-dev` on Ubuntu.

For information about running the software, please see the file `HOWTO.md`.
