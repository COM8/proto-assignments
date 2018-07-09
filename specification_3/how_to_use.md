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