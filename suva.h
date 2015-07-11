// Global application definitions
// $Id: suva.h,v 1.10 2002/08/08 23:05:10 darryl Exp $
#ifndef _SUVA_H
#define _SUVA_H

//! Maximum hostname length
#define MAX_HOST		1024

//! Maximum organization name
#define MAX_ORG			256

//! Maximum user name
#define MAX_USER		128

//! Maimum Suvlet name length
#define MAX_SUVLETN		1024

//! General purpose buffer length
#define MAX_BUFFER		1024

//! STL error text length
#define MAX_ERRSTR		256

//! Random challenge key length (AENC/ADEC)
#define MAX_RANDKEY		128

//! RSA key length
#define MAX_RSAKEY		1025

//! Maximum RSA key filename length
#define MAX_KEYN		32

//! Maximum system ID length
#define MAX_SYSNAME		32

//! Error
#define STL_ERRO		0x00

//! Authentication initiation
#define STL_AINI		0x01

//! "No-care" authentication
#define STL_NINI		0x02

//! Public key request
#define STL_PKEY		0x03

//! Authentication encrypt string
#define STL_AENC		0x04

//! Authentication decrypted string
#define STL_ADEC		0x05

//! New session key
#define STL_NKEY		0x06

//! SCL command
#define STL_SCLC		0x07

//! Used for tunnel data
#define STL_DATA		0x08

//! Open tunnel request
#define STL_OTUN		0x09

//! Version information packet
#define STL_VINI		0x0A

//! Number of STL op-codes
#define MAX_STLOP		STL_VINI

//! A normal SCL result op-code
#define SCL_RETURN		0x00

//! Open connection to remote server
#define SCL_OCONN		0x01

//! Close connection to remote server
#define SCL_CCONN		0x02

//! Create suvlet
#define SCL_CSUVLET		0x03

//! Destroy suvlet
#define SCL_DSUVLET		0x04

//! Function call
#define SCL_FCALL		0x05

//! Function return
#define SCL_FRETURN		0x06

//! Function call error
#define SCL_FERROR		0x07

//! Number of SCL op-codes
#define MAX_SCLOP		SCL_FERROR

//! Last operation successful
#define SRC_OKAY		0x01

//! Connecting
#define SRC_CONNECT		0x02

//! Connection timed-out
#define SRC_CONN_TO		0x03

//! Connection failed (ie: connection refused)
#define SRC_CONN_FAIL	0x04

//! No connection established
#define SRC_CONN_NONE	0x05

//! Connection already established
#define SRC_CONN_INUSE	0x06

//! Connected
#define SRC_CONNECTED	0x07

//! Suvlet does (or does not) exist
#define SRC_SEXISTS		0x08

//! Failed to spawn suvlet
#define SRC_SPAWN		0x09

//! Spawned suvlet
#define SRC_SPAWNED		0x0A

//! Remote suvlet died
#define SRC_SDIED		0x0B

//! Suvlet terminated (SCL_DSUVLET)
#define SRC_STERM		0x0C

//! Invalid payload size
#define SRC_BADSIZE		0x0D

//! Fatal system call failure
#define SRC_SYSCALL		0x0E

//! Authentication timed out reached
#define SRC_AUTH_FAIL	0x0F

//! Invalid organization name
#define SRC_INVORG		0x10

//! Invalid op-code
#define SRC_INVOPC		0x11

//! No authorization, access denied
#define SRC_NOAUTH		0x12

//! New session key acknowledgement
#define SRC_NKEY		0x13

//! Session key error
#define SRC_NKEY_FAIL	0x14

//! Open tunnel connection successful
#define SRC_OTUN		0x15

//! Unable to open tunnel
#define SRC_OTUN_FAIL	0x16

//! Authentication initiation acknowledgement
#define SRC_AINI		0x17

//! "No-care" authentication acknowledgement
#define SRC_NINI		0x18

//! Unable to retrieve public key
#define SRC_PKEY_FAIL	0x19

//! Payload size too big
#define SRC_TOOBIG		0x20

//! Allocated STL/SCL packet
#define MEM_PACKET		1

//! Allocated STL/SCL payload
#define MEM_PAYLOAD		2

//! General-purpose buffer memory
#define MEM_BUFFER		3

//! Suva database
#define MEM_SDB			4

//! Key list
#define MEM_KEY			5

//! Organization list
#define MEM_ORG			6

//! Child list
#define MEM_CHILD		7

//! Suvlet list
#define MEM_SUVLET		8

//! IP list
#define MEM_IPLIST		9

//! Tunnel list
#define MEM_TUNNEL		10

//! File descriptor list
#define MEM_FDLIST		11

//! Socket list
#define MEM_SOCKET		12

//! Host key database
#define MEM_HOSTKEY		13

//! Public RSA key cache
#define MEM_KEYCACHE	14

//! Active key polling sessions
#define MEM_KEYPOLL		15

//! Public key (polled) list
#define MEM_PKEY		16

//! Server list memory
#define MEM_SERVER		17

//! Signal handler callback typedef
typedef void (*sighandler_t)(int);

#endif // _SUVA_H
