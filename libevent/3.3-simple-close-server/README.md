# Description

The same application as in 1.3 but listening socket is created via
**libevent** functions (it simplifies a little bit creation of socket and setting needed properties to it). System functions are not used. Everything is done via libevent.

Server closes all connections immediately.