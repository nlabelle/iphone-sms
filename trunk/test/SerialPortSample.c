/*
    File:			SerialPortSample.c
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

// Apple internal modems default to local echo being on.If your modem has local echo disabled,
//undefine the following macro.
//
//#define LOCAL_ECHO

#define kATCommandString	"AT\r"

#ifdef LOCAL_ECHO
#define kOKResponseString	"AT\r\r\nOK\r\n"
#else
#define kOKResponseString	"\r\nOK\r\n"
#endif

enum {
	kNumRetries = 3
};

//Hold the original termios attributes so we can reset them
static struct termios gOriginalTTYAttrs;

static int      OpenSerialPort(const char *bsdPath);
static char    *LogString(char *str);
static int      InitializeModem(int fileDescriptor);
static void     CloseSerialPort(int fileDescriptor);
struct termios term;
int 
InitConn(int speed)
{
	unsigned int    blahnull = 0;
	unsigned int    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
	int             fd = open("/dev/tty.debug", O_RDWR | 0x20000 | O_NOCTTY);

	if (fd == -1) {
		fprintf(stderr, "%i(%s)\n", errno, strerror(errno));
		exit(1);
	}
	ioctl(fd, 0x2000740D);
	fcntl(fd, 4, 0);
	tcgetattr(fd, &term);

	ioctl(fd, 0x8004540A, &blahnull);
	cfsetspeed(&term, speed);
	cfmakeraw(&term);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 5;

//	term.c_iflag = (term.c_iflag & 0xFFFFF0CD) | 5;
//	term.c_oflag = term.c_oflag & 0xFFFFFFFE;
//	term.c_cflag = (term.c_cflag & 0xFFFC6CFF) | 0x3CB00;
//	term.c_lflag = term.c_lflag & 0xFFFFFA77;

	term.c_cflag = (term.c_cflag & ~CSIZE) | CS8;
	term.c_cflag &= ~PARENB;
	term.c_lflag &= ~ECHO;

	if(tcsetattr(fd, TCSANOW, &term) == -1)
	    fprintf(stderr,"TCSANOW error!\n");

	ioctl(fd, TIOCSDTR);
	ioctl(fd, TIOCCDTR);
	ioctl(fd, TIOCMSET, &handshake);

	return fd;
}

//Given the path to a serial device, open the device and configure it.
// Return the file descriptor associated with the device.
static int 
OpenSerialPort(const char *bsdPath)
{
	int             fileDescriptor = -1;
	int             handshake;
	struct termios  options;

	//Open the serial port read / write, with no controlling terminal, and don 't wait for a connection.
	// The O_NONBLOCK flag also causes subsequent I / O on the device to be non - blocking.
	// See open(2) ("man 2 open") for details
	fileDescriptor = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fileDescriptor == -1) {
		printf("Error opening serial port %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
		goto error;
	}
	//Note that open() follows POSIX semantics:multiple open() calls to the same file will succeed
	//              unless the TIOCEXCL ioctl is issued.This will prevent additional opens except by root - owned
	//              processes.
	//              See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

	fprintf(stderr,"TIOCEXCL: %x\n",TIOCEXCL);
	if(ioctl(fileDescriptor, TIOCEXCL) == -1) {
		printf("Error setting TIOCEXCL on %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
		goto error;
	}
	//Now that the device is open, clear the O_NONBLOCK flag so subsequent I / O will block.
	//              See fcntl(2) ("man 2 fcntl") for details.

	fprintf(stderr,"F_SETFL:%x\n",F_SETFL);
	if(fcntl(fileDescriptor, F_SETFL, 0) == -1) {
		printf("Error clearing O_NONBLOCK %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
		goto error;
	}
	//Get the current options and save them so we can restore the default settings later.
	if (tcgetattr(fileDescriptor, &gOriginalTTYAttrs) == -1) {
		printf("Error getting tty attributes %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
		goto error;
	}
	fprintf(stderr,"dump the tty attr:\n");
	int i;
	for(i=0; i<sizeof(struct termios); i++)
		fprintf(stderr,"%02X ",((unsigned char *)&gOriginalTTYAttrs)[i]);
	fprintf(stderr,"\n");
	//The serial port attributes such as timeouts and baud rate are set by modifying the termios
	// structure and then calling tcsetattr() to cause the changes to take effect.Note that the
	//              changes will not become effective without the tcsetattr() call.
	//              See tcsetattr(4) ("man 4 tcsetattr") for details.

        options = gOriginalTTYAttrs;

	//Print the current input and output baud rates.
	// See tcsetattr(4) ("man 4 tcsetattr") for details

	printf("Current input baud rate is %d\n", (int) cfgetispeed(&options));
	printf("Current output baud rate is %d\n", (int) cfgetospeed(&options));

	//Set raw input(non - canonical) mode, with reads blocking until either a single character
	// has been received or a one second timeout expires.
	// See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.

        cfmakeraw(&options);
	options.c_cc[VMIN] = 1;
	options.c_cc[VTIME] = 10;

	//The baud rate, word length, and handshake options can be set as follows:

	//Set 115200 baud
	cfsetspeed(&options, B115200);
//	options.c_cflag |= (CS8 | //Use 8 bit words
//		PARENB | //Parity enable(even parity if PARODD not also set)
//				    CCTS_OFLOW | //CTS flow control of output
//				    CRTS_IFLOW);
	//RTS flow control of input

	// Set data bits to 8.
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	// Disable parity.
	options.c_cflag &= ~(PARENB | PARODD);
	// Set stop bits to one.
	options.c_cflag &= ~CSTOPB;

	//Print the new input and output baud rates.Note that the IOSSIOSPEED ioctl interacts with the serial driver
	// directly bypassing the termios struct.This means that the following two calls will not be able to read
	//              the current baud rate if the IOSSIOSPEED ioctl was used but will instead return the speed set by the last call
	//              to cfsetspeed.

        printf("Input baud rate changed to %d\n", (int) cfgetispeed(&options));
	printf("Output baud rate changed to %d\n", (int) cfgetospeed(&options));

	//Cause the new options to take effect immediately.
	if (tcsetattr(fileDescriptor, TCSANOW, &options) == -1) {
		printf("Error setting tty attributes %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
		goto error;
	}
	//To set the modem handshake lines, use the following ioctls.
	// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

	if(ioctl(fileDescriptor, TIOCSDTR) == -1) //Assert Data Terminal Ready(DTR) 
	{
		printf("Error asserting DTR %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
	}

	if(ioctl(fileDescriptor, TIOCCDTR) == -1) //Clear Data Terminal Ready(DTR) 
	{
		printf("Error clearing DTR %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
	}	

	handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
	if (ioctl(fileDescriptor, TIOCMSET, &handshake) == -1)
		//Set the modem lines depending on the bits set in handshake
	{
		printf("Error setting handshake lines %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
	}
	//To read the state of the modem lines, use the following ioctl.
		// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

	if(ioctl(fileDescriptor, TIOCMGET, &handshake) == -1)
		//Store the state of the modem lines in handshake
	{
		printf("Error getting handshake lines %s - %s(%d).\n",
		       bsdPath, strerror(errno), errno);
	}
	printf("Handshake lines currently set to %d\n", handshake);

	//Success
	return fileDescriptor;

	//Failure path
error:
	if (fileDescriptor != -1) {
		close(fileDescriptor);
	}
	return -1;
}

//Replace non - printable characters in str with '\'-escaped equivalents.
// This function is used for convenient
//	logging of data traffic.
static char    *LogString(char *str) {
	static char     buf[2048];
	char           *ptr = buf;
	int             i;

	*ptr = '\0';

	while (*str) {
		if (isprint(*str)) {
			*ptr++ = *str++;
		} else {
			switch (*str) {
			case ' ':
				*ptr++ = *str;
				break;

			case 27:
				*ptr++ = '\\';
				*ptr++ = 'e';
				break;

			case '\t':
				*ptr++ = '\\';
				*ptr++ = 't';
				break;

			case '\n':
				*ptr++ = '\\';
				*ptr++ = 'n';
				break;

			case '\r':
				*ptr++ = '\\';
				*ptr++ = 'r';
				break;

			default:
				i = *str;
				(void) sprintf(ptr, "\\%03o", i);
				ptr += 4;
				break;
			}

			str++;
		}

		*ptr = '\0';
	}

	return buf;
}

//Given the file descriptor for a
//modem device, attempt to initialize the modem by sending it
// a standard AT command and reading the response.If successful, the modem 's response will be "OK".
// Return true if successful
//	,otherwise false.
static int      InitializeModem(int fileDescriptor) {
	char            buffer[256];
        //Input buffer
	char           *bufPtr;
        //Current char in buffer
        ssize_t numBytes;
        //Number of bytes read or written
	int             tries;
        //Number of tries so far
	int             result = 0;

				for             (tries = 1; tries <= kNumRetries; tries++) {
					printf("Try #%d\n", tries);

					//Send an AT command to the modem
					numBytes = write(fileDescriptor, kATCommandString, strlen(kATCommandString));

					if (numBytes == -1) {
						printf("Error writing to modem - %s(%d).\n", strerror(errno), errno);
						continue;
					} else {
						printf("Wrote %ld bytes \"%s\"\n", numBytes, LogString(kATCommandString));
					}

					if (numBytes < strlen(kATCommandString)) {
						continue;
					}
					printf("Looking for \"%s\"\n", LogString(kOKResponseString));

					//Read characters into our buffer until we get a CR or LF
						bufPtr = buffer;
					do {
						numBytes = read(fileDescriptor, bufPtr, &buffer[sizeof(buffer)] - bufPtr - 1);
						if (numBytes == -1) {
							printf("Error reading from modem - %s(%d).\n", strerror(errno), errno);
						} else if (numBytes > 0) {
							bufPtr += numBytes;
							if (*(bufPtr - 1) == '\n' || *(bufPtr - 1) == '\r') {
								break;
							}
						} else {
							printf("Nothing read.\n");
						}
					} while (numBytes > 0);

					//NUL terminate the string and see if we
				//		got an OK response
							* bufPtr = '\0';

					printf("Read \"%s\"\n", LogString(buffer));

					if (strncmp(buffer, kOKResponseString, strlen(kOKResponseString)) == 0) {
						result = 1;
						break;
					}
				}

				return result;
			}

	//Given the file descriptor for a
//		serial device, close that device.
			void            CloseSerialPort(int fileDescriptor) {
		//Block until all written output has been sent from the device.
		// Note that this call is simply passed on to the serial device driver.
		// See tcsendbreak(3) ("man 3 tcsendbreak") for details.
		if (tcdrain(fileDescriptor) == -1) {
			printf("Error waiting for drain - %s(%d).\n",
			       strerror(errno), errno);
		}
		//Traditionally it is good practice to reset a serial port back to
			// the state in which you found it.This is why the original termios struct
		              //was saved.
		if              (tcsetattr(fileDescriptor, TCSANOW, &gOriginalTTYAttrs) == -1) {
			printf("Error resetting tty attributes - %s(%d).\n",
			       strerror(errno), errno);
		}
		                close(fileDescriptor);
		}

		int             main(void) {
			int             fileDescriptor;
			/*
			 *	error number layout as follows (see mach/error.h):
			 *
			 *	hi		 		       lo
			 *	| system(6) | subsystem(12) | code(14) |
			 */
			//Now open the modem port we found, initialize the modem, then close it

				fileDescriptor = OpenSerialPort("/dev/tty.debug");
		//		fileDescriptor = InitConn(115200);
			if (-1 == fileDescriptor) {
				return EX_IOERR;
			}
			if (InitializeModem(fileDescriptor)) {
				printf("Modem initialized successfully.\n");
			} else {
				printf("Could not initialize modem.\n");
			}

			CloseSerialPort(fileDescriptor);
			printf("Modem port closed.\n");

			return EX_OK;
		}
