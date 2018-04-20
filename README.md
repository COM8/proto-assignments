# protocol-assignment-1

### ToDo:
* Replace ```Sender MAC``` with something more unique to allow miltiple connetion from one NIC
* Finish File-Transfer-ACK

### Changelog:
* 20.04.2018 [Fabian] Initial commit

### Client-Hello-Handshake:
```
+------+------------+----------+
| Type | Sender MAC | Checksum |
+------+------------+----------+
```

Type [4 Bit]:<br/>
	0000 => Client hello (initial message from the client to the server)

Sender MAC [48 Bit]<br/>
	To uniquely identify each client on the server

Checksum [x Bit]

### Server-Hello-Handshake:
```
+------+------------+-----------+----------+
| Type | Sender MAC | File-port | Checksum |
+------+------------+-----------+----------+
```

Type [4 Bit]:<br/>
	0001 => Server connection accepted (server accepted the client)

Sender MAC [48 Bit]<br/>
	To uniquely identify each client on the server

File-port [16 Bit]:<br/>
	A free server port for file uploads

Checksum [x Bit]

### File-Creation:
```
+------+------------+-----------------+-----------+------------------+-----------+
| Type | Sender MAC | Sequence Number | File Type | File Name Length | File Name |
+------+------------+-----------------+-----------+------------------+-----------+
+-------------------------+------------------+----------+
| Relat. File Path Length | Relat. File Path | Checksum |
+-------------------------+------------------+----------+
```

Type [4 Bit]:<br/>
	0010 => File creation package

Sender MAC [48 Bit]<br/>
	To uniquely identify each client on the server

Sequence Number [32 bit]:<br/>
	Like TCP

File Type [1 Bit]:<br/>
	0 => Folder<br/>
	1 => File<br/>

File Name Length [x Bit]

File Name Length [defined in "File Name Length" in Bit]

Relat. File Path Length [x Bit]

Relat. File Path [defined in "Relat. File Path Length" in Bit]

Checksum [x Bit]

### File-Transfer:
```
+------+------------+-----------------+-------+----------------+---------+----------+
| Type | Sender MAC | Sequence Number | Flags | Content Length | Content | Checksum |
+------+------------+-----------------+-------+----------------+---------+----------+
```

Type [4 Bit]:<br/>
	0011 => File transfer package

Sender MAC [48 Bit]<br/>
	To uniquely identify each client on the server

Sequence Number [32 bit]:<br/>
	Like TCP

Flags [4 Bit]:
```
0000
||||
|||+-> First package for the given file
||+--> File content
|+--->
+----> Last package for the file
```

Content Length [x Bit]

Content [defined in "Content Length" in Bit]

### File-Transfer-ACK:
```
+------+--------------------
| Type | Sender MAC
+------+--------------------
```
