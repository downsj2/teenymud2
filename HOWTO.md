# How to run the server

Assuming that you've successfully built the `teenymud` and
`convertdb` binaries, you should pick a directory from which the server
is going to run.  You should copy the binaries into that directory,
along with the file `sample.conf`, which is a sample server configuration
file.

The configuration directives contained in the file are fairly
self explanatory, and you should either experiment with them, read the
source code to find out exactly what they do, or simply use the defaults
in the sample.  You should, however, rename the file to the name of your
MUD (with the `.conf` extension), and change the name setting in the file.

The default directory layout uses the following directories, which
you should create:
- database: Where the running database is kept.
- text: Where the text files and messages are kept.
- logs: Where the server logs are kept.
- files: Where server accessible files are kept.

The only directory you need to worry about initially is the
`text' directory.  The files in here determine what the server sends
to new connections, when it goes down, etc.  The default files are:
- help.txt: The help file.  The one distributed in the source directory should be used.
- news.txt: The news file.  This is a per-server file for keeping local notices and the like; it is in the same format as the help file.
- motd.txt: Displayed to players after logging in.
- wiznews.txt: Displayed to wizards after logging in.
- greet.txt: Displayed to all new network connections.
- welcome.txt: Displayed to new players.
- register.txt: Displayed when a player creation from the greet screen is rejected because `registration' is required.
- goodbye.txt: Displayed as a player disconnects.
- shutdown.txt: Displayed as the server shuts down.
- guest.txt: Displayed when GUESTs log in.
- timeout.txt: Displayed when a player idles out.
- badconnect.txt: Displayed when a login attempt fails.
- badcreate.txt: Displayed when a creation attempt fails.

A new database is created by giving `convertdb` the `--initialize-db`
argument.  Since both `teenymud` and `convertdb` always require a
configuration file, the command would be:
`convertdb --initialize-db --configuration mudname.conf`
It should complete in a few seconds, and you should be able to
then start up the server.  The most common way of doing that is:
`teenymud --configuration mudconf.conf --detach`
That causes the server to read your configuration file, and then detach
from the tty as it starts up.

The initial player name is "Wizard", with a password of
"potrzebie".  YOU SHOULD CHANGE THAT PASSWORD.  The initial room is
"Limbo", which is the root room in the default configuration.  If you're
using the sample configuration, room #2 is the starting location of all
new players, and should be the first object you create (using @dig).

From here on out, you're on your own.  Read the help file for
more information, or check on GitHub.

# How to use converdb

If you're upgrading from a previous release of TeenyMUD, you must
dump your database out in plain text format using the old software.
Then you should have a file that `convertdb` will parse.  For loading
in a text database, you should use the `--input-file` command line
argument, following by the name of the text file.  So, an example command
line would be:
`convertdb --input-file textdb.file --configuration mudname.conf`
It will then automatically attempt to determine the format of the database,
read it in, and make any needed changes in order to get it to work with
TeenyMUD II.  There are couple of special exceptions, though.

For TeenyMUD 1.3, you should either give the `--xibo-compat`
command line option, or answer the question the program asks.  If, for
some odd reason, you have a TeenyMUD 1.1-C database file, use the
`--nonstandard` command line option.

Special note: if you're upgrading from a pre-1.4 database format,
the `convertdb` program will ask if you'd like for it to create a Root
Room.  Answering yes will cause it to create one, and then use it when
it sets the Room Location while upgrading the database.  If you answer
no, it will use whatever the root room setting in the configuration is.

With some exceptions, this release can read any text database
produced by any version of TeenyMUD ever done by Jason Downs.  If the
database uses UNIX-style crypt(3) for passwords, though, your passwords
won't work.  (Compile with UNIX_CRYPT defined in order to invisibly
support old fashioned passwords.)

Post 1.4 databases should load in fine, except perhaps for some
flags.  (I no longer have source code for database format version 201,
and I know 300 reused some of its flag values.)

Version 2.0.4 includes minimal support for reading in _vanilla_
TinyMUD databases directly with the `convertdb` program.  It can _not_
read in TinyMUSH, MUSE, MUCK, or any other sundry non-TeenyMUD format.

To dump a TeenyMUD II database into a text file, use the
`--output-file` command line option, instead of `--input-file`.  The
text database will then be written to the specified file.

Do note that if you're moving between different machines that are of
different architectures, you should always write your database out to text,
and then read it back in on the new machine.  Working database files will
usually not work between different architectures and operating systems.
It's also a good idea to simply back your database up to text periodically,
in case the database files get corrupted somehow.
