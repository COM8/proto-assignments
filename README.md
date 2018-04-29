# protocol-assignment-1

### ToDo:
* Add length for last Sequence and FID Length

### Important:
* UDP MTU
* Don't create packages with a size of (bit-size mod 8) != 0. It makes it hard on the receiver side to interpret those!

### Changelog:
* 29.04.2018 [Kilian] Added File-Status Message
* 29.04.2018 [Fabian] Added message description and ```Ping``` message
* 26.04.2018 [Kilian] Added Convention, added CRC and Hash,added ability to delete Files and Folders, minor optimisations
* 20.04.2018 [Fabian] Initial commit
* 22.04.2018 [Fabian] Protocol

## Convention:
* FID := Relative Path with File name (folder/file.txt)

## Protocol:

### General field descriptions:
Type [4 Bit]:<br/>
	0000 => Client-Hello-Handshake<br/>
	0001 => Server-Hello-Handshake<br/>
	0010 => File-Creation<br/>
	0011 => File-Transfer<br/>
	0100 => ACK<br/>
	0101 => Ping<br/>
	1000 => File-Status<br/>

Client ID [32 Bit]:<br/>
	An unique client id generated by the server on first contact<br/>
	e.g. static filed with an int that gets incremented for each connected client

Checksum [32 Bit]:<br/>
	CRC32 Algorithm [Wiki Link](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

Sequence Number [32 bit]:<br/>
	Like TCP

### Client-Hello-Handshake:
The initial connection message that gets send by the client.
```
0      4      20         52       56
+------+------+----------+--------+
| Type | Port | Checksum | UNUSED |
+------+------+----------+--------+
```

Port [16 Bit]:<br/>
	The port on which the client listens to server messages

UNUSED [4 Bit]:<br/>
	To ensure the package has mod 8 = 0 size

### Server-Hello-Handshake:
Once the server received a ```Client-Hello-Handshake``` message he should reply with this message.
```
0      4       8           40            56         88
+------+-------+-----------+-------------+----------+
| Type | Flags | Client ID | Upload-port | Checksum |
+------+-------+-----------+-------------+----------+
```
Upload-port [16 Bit]:<br/>
	The Port where the client should send all following messages to

Flags [4 Bit]:
```
00000
||||
|||+-> Client accepted
||+--> Too many clients - connection revoked
|+---> *UNUSED*
+----> *UNUSED*
```

### File-Creation:
Marks the start of a file transfer. Tells the server to create the given file with the given path.
```
0      4           36                68          70					
+------+-----------+-----------------+-----------+------------+-----+----------+----------+
| Type | Client ID | Sequence Number | File Type | FID Length | FID | SHA3 256 | Checksum |
+------+-----------+-----------------+-----------+------------+-----+----------+----------+

```

File Type [2 Bit]:<br/>
	00 => Folder<br/>
	01 => File<br/>
	10 => Delete File<br/>
	11 => Delete Folder<br/>

FID Length [x Bit] **Don't forget about the MTU**

FID [defined in "File Name Length" in Bit]:
	With relative Path

SHA3 256:
	Is the SHA3 256 Hash of the File itself to check it after the transfer. [Wiki Link](https://en.wikipedia.org/wiki/SHA-3)

If a FID is unknown by the Server he will create the file


### File-Transfer:
The actual file transfer message containing the file content.
```
0      4           36                68      72
+------+-----------+-----------------+-------+----------------+---------+----------+
| Type | Client ID | Sequence Number | Flags | Content Length | Content | Checksum |
+------+-----------+-----------------+-------+----------------+---------+----------+
```

Flags [4 Bit]:
```
0000
||||
|||+-> First package for the given file
||+--> File content
|+---> *UNUSED*
+----> Last package for the file
```

Content Length [x Bit] **Don't forget about the MTU**

Content [defined in "Content Length" in Bit] **Don't forget about the MTU**

### ACK:
For acknowledging ```Ping```, ```File-Creation``` and ```File-Transfer``` messages.
```
0      4           36                    68
+------+-----------+---------------------+
| Type | Client ID | ACK Sequence Number |
+------+-----------+---------------------+
```

ACK Sequence Number [like Sequence Number]:<br/>
	The acknowledged ```Sequence Number``` or ```Ping Sequence Number```

### Transfer-Ended:
Gets send by the client once he wants to end the transfer.
```
0      4       8           40         72
+------+-------+-----------+----------+
| Type | Flags | Client ID | Checksum |
+------+-------+-----------+----------+
```

Flags [4 Bit]:
```
0000
||||
|||+-> Transfer finished
||+--> Cancelled by user
|+---> Error
+----> *UNUSED*
```

### Ping:
This message is used for ensuring the opponent is still there. The opponent should acknowledge each received ```Ping``` message with an ```Server-ACK```.Should get send by each side if there was no message exchange for more than 5 seconds.<br/>
It also can be used for package loss and throughput tests with a modified ```Payload Length```.
```
0      4                      36               64
+------+----------------------+----------------+---------+
| Type | Ping Sequence Number | Payload Length | Payload |
+------+----------------------+----------------+---------+
```

Ping Sequence Number [32 Bit]<br/>
	An unique number for identifying each ping

Payload Length [28 Bit]:<br/>
	Describes how long the the following payload is in byte

Payload [X Byte]:<br/>
	Defined via the ```Payload Length```

### File-Status:
This Message Sends an FID and gets as response the sequence number of the last successful package.
```
0      4        5                  
+------+--------+------------+-----+---------------+
| Type |  Flags | FID-Length | FID | Last Sequence |
+------+--------+------------+-----+---------------+
```

Flags [1 Bit]:
```
0 := Request Status of FID
1 := Response of request
```

FID Length [x Bit] **Don't forget about the MTU**

FID [defined in "File Name Length" in Bit]:
	With relative Path

Last Sequence [S Bit]:
	is the last successful sequence of the Package (don't care in Request)


## Process example:

```
      Client				  Server
	|	Client-Hello-Handshake	    |
	| --------------------------------> | The clients starts the connection on the default port
	|				    | and tells the server the port on which he listens for answers
	|	Server-Hello-Handshake      |
	| <-------------------------------- | The server responds with a client ID and a port where the
	|				    | server is listening for incoming transfer messages
	|	File-Creation		    |
	| --------------------------------> | The client sends this message to inform the server about
	|				    | the new file that will be transferred
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> | The client starts sending the file
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	Server-ACK  		    |
	| <-------------------------------- | The server sends an ACK message for each message
	|				    | it received from the client
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	Transfer-Ended		    |
	| --------------------------------> | The client tells the server that the transfer finished
```
