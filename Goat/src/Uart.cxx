// Goat - Scintilla based Text Editor
/** @file Uart.cxx
 ** Interface to UART facilities.
 **/
// Copyright 2011-2015 by Nunzio Raciti <nunzio.raciti@gmail.com>
// The License.txt file describes the conditions under which this software may be distributed.

#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#else
#include <gtk/gtk.h>
#include <glib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pwd.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "Goat.h"
#include "crc16.h"
#include "Mutex.h"
#include "Uart.h"
#include "Term.h"


static void *XmodemTransmitThread(void *ptr);
static void *RawReplTransmitThread(void *ptr);

/*
 * Target: SciTEGTK::ExecuteOnConsole -> SciTEGTK::PostCallbackConsole -> SciTEBase::ConsoleInsertChars()
 *
 * Host:   SciTEBase::CharAddedConsole() ->  UART::SendChars()
 */

/* CRC16 code.....*/
/*
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* CRC16 implementation acording to CCITT standards */

static const unsigned short crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

unsigned short UART::crc16_ccitt(const unsigned char *buf, int len) {
	register int counter;
	register unsigned short crc = 0;
	for (counter = 0; counter < len; counter++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(unsigned char *)buf++)&0x00FF];
	return crc;
}


int UART::check(int crc, const unsigned char *buf, int sz) {
	if (crc) {
		unsigned short crc = crc16_ccitt(buf, sz);
		unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
		if (crc == tcrc)
			return 1;
	} else {
		int i;
		unsigned char cks = 0;
		for (i = 0; i < sz; ++i) {
			cks += buf[i];
		}
		if (cks == buf[sz])
			return 1;
	}

	return 0;
}

int UART::SerialTrasmitFile(const char *name, int type) {
	FILE *pFile;
	size_t result;

	pFile = fopen(name , "rb");
	if (pFile==NULL) {
		printf("File error\n");
		return (-1);
	}

	// obtain file size:
	fseek(pFile , 0 , SEEK_END);
	sendFileBufSize = ftell(pFile);
	rewind(pFile);

	// allocate memory to contain the whole file:
	sendFileBuf = (unsigned char *) malloc(sizeof(char)*sendFileBufSize);
	if (sendFileBuf == NULL) {
		printf("Memory error\n");
		return (-2);
	}

	// copy the file into the buffer:
	result = fread(sendFileBuf,1,sendFileBufSize,pFile);
	if (result != sendFileBufSize) {
		printf("Reading error\n");
		free(sendFileBuf);
		sendFileBuf = NULL;
		return (-3);
	}

	/* the whole file is now loaded in the memory buffer. */
	fclose(pFile);

	/* Following really needs to be rewritten in a better way... */
	/* Unfortunately Windows doesn't help! */
#ifdef GTK
	{
		pthread_t tid = 0;
		switch (type) {
		case UART_SEND_XMODEM:
			pthread_create(&tid, NULL, XmodemTransmitThread, (void *)(this));
			break;
		case UART_SEND_RAW_REPL:
			pthread_create(&tid, NULL, RawReplTransmitThread, (void *)(this));
			break;
		}
		return tid != 0;
	}
#endif

#ifdef WIN32
	{
		uintptr_t result;

		switch (type) {
		case UART_SEND_XMODEM:
			result = _beginthread((void ( *)(void *))XmodemTransmitThread, 1024 * 1024, reinterpret_cast<void *>(this));
			return result != static_cast<uintptr_t>(-1);
			break;
		case UART_SEND_RAW_REPL:
			result = _beginthread((void ( *)(void *))RawReplTransmitThread, 1024 * 1024, reinterpret_cast<void *>(this));
			return result != static_cast<uintptr_t>(-1);
			break;
		}
	}
#endif
}

static void *XmodemTransmitThread(void *ptr) {
	UART *uart_class = static_cast<UART *>(ptr);
	uart_class->XmodemTransmit();
	return NULL;
}

static void *RawReplTransmitThread(void *ptr) {
	UART *uart_class = static_cast<UART *>(ptr);
	uart_class->RawReplTransmit();
	return NULL;
}

#define STATE_INITIAL		0
#define STATE_RX_TIMEOUT	1
#define STATE_RX_HEAD_0		2
#define STATE_RX_HEAD_1		3
#define STATE_RX_FIRST_EOF	4
#define STATE_RX_SECOND_EOF 5
#define STR_TO_EXIT 		"raw REPL; CTRL-B to exit\r\n>"
#define RX_TIMEOUT			1000

int UART::RawReplTransmit() {
	unsigned char 	xBuff[1024];
	size_t 			numchar;
	int 			state, byte, ret;
	unsigned char 	ch;
	int 			timeout;

	ClearRingBuffer2();
	SetSendFileStatus(UART_SEND_RAW_REPL);
	// Ctrl-C twice: interrupt any running program
	OutByte('\r');
	OutByte(3);
	OutByte(3);
	// ctrl-A: enter raw REPL
	OutByte('\r');
	OutByte(1);
	// data = self.read_until(1, b'to exit\r\n>')
	msSleep(100);
	memset(xBuff,0,sizeof(xBuff));
	for (numchar = 0; numchar < sizeof(xBuff); numchar++) {
		byte = InByte(100);
		if (byte < 0) break;
		xBuff[numchar] = byte;
	}

	if (numchar < sizeof(STR_TO_EXIT)) {
		goto enter_raw_repl2;
	}

	if (strstr((char *)xBuff, STR_TO_EXIT) != 0) {
		goto enter_raw_repl_exit_fine;
	}

enter_raw_repl2:
	// self.serial.write(b'\x04') # ctrl-D: soft reset
	OutByte(4); // ctrl-D: soft reset
	msSleep(100);
	memset(xBuff,0,sizeof(xBuff));
	for (numchar = 0; numchar < sizeof(xBuff); numchar++) {
		byte = InByte(100);
		if (byte < 0) break;
		xBuff[numchar] = byte;
	}

	if (numchar < sizeof(STR_TO_EXIT)) goto enter_raw_repl2;

	if (strstr((char *)xBuff, STR_TO_EXIT) != 0) {
		goto enter_raw_repl_exit_fine;
	}
	return -2;

enter_raw_repl_exit_fine:

	for (numchar = 0; numchar < sendFileBufSize; numchar++)  {
		OutByte(sendFileBuf[numchar]);
	}

	msSleep(100);
	// Ctrl-D: input finished
	OutByte(4);
	msSleep(100);

	memset(xBuff,0,sizeof(xBuff));
	state = STATE_INITIAL;

	/* follow */
	while (state != STATE_RX_SECOND_EOF) {
		timeout = RX_TIMEOUT;
		while (RingBufferIsEmpty()) {
			if (timeout) {
				msSleep(1);
				if (--timeout == 0) {
					if (state == STATE_RX_FIRST_EOF || state == STATE_RX_SECOND_EOF) {
						ret = 1;
						goto RawReplTransmit_end;
					}
					if (state == STATE_INITIAL || state == STATE_RX_HEAD_0 || state == STATE_RX_HEAD_1) {
						ret = 0;
						goto RawReplTransmit_end;
					}
				}
			}
		}
		byte = GetCharFromBuffer();
		if (byte < 1) {
			continue;
		}

		ch = (0x00ff & byte);

		if (state == STATE_INITIAL && ch == 'O') {
			state = STATE_RX_HEAD_0;
			continue;
		}

		if (state == STATE_RX_HEAD_0 && ch == 'K') {
			state = STATE_RX_HEAD_1;
			continue;
		}

		if (state == STATE_RX_HEAD_1 && ch == 0x04 /* EOF */) {
			state = STATE_RX_FIRST_EOF;
			continue;
		}

		if (state == STATE_RX_FIRST_EOF && ch == 0x04 /* EOF_error_string_EOF **/) {
			state = STATE_RX_SECOND_EOF;
			continue;
		}

		if (state >= STATE_RX_HEAD_1) {
			/* Unfortunately this thread can't write on GUI window  directly */
			PutCharToBuffer2(ch);
		}
	}

RawReplTransmit_end:

	msSleep(100);
	// Ctrl-B: enter friendly REPL
	OutByte('\r');
	OutByte(2);

	// remove two lines
	for (char numLF = 0; numLF <= 2;) {
		byte = InByte(100);
		if (byte < 0) break;
		if (char(0x00ff & byte) == UART_EOL_LF) numLF++;
	}

	SetSendFileStatus(UART_SEND_NO_FILE);
	ClearRingBuffer2();
	return ret;
}

int UART::XmodemTransmit() {
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned int timeout = MS_TIMEOUT;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry, ret;

	/* Waiting for character 'C' */
	while ((c = PeekCharFromBuffer()) != 'C' && timeout) {
		msSleep(1);
		timeout--;
	}

	SetSendFileStatus(UART_SEND_XMODEM);

	for (;;) {
		for (retry = 0; retry < 16; ++retry) {
			if ((c = InByte((DLY_1S)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = InByte(DLY_1S)) == CAN) {
						OutByte(ACK);
						ret = -1; /* canceled by remote */
						goto exit_trans;
					}
					break;
				default:
					break;
				}
			}
		}
		OutByte(CAN);
		OutByte(CAN);
		OutByte(CAN);
		ret = -2; /* no sync */
		goto exit_trans;

		for (;;) {
start_trans:
			xbuff[0] = SOH;
			bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = sendFileBufSize - len;
			if (c > bufsz) c = bufsz;
			if (c >= 0) {
				memset(&xbuff[3], CTRLZ, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				} else {
					memcpy(&xbuff[3], &sendFileBuf[len], c);
					//if (c < bufsz) xbuff[3+c] = CTRLZ; // now useless, see the memset() before.
				}
				if (crc) {
					unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
					xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
					xbuff[bufsz+4] = ccrc & 0xFF;
				} else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz+3] = ccks;
				}
				for (retry = 0; retry < MAXRETRANS; ++retry) {
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						OutByte(xbuff[i]);
					}
					if ((c = InByte(DLY_1S)) >= 0) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = InByte(DLY_1S)) == CAN) {
								OutByte(ACK);
								ret = -1; /* canceled by remote */
								goto exit_trans;
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				OutByte(CAN);
				OutByte(CAN);
				OutByte(CAN);
				ret = -4; /* xmit error */
				goto exit_trans;
			} else {
				for (retry = 0; retry < 10; ++retry) {
					OutByte(EOT);
					if ((c = InByte((DLY_1S)<<1)) == ACK) break;
				}
				ret = (c == ACK)?len:-5;
				goto exit_trans;
			}
		}
	}

exit_trans:

	SetSendFileStatus(UART_SEND_NO_FILE);
	free(sendFileBuf);
	sendFileBuf= NULL;
	return ret;
}

#ifdef WIN32
int UART::ListenPort() {
	static char Buffer[BUFFER_RX + 1];
	DWORD bytes = 0;

	if (!IsStarted())
		return 0;

	if (!Status()) {
		return 0;
	}

	if (portHandle == INVALID_HANDLE_VALUE)
		return 0;

	if (!ReadFile(portHandle, Buffer, BUFFER_RX, &bytes, NULL)) {
		return 0;
	}

	if (bytes > 0) {
		for (DWORD cc=0; cc < bytes; cc++) {
			PutCharToBuffer((char) Buffer[cc]);
		}
	}

	return bytes;
}
#else
int UART::ListenPort() {
	static char Buffer[BUFFER_RX + 1];
	struct timeval tv;
	fd_set fds;
	int bytes;
	int n;
	int tmout = 1000;

	if (!IsStarted())
		return 0;

	if (!Status()) {
		msSleep(1000);
		Start();
		return 0;
	}

	tv.tv_sec = tmout / 1000;
	tv.tv_usec = (tmout % 1000) * 1000L;
	FD_ZERO(&fds);
	FD_SET(portFd, &fds);

	if (select(portFd+1, &fds, NULL, NULL, &tv) > 0)
		n = 1 * (FD_ISSET(portFd, &fds) > 0) + 2 * (FD_ISSET(0, &fds) > 0);

	bytes = 0;
	if ((n & 1) == 1) {
		bytes = read((int) portFd , Buffer, BUFFER_RX);
	}

	if (bytes > 0) {
		for (int cc=0; cc < bytes; cc++) {
			PutCharToBuffer((char) Buffer[cc]);
		}
	} else {
		bytes = 0;
	}

	return bytes;
}
#endif

bool UART::RingBufferIsEmpty() {
	Lock lock(mutex);
	return (indexRead == indexWrite);
}

void inline UART::PutCharToBuffer(char byte) {
	Lock lock(mutex);

	buffer[indexWrite++] = byte;
	if (indexWrite == BUFFER_RX)
		indexWrite = 0;
}


int UART::GetCharFromBuffer() {
	char byte;
	Lock lock(mutex);

	if (indexWrite == indexRead)
		return (0xff00); /* Buffer empty */

	byte = buffer[indexRead++];

	if (indexRead == BUFFER_RX)
		indexRead = 0;

	return (int)byte;
}

void UART::ClearRingBuffer2() {
	indexWrite2= 0;
	indexRead2 = 0;
}

bool UART::RingBuffer2IsEmpty() {
	Lock lock(mutex2);
	return (indexRead2 == indexWrite2);
}

void inline UART::PutCharToBuffer2(char byte) {
	Lock lock(mutex2);

	buffer2[indexWrite2++] = byte;
	if (indexWrite2 == BUFFER2_RX)
		indexWrite2 = 0;
}


int UART::GetCharFromBuffer2() {
	char byte;
	Lock lock(mutex2);

	if (indexWrite2 == indexRead2)
		return (0xff00); /* Buffer2 empty */

	byte = buffer2[indexRead2++];

	if (indexRead2 == BUFFER2_RX)
		indexRead2 = 0;

	return (int)byte;
}

int UART::PeekCharFromBuffer() {
	char byte;
	Lock lock(mutex);

	if (indexWrite == indexRead)
		return (0xff00); /* Buffer empty */

	byte = buffer[indexRead];

	return byte;
}

int UART::GetCopyOfBuffer(char *s, int len) {
	int value;
	int bytes;

	for (bytes=0; bytes < len; bytes++) {
		value = GetCharFromBuffer();
		if (value > 0xFF) {
			break;
		}
		s[bytes] = (char)(0xFF & value);
	}

	return bytes;
}

int UART::InByte(unsigned short timeout /* ms */) {
	int byte;
	while (RingBufferIsEmpty()) {
		if (timeout) {
			msSleep(1);
			if (--timeout == 0)
				return -2;
		}
	}
	byte = GetCharFromBuffer();
	return (0x00ff & byte);
}

void  UART::OutByte(int byte) {
	Tx((const char *) &byte, 1);
}

UART *UART::instance = NULL;

UART::UART() {
	pConWin = 0;
	mutex = Mutex::Create();
	mutex2 = Mutex::Create();
#ifdef WIN32
	portHandle = INVALID_HANDLE_VALUE;
#else
	portFd = -1;
#endif
	memset(lockfile,0,sizeof(lockfile));

	crReceived = 0;
	portOpened = 0;
	portFlow = 0;
	portParity=0;
	portSpeed = 115200;
	serialEOL = UART_EOL_LF;
	numBits = 8;
	numStopBits = 1;
	sendFileStatus = UART_SEND_NO_FILE;
	indexWrite= 0;
	indexRead = 0;
	indexWrite2= 0;
	indexRead2 = 0;
	sprintf(message, NO_OPEN_PORT);
	sendFileBufSize = 0;

	sendFileBuf = NULL;
	isAlive = true;
	isStarted = false;
	instance = this;
}

UART::~UART() {
	sprintf(message, NO_OPEN_PORT);
	ClosePortAndRemoveLockFile();
	instance = NULL;
	isAlive = false;
	isStarted = false;
	sendFileStatus = UART_SEND_NO_FILE;
	delete mutex;
	delete mutex2;
	mutex = 0;
	mutex2 = 0;
	if (sendFileBuf != NULL) {
		free(sendFileBuf);
		sendFileBuf= NULL;
	}
}
void UART::SetTargetConsoleWin(GUI::ScintillaWindow *w) {
	pConWin = w;
}

void UART::SetSpeed(unsigned int s) {
	portSpeed = s;
}
void UART::SetNumBits(unsigned char n) {
	numBits = n;
}
void UART::SetNumStop(unsigned char n) {
	numStopBits = n;
}
void UART::SetParity(unsigned char v) {
	portParity = v;
}
void UART::SetPort(const char *s) {
	strncpy(portName, s, MAX_PORT_STR_SIZE-1);
}
void UART::SetEOL(const char eol) {
	serialEOL = eol;
}
char UART::GetEOL(void) {
	return serialEOL;
}

int UART::Tx(const char *buf, int lenght) {
#ifdef WIN32
	DWORD bytesWritten;
	WriteFile(portHandle, buf, lenght, &bytesWritten, NULL);
	return bytesWritten;
#else
	return (write(portFd, buf, lenght));
#endif
}

bool UART::SendCrlf(void) {
	const char crlf_buf[2] = {'\r', '\n'};

	if (!portOpened)
		return false;

	return (Tx(crlf_buf, 2) != 2);
}

int UART::Send(const char *string, int length) {

	if (!portOpened)
		return -1;

	if (length == 0)
		return 0;

	if (length == -1)
		length = strlen(string);

	return Tx(string, length);
}

bool UART::OpenPort(const char *portName) {
	portOpened = false;

#ifdef WIN32
	char portWinName[MAX_PORT_STR_SIZE+1];
	wchar_t wPortWinName[MAX_PORT_STR_SIZE+1];

	sprintf(portWinName,"\\\\.\\%s",portName);
	mbstowcs(wPortWinName,(const char *)portWinName, MAX_PORT_STR_SIZE);
	// Open the serial port.
	portHandle = CreateFile(wPortWinName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (portHandle != INVALID_HANDLE_VALUE)
		portOpened = true;
#else
	portFd = open(portName, O_RDWR | O_NOCTTY);

	if (portFd > 0)
		portOpened = true;
#endif
	return portOpened;
}

char *UART::GetMessage(void) {
	static char message_copy[MAX_MESSAGE_SIZE+1]="";

	if (strncmp(message,message_copy,MAX_MESSAGE_SIZE) == 0) {
		return 0;
	} else {
		strncpy(message_copy,message,MAX_MESSAGE_SIZE);
		return message;
	}
}

void UART::Stop(void) {
	isStarted = false;
	ClosePortAndRemoveLockFile();
	sprintf(message,NO_OPEN_PORT);
}

bool UART::Status(void) {

#ifdef WIN32
#if 0
	DCB myDCB;

	FillMemory(&myDCB , sizeof(myDCB), 0);
	myDCB.DCBlength = sizeof(myDCB);

	if (!GetCommState(portHandle,&myDCB))
		return false;
	else
		return true;
#endif

	return true;

#else
	/* It is so easy, using a serious O.S. */
	struct termios t;
	return !tcgetattr(portFd, &t);
#endif
}

bool UART::Start(void) {
#ifdef WIN32
	int 			StopBits;
	DWORD  			DCB_Baud_Rate;
	DCB 			PortDCB;
	COMMTIMEOUTS 	CommTimeouts;
#else
	struct termios 	termios_p;
#endif

	isStarted = false;

	ClosePort();
	RemoveLockFile();

	if (!OpenPort(portName)) {
		sprintf(message,"Cann't open %s : (errno=%d)", portName, errno);
		return false;
	}

	if (CreateLockFile(portName) == -1) {
		ClosePort();
		sprintf(message,NO_OPEN_PORT);
		return false;
	}

	sprintf(message,"%s : %d,%d", portName, portSpeed, numBits);

#ifndef WIN32
	tcgetattr(portFd, &termios_save);  /* save current serial port settings */
	bzero(&termios_p, sizeof(termios_p)); /* clear struct for new port settings */

	switch (portSpeed) {
	case 300:
		termios_p.c_cflag = B300;
		break;
	case 600:
		termios_p.c_cflag = B600;
		break;
	case 1200:
		termios_p.c_cflag = B1200;
		break;
	case 2400:
		termios_p.c_cflag = B2400;
		break;
	case 4800:
		termios_p.c_cflag = B4800;
		break;
	case 9600:
		termios_p.c_cflag = B9600;
		break;
	case 19200:
		termios_p.c_cflag = B19200;
		break;
	case 38400:
		termios_p.c_cflag = B38400;
		break;
	case 57600:
		termios_p.c_cflag = B57600;
		break;
	case 115200:
		termios_p.c_cflag = B115200;
		break;
	}
	switch (numBits) {
	case 5:
		termios_p.c_cflag |= CS5;
		break;
	case 6:
		termios_p.c_cflag |= CS6;
		break;
	case 7:
		termios_p.c_cflag |= CS7;
		break;
	case 8:
		termios_p.c_cflag |= CS8;
		break;
	}

	switch (portParity) {
	case 1:
		termios_p.c_cflag |= PARODD | PARENB;
		sprintf(message,"%s,O", message);
		break;
	case 2:
		termios_p.c_cflag |= PARENB;
		sprintf(message,"%s,E", message);
		break;
	default:
		sprintf(message,"%s,N", message);
		break;
	}

	if (numStopBits == 2)
		termios_p.c_cflag |= CSTOPB;

	termios_p.c_cflag |= CREAD;
	sprintf(message,"%s,%d", message, numStopBits);

	termios_p.c_iflag = IGNPAR | IGNBRK;

	switch (portFlow) {
	case 1:
		termios_p.c_iflag |= IXON | IXOFF;
		sprintf(message,"%s - Xon/Xoff", message);
		break;
	case 2:
		termios_p.c_cflag |= CRTSCTS;
		sprintf(message,"%s - RTS/CTS", message);
		break;
	default:
		termios_p.c_cflag |= CLOCAL;
		break;
	}

	termios_p.c_oflag = 0;
	termios_p.c_lflag = 0;
	termios_p.c_cc[VTIME] = 0;
	termios_p.c_cc[VMIN] = 1;
	tcsetattr(portFd, TCSANOW, &termios_p);
	tcflush(portFd, TCOFLUSH);
	tcflush(portFd, TCIFLUSH);

	isStarted = true;

	return true;
#else

	if (portHandle == INVALID_HANDLE_VALUE)
		return false;

	switch (portParity) {
	case 1:
		sprintf(message,"%s,O", message);
		break;
	case 2:
		sprintf(message,"%s,E", message);
		break;
	default:
		sprintf(message,"%s,N", message);
		break;
	}

	switch (numStopBits) {
	case 0:
		StopBits = ONESTOPBIT;
		sprintf(message,"%s,1", message);
		break;
	case 1:
		/* Later maybe .... */
		//StopBits = ONE5STOPBITS;
		//sprintf(message,"%s,1.5", message);
		StopBits = ONESTOPBIT;
		sprintf(message,"%s,1", message);
		break;
	case 2:
		StopBits = TWOSTOPBITS;
		sprintf(message,"%s,2", message);
		break;
	}

	switch (portSpeed) {
	case 110:
		DCB_Baud_Rate = 110;
		break;
	case 300:
		DCB_Baud_Rate = CBR_300;
		break;
	case 600:
		DCB_Baud_Rate = CBR_600;
		break;
	case 1200:
		DCB_Baud_Rate = CBR_1200;
		break;
	case 2400:
		DCB_Baud_Rate = CBR_2400;
		break;
	case 4800:
		DCB_Baud_Rate = CBR_4800;
		break;
	case 9600:
		DCB_Baud_Rate = CBR_9600;
		break;
	case 19200:
		DCB_Baud_Rate = CBR_19200;
		break;
	case 38400:
		DCB_Baud_Rate = CBR_38400;
		break;
	case 57600:
		DCB_Baud_Rate = CBR_57600;
		break;
	case 115200:
		DCB_Baud_Rate = CBR_115200;
		break;
	default:
		return INVALID_HANDLE_VALUE;
	}

	FillMemory(&PortDCB, sizeof(PortDCB), 0);
	PortDCB.DCBlength = sizeof(PortDCB);

	//Get the default port setting information.
	if (!GetCommState(portHandle, &PortDCB)) {
		ClosePort();
		return false;
	}

	// Change the settings.
	PortDCB.BaudRate 	= DCB_Baud_Rate;   // BAUD Rate
	PortDCB.ByteSize 	= numBits;           // Number of bits/byte, 5-8
	PortDCB.Parity 		= portParity;          // 0-4=no,odd,even,mark,space
	PortDCB.StopBits	= StopBits;        // StopBits
	PortDCB.fAbortOnError = 0;

	// Re-configure the port with the new DCB structure.
	if (!SetCommState(portHandle, &PortDCB)) {
		ClosePort();
		return false;
	}

	// Retrieve the time-out parameters for all read and write operations
	// on the port.
	if (!GetCommTimeouts(portHandle, &CommTimeouts)) {
		ClosePort();
		return false;
	}
	memset(&CommTimeouts, 0x00, sizeof(CommTimeouts));
	CommTimeouts.ReadIntervalTimeout = MAXDWORD;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0; /* wait 100ms before escape from Readfile() */
	CommTimeouts.WriteTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(portHandle, &CommTimeouts)) {
		ClosePort();
		return false;
	}

	if (!PurgeComm(portHandle, PURGE_TXCLEAR | PURGE_RXCLEAR)) {
		ClosePort();
		return false;
	}

	isStarted = true;
	return true;
#endif
}

void UART::ClosePort(void) {

	if (!portOpened)
		return;

#ifdef WIN32
	if (portHandle != INVALID_HANDLE_VALUE) CloseHandle(portHandle);
	portHandle = INVALID_HANDLE_VALUE;
#else
	tcsetattr(portFd, TCSANOW, &termios_save);
	tcflush(portFd, TCOFLUSH);
	tcflush(portFd, TCIFLUSH);
	if (portFd > 0)
		close(portFd);
	portFd = -1;
#endif
	portOpened = false;
}
/*
 * Find out name to use for lockfile when locking tty.
 */
char *UART::mbasename(char *s, char *res, int reslen) {
	char *p;

	if (strncmp(s, "/dev/", 5) == 0) {
		/* In /dev */
		strncpy(res, s + 5, reslen - 1);
		res[reslen-1] = 0;
		for (p = res; *p; p++)
			if (*p == '/') *p = '_';
	} else {
		/* Outside of /dev. Do something sensible. */
		if ((p = strrchr(s, '/')) == NULL)
			p = s;
		else
			p++;
		strncpy(res, p, reslen - 1);
		res[reslen-1] = 0;
	}

	return res;
}

int UART::CreateLockFile(const char *portName) {
#ifndef WIN32
	char buf[128];
	char *username;
	struct stat stt;
	int fd, n = 0;
	int pid;
	int mask;
	uid_t real_uid;
	gid_t real_gid;

	real_uid = getuid();
	real_gid = getgid();
	username = (getpwuid(real_uid))->pw_name;

	/* First see if the lock file directory is present. */
	if (P_LOCK[0] && stat(P_LOCK, &stt) == 0)
		snprintf(lockfile, sizeof(lockfile), "%s/LCK..%s", P_LOCK, mbasename((char *)portName, buf, sizeof(buf)));
	else
		lockfile[0] = 0;

	if (lockfile[0] && (fd = open(lockfile, O_RDONLY)) >= 0) {
		n = read(fd, buf, 127);
		close(fd);
		if (n > 0) {
			pid = -1;
			if (n == 4) {
				/* Kermit-style lockfile. */
				pid = *(int *)buf;
			} else {
				/* Ascii lockfile. */
				buf[n] = 0;
				sscanf(buf, "%d", &pid);
			}
			if (pid > 0 && kill((pid_t)pid, 0) < 0 && errno == ESRCH) {
				fprintf(stderr, "Lockfile is stale. Overriding it..\n");
				sleep(1);
				unlink(lockfile);
			} else {
				n = 0;
			}
		}

		if (n == 0) {
			fprintf(stderr, "Device %s is locked.\n", portName);
			lockfile[0] = 0;
			return -1;
		}
	}

	if (lockfile[0]) {
		/* Create lockfile compatible with UUCP-1.2 */
		mask = umask(022);
		if ((fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0) {
			fprintf(stderr, "Cannot create lockfile. Sorry.\n");
			lockfile[0] = 0;
			return -1;
		}
		(void)umask(mask);
		(void)chown(lockfile, real_uid, real_gid);
		snprintf(buf, sizeof(buf), "%10ld gtkterm %.20s\n", (long)getpid(), username);
		write(fd, buf, strlen(buf));
		close(fd);
	}

	return 0;
#else
	return 0;
#endif
}

void UART::RemoveLockFile(void) {
#ifndef WIN32
	if (lockfile[0])  unlink(lockfile);
#endif
}

void UART::ClosePortAndRemoveLockFile(void) {
	ClosePort();
	RemoveLockFile();
}

void UART::SendBreak(void) {
	if (!portOpened)	return;
#ifdef WIN32
#else
	tcsendbreak(portFd, 0);
#endif
}

void *UART::GetSerialPortFD() {
#ifdef WIN32
	return portHandle;
#else
	return (void *)(portFd);
#endif
}
