# Description

Example of writing simple module for lua in c.

After that run lua and load module:
$ ./lua
Lua 5.3.4  Copyright (C) 1994-2017 Lua.org, PUC-Rio
> cow = require 'libcowsay'
> cow.say('I moo-a for Lua')