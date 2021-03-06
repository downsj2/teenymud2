### Changes since TeenyMUD 2.0beta1:
	- Added `--with-gdbm' and `--with-bsddbm' configuration options,
	  and cleaned up the Makefiles.  Still needs more work, though.
	- Added INHERIT flag.  Objects can now only force their owner if
	  they (the object) is set INHERIT.  INHERIT objects also have
	  Wizard privs if their owner does.  INHERIT can only be set by
	  Players who own themselves.
	- Added TIME specification to configuration system.  Allows interval
	  settings and the like to be specified in seconds, minutes, hours,
	  or days.
	- Dump newp_file to players that've never logged in before.
	- Add the endian check back to configure.
	- Fix EXTERNAL exit (command) matching.
	- Added /all, /no_space switches to @EDIT, added $ and ^ tokens.
	- Changed signal initializations.
	- Converted variable argument functions over to ANSI.
	- Added an snprintf() implementation to the compat library, and
	  switched many calls to sprintf() in the server over to it.
	- Don't try to run queued commands for objects that don't exist.
	- If a player is set INHERIT, then it's the same as if all of their
	  objects were set INHERIT.
	- Added <> forced excution command parsing.
	- Added the %@ substitution, and lots of support goo.
	- Added ARG_FEXEC special switch.
	- Added format primitive.
	- Fixed {} quoting bug.
	- Fixed massive security hole in controls().
	- Players can login using their object number.
	- Added LIGHT and OPAQUE flags.
	- Fixed some rather nasty db and memory bugs.
	- Added support for reading vanilla TinyMUD databases.
	- Added support for filtering garbage objects.
	- Fixed a bunch more recycle bugs.
	- More Makefile changes, especially the switch to lorder/tsort.
	  (readline was toast under ranlib-less SysV without lorder/tsort.)
	- `Ported' to Ultrix 4.3 (broken strftime(3) in libc).

### Changes since TeenyMUD 2.0.2beta:
	- Fixed `give' again.
	- Finished `porting' to AIX 3.2.5 using XLC.
	- Changed @group/disolve to @group/delete.
	- Expanded upgrade support for databases that don't have root rooms.
	  `convertdb' will now ask if you wish to create a new root room.
	- Fixed a small bug in `examine'.

### Changes since TeenyMUD 2.0.3beta:
	- Added /move switch to @copy.
	- Added global command switches, and implemented global /quiet.
	  Currently, only @halt uses this.  Many commands will, shortly.
	- Added '@' location-of matching to some of the resolver routines.
	  Taught @teleport how to use this.  (I.e., @tel me=@them)
	- Added /clear switch to @savestty.
	- Added a string array type to the configuration system, and added
	  configuration support for defining illegal player names.
	  (TODO: Make it all dynamic.)
	- Split command routines out of dbutils.c, to dbcmds.c.
	- Rewrote display_flags() to do proper sorting.  (Changes the
	  output of the examine command a bit.)
	- Removed an extra strcpy() from check_news().  Heh.
	- Reorganized some of the low level STTY/@savestty code, and added
	  a /restore switch to @savestty.
	- Added hfile_autoload conf option, defaulting to true.
	- Added /reload switches to help and news, for when hfile_autoload
	  is turned off.
	- Deleted match_regexp(), it was broken and wasn't used.
	- Money movement rewards ("You found a penny!") found inflation.
	- Money routines moved to their own file.
	- Made all configuration strings dynamic.
	- Money names are configurable.
	- Added IMMUTABLE attribute flag, settable by Wizards.  Makes an
	  attribute visible but unchangable by the object's owner.
	- Changed the default flags for Hostname/SITE from INTERNAL to
	  IMMUTABLE.  Added DB_IMMUTATTRS flag to text db system to
	  automatically upgrade older databases.
	- Updated the FSF's mailing address everywhere; GPL'd all of the
	  Makefiles, and sample.conf.
	- Got `pcc' to grok everything again.  Everything compiles and almost
	  works on 4.2BSD.  Ack.  **NEED TO FINISH BEFORE 2.0.4!**
	- Added KillFail/OKillFail/AKillFail attributes; these work as
	  expected, and on HAVENed locations.
	- Added upcase/lowcase/swapcase primitives.
	- Added support for lorder-less machines.
	- Added 'include' directive to conf file loading.
	- Added '@wipe' and '@palias' commands.
	- Changed 'players.c' to 'ptable.c' and reorganized the code.
	- Fixed some code optimizations which broke on some compilers.
	- Fixed failed creation logging.
	- Added ELOQUENT flag for players and things.
	- Fixed potential crash in @edit.
	- @owned no longer requires arguments.
	- Fixed recursive messages in resolv routines.
	- Fixed byte swapping for call to tcp_authuser in tcpip.c.
	- Fixed lots of bugs ELOQUENT found.
	- Fixed a /default bug in @case.
	- Fixed yet another bug in bad_pnames.
	- Changed MALLOC_DEBUG to MEMTRACKING, making it more robust but
	  much, much slower.
	- Changed to include the username in the Hostname attribute.
	- Non-existant text files now produce an error message.
	- Added missing shutdown() calls to tcp_authuser().
	- Fixed Yet Another tcp_authuser() bug.
	- Player names may not be changed if global registration is in effect,
	  except by GOD (who no longer has to supply a password).
	- Added the TRANSPARENT flag and transparent exits.

### Changes since TeenyMUD 2.0.4beta:
	- Fixed parse_boolexp() NULL checking.		[PL#1]
	- Fixed AUDIBLE/OBVIOUS clash.			[PL#1]
	- Fixed isBUILDER macro.			[PL#2]
	- Fixed password buffer lengths.		[PL#2]
	- QUIET puppets are quieter.			[PL#2]
	- Fixed a small bug in resolve_player that was	[PL#2]
	  causing big problems.
	- Fixed the format of WHERE output to handle	[PL#2]
	  longer port and fd numbers.
	- Fixed free_help to not free FMODTIME.		[PL#2]
	- Fixed set_str_elt() for player names.		[PL#3]
	- Fixed some text db conversion bugs.		[PL#3]
	- Fixed division by zero in rand primitive.	[PL#4]
	- Fixed WHO commands to not match twilight zone [PL#4]
	  connections.
	- Fixed bug so that iqueue entries actually	[PL#4]
	  run, allowing things like @foreach to actually
	  work again.
	- Added the `registername' conf option.		[PL#5]
	- Fixed parse_name_pwd() to not return failure	[PL#5]
	  if pwd is NULL.
	- Finished adding support for NULL player	[PL#5]
	  passwords.  Some support was there, but if you used it,
	  it'd probably crash the game, so this was an appropiate
	  thing to fix.
	- Fixed locking code so that *player works	[PL#5]
	  again.  I believe that's the only place RSLV_SPLATOK
	  should be used.
	- Fixed resolve_player() to be more general,	[PL#5]
	  and match things as well.  Also made RSLV_ALLOK
	  default behaviour for Wizards.  This makes resolve_player()
	  the general matching routine for things that might be able
	  to hear things, like it should've already been.
	- Fixed kill to function properly with money disabled.
	- Fixed money prototypes in externs.h
	- Fixed group_create() declaration.
	- Fixed truncation bug in fval().
	- Fixed @foreach, @lock, @unlock, @copy, maybe
	  some others to be less sensetive to whitespace.
	- Added /sort switch to examine.
	- Added enable_autohome configuration option.  Sends players to
	  their homes upon disconnection.
	- Added compact_contents configuration option.  Makes contents
	  lists look more like Obvious Exists lists.
	- Added MATCH, PMATCH, and STRDCAT primitives.
	- Upgraded to Autoconf 2.5.
	- Added legal_thingloc_check(), to check for loops object locations.
	- Added lots of calls to legal_thingloc_check().
	- Changed varargs inclusion, again.
	- Rewrote long money name formats, to work as they were meant to...
	- HTML and Pueblo support...
	  o enable_pueblo, help_url, news_url configuration options.
	  o Vrml_url, Htdesc standard attributes.
	  o /html switches.
	  o html settings in STTY.
	- Fixed small bug in do_examine().
	- Fixed a small bug in do_stty().
	- Added /nohtml switches to help and news commands.
	- Added 'none' to the "no flags" keys in parse_flags().

### Changes since TeenyMUD 2.0.5:
	- Changed all time values in the database to time_t, adding
	  get_time_elt() and set_time_elt().
	- Updated various parts of the code to run on modern systems.
	- Disabled --with-bsddb configuration option.

### Changes since TeenyMUD 2.0.6:
	- The old hash library is no more; uthash is now used in it's place,
	  where appropriate.  Some uses of hashing are simply gone.
	- ty_strncpy and strncat have been replaced with strlcpy and strlcat.
	- Many bugs have been fixed, most dealing with memory management.

### Changes since TeenyMUD 2.0.7:
	- More updates to time_t support.
	- Added time_t size flag to textdump.
	- Updated autoconf to a modern version.  Legacy platforms will no longer work.
	- Fixed minor compiler warnings and bugs.
	- Completely removed bsddb support.

### Changes since TeenyMUD 2.0.8:
	- Fixed non-binary pronouns.
