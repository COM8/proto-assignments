# Protocol Design Assignment
`csync` is a file synchronization client and server written in C/C++ for the Protocol Design lecture during the summer term 2018.

It features:
* A custom file transfer protocol based on UDP [(read more...)](https://github.com/COM8/proto-assignments/wiki/Protocol)
* Transfer package integrity checks (checksums)
* Multi user support
* User authentication support
* Delta Sync [(read more...)](https://github.com/COM8/proto-assignments/wiki/Delta-Sync)
* End-to-End encryption with a multiway Diffie-Hellman handshake [(read more...)](https://github.com/COM8/proto-assignments/wiki/Security-considerations)

## Build
To compile `csync` you need a C++17 capable compiler ([G++ >= 7](https://gcc.gnu.org/projects/cxx-status.html) , [Clang++ >= 6](https://clang.llvm.org/cxx_status.html)).
For Buildsystems you need cmake >= 3

In order to build you need to execute in the repo
 ```
mkdir build
cd build
cmake ..
make
```

## How to use
### Run the server example
./build/csync -s -p 12345 -cc 0<br/>
`-cc <uint>` messages per second (0 = unlimited)<br/>
`-s` run as server<br/>
`-p <uint>` server listen port<br/>

### Run the client example
./build/csync -h 192.168.178.24 -p 12345 -f build -u user0 -pass password0<br/>
`-h <host>` the server address<br/>
`-p <uint>` the server listen port<br/>
`-f <path>` the sync path<br/>
`-u <string>` the username<br/>
`-pass <string>` the password for the username<br/>

The server accepts the following users:
```
users["user0"] = User{"user0", "password0"};
users["user1"] = User{"user1", "password1"};
users["user2"] = User{"user2", "password2"};
users["user3"] = User{"user3", "password3"};
users["user4"] = User{"user4", "password4"};
```

To add more users just edit [`src/UserStore.cpp`](https://github.com/COM8/protocol-assignment-1/blob/master/src/UserStore.cpp) and recompile.

### Limitations
* Watch the [Issues](https://github.com/COM8/proto-assignments/issues) tracker for an up to date list of all broken features.

### Future work
**At the moment** we do not plan to continue developing the `csync` client/server.

### More information
* [Protocol](https://github.com/COM8/proto-assignments/wiki/Protocol)
* [Benchmarks](https://github.com/COM8/proto-assignments/wiki/Benchmarks)
* [Delta Sync](https://github.com/COM8/proto-assignments/wiki/Delta-Sync)
* [Security considerations](https://github.com/COM8/proto-assignments/wiki/Security-considerations)
