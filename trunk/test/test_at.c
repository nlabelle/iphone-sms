/*
  Turbo Sim SMS Reset Utility for the iPhone -- Copyright 2007 iZsh
 
  Credits: gruffy for discovering the root cause, iZsh
  and special thanks to Zf and #iphone-turbosim for all the
  support and help while tracking this problem down.

  All code, information or data [from now on "data"] available
  from the "iPhone dev team" [1] or any other project linked from
  this or other pages is owned by the creator who created the data.
  The copyright, license right, distribution right and any other
  rights lies with the creator.
  
  It is prohibitied to use the data without the written agreement
  of the creator. This included using ideas in other projects
  (commercial or not commercial).
  
  Where data was created by more than 1 creator a written agreement
  from each of the creators has to be obtained.

  Punishment: Monkeys coming out of your ass Bruce Almighty style.

  [1] http://iphone.fiveforty.net/wiki/index.php?title=Main_Page
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

#define LOG stdout
#define SPEED 115200

#pragma pack(1)

#define ever ;;
#define BUFSIZE (65536+100)
unsigned char readbuf[BUFSIZE];

struct termios term;

#define DEBUG_ENABLED 1

#ifndef DEBUG_ENABLED
#define DEBUGLOG(x) 
#else
#define DEBUGLOG(x) x
#endif

#define UINT(x) *((unsigned int *) (x))

const char * RE = "Why the hell are you reversing this app?! We said we were "\
"going to release the sources...";

void HexDumpLine(unsigned char *buf, int remainder, int offset)
{
  int i = 0;
  char c = 0;

  // Print the hex part
  fprintf(LOG, "%08x | ", offset);
  for (i = 0; i < 16; ++i) {
    if (i < remainder)
      fprintf(LOG, "%02x%s", buf[i], (i == 7) ? "  " : " ");
    else
      fprintf(LOG, "  %s", (i == 7) ? "  " : " ");
  }
  // Print the ascii part
  fprintf(LOG, " | ");
  for (i = 0; i < 16 && i < remainder; ++i) {
    c = buf[i];
    if (c >= 0x20 && c <= 0x7e)
      fprintf(LOG, "%c%s", c, (i == 7) ? " " : "");
    else
      fprintf(LOG, ".%s", (i == 7) ? " " : "");
  }

  fprintf(LOG, "\n");
}

void HexDump(unsigned char *buf, int size)
{
  int i = 0;

  for (i = 0; i < size; i += 16)
    HexDumpLine(buf + i, size - i, i);
  fprintf(LOG, "%08x\n", size);
}

void SendCmd(int fd, void *buf, size_t size)
{
  DEBUGLOG(fprintf(LOG, "Sending:\n"));
  DEBUGLOG(HexDump((unsigned char*)buf, size));

  if(write(fd, buf, size) == -1) {
    fprintf(stderr, "Shit. %s\n", strerror(errno));
    exit(1);
  }
}

#define SendBytes(fd, args...) {\
  unsigned char sendbuf[] = {args}; \
  SendCmd(fd, sendbuf, sizeof(sendbuf)); \
}

void SendStrCmd(int fd, char *buf)
{
  SendCmd(fd, buf, strlen(buf));
}

int ReadResp(int fd)
{
  int len = 0;
  struct timeval timeout;
  int nfds = fd + 1;
  fd_set readfds;
  int select_ret;

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);

  // Wait a second
  timeout.tv_sec = 3;
  timeout.tv_usec = 500000;

  fprintf(stderr,"s");
  while (select_ret = select(nfds, &readfds, NULL, NULL, &timeout) > 0)
  {
    fprintf(stderr,"1");
    DEBUGLOG(fprintf(stderr,"select_ret = %d\n",select_ret));
    len += read(fd, readbuf + len, BUFSIZE - len);
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;
  }
  if (len > 0) {
    fprintf(stderr,"2");
    DEBUGLOG(fprintf(LOG, "Read:\n"));
    DEBUGLOG(HexDump(readbuf, len));
  }
  return len;
}

#if 0
int InitConn(int speed)
{
  int fd = open("/dev/tty.debug", O_RDWR | 0x20000 | O_NOCTTY);
  unsigned int blahnull = 0;
  unsigned int handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;

  if(fd == -1) {
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

  term.c_iflag = (term.c_iflag & 0xFFFFF0CD) | 5;
  term.c_oflag =  term.c_oflag & 0xFFFFFFFE;
  term.c_cflag = (term.c_cflag & 0xFFFC6CFF) | 0x3CB00;
  term.c_lflag =  term.c_lflag & 0xFFFFFA77;

  term.c_cflag = (term.c_cflag & ~CSIZE) | CS8;
  term.c_cflag &= ~PARENB;
  term.c_lflag &= ~ECHO;

  tcsetattr(fd, TCSANOW, &term);

  ioctl(fd, TIOCSDTR);
  ioctl(fd, TIOCCDTR);
  ioctl(fd, TIOCMSET, &handshake);

  return fd;
}
#else
int InitConn(int speed)
{
  struct termios buf;
  int rate;
  
  int fd = open ("/dev/tty.debug", O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    return fd;
  }
  /* open() calls from other applications shall fail now */
  ioctl (fd, TIOCEXCL, (char *) 0);

  if (tcgetattr (fd, &buf)) {
    fprintf (stderr, "Could not open serial port %s.\n", "/dev/tty.debug");
    return (0);
  }

  //
  // Reset to the serial port to raw mode.
  //
  buf.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
		   INLCR | IGNCR | ICRNL | IXON);
  buf.c_oflag &= ~OPOST;
  buf.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  buf.c_cflag &= ~(CSIZE | PARENB);
  buf.c_cflag |= CS8;

    //buf.c_cflag |= CRTSCTS;
    //buf.c_cflag &= ~CRTSCTS;
  //
  // Get the baud rate.
  //
  switch (speed) {
  case 9600:
    {
      rate = B9600;
      break;
    }

  case 19200:
    {
      rate = B19200;
      break;
    }

  case 38400:
    {
      rate = B38400;
      break;
    }

  case 57600:
    {
      rate = B57600;
      break;
    }

  case 115200:
    {
      rate = B115200;
      break;
    }
  }

  //
  // Set the input and output baud rate of the serial port.
  //
  cfsetispeed (&buf, rate);
  cfsetospeed (&buf, rate);
  //
  // Set data bits to 8.
  //
  buf.c_cflag &= ~CSIZE;
  buf.c_cflag |= CS8;

  //
  // Set stop bits to one.
  //
  buf.c_cflag &= ~CSTOPB;

  //
  // Disable parity.
  //
  buf.c_cflag &= ~(PARENB | PARODD);

  //
  // Set the new settings for the serial port.
  //
  if (tcsetattr (fd, TCSADRAIN, &buf)) {
    return -1;
  }
  //
  // Wait until the output buffer is empty.
  //
  tcdrain (fd);
  
  return fd;
}
#endif
void usage(char *prog)
{
  fprintf(stderr, "Usage: %s\n", prog);
  exit(1);
}

void credit(void)
{
  printf("Turbo Sim SMS Reset Utility for the iPhone -- Copyright 2007 iZsh\n\n"\
         "Credits: gruffy for discovering the root cause, iZsh\n"\
         "and special thanks to Zf and #iphone-turbosim for all the\n"\
         "support and help while tracking this problem down.\n\n"\
         "* Leet Hax not for commercial uses\n"\
         "  Punishment: Monkeys coming out of your ass Bruce Almighty style.\n\n"
         );
}

void SendAT(int fd)
{
  SendStrCmd(fd, "AT\r");
}

void AT(int fd)
{
  fprintf(stderr,"check AT");
  SendAT(fd);
  
  for (ever) {
    fprintf(stderr,".");
    if(ReadResp(fd) != 0) {
      if(strstr(readbuf,"OK"))
      {
	fprintf(stderr,"OK\n");
	break;
      }
    }
    SendAT(fd);
  }
}

typedef unsigned char	UCHAR;
typedef unsigned char	*LPSTR;
typedef unsigned char	*LPBYTE;
extern LPSTR BinToHex(LPBYTE p, int len);
extern int ComposeSubmitSms(LPBYTE pResult, int len, char *dst, char *smsc, char *utf8_str);

int SetPDUMode(int fd)
{
    AT(fd);
    SendStrCmd(fd, "ATE0\r");
    ReadResp(fd);
    SendStrCmd(fd, "AT+CMGF=0\r");
    ReadResp(fd);
    if(strstr(readbuf,"OK"))
	   return 0;
   return -1; 
}

int SendSMS(int fd, char *to, char *text)
{
    UCHAR buf[200]; 
    UCHAR at_cmd[1024];
    LPSTR strHex;
    int len;
    int pdu_len;
    int retry = 10;
    
    AT(fd);
    len = ComposeSubmitSms(buf, sizeof(buf), to, NULL, text);
    strHex = BinToHex(buf, len);
    pdu_len = strlen(strHex)/2-1;
    //printf("%d: %s\n",strlen(strHex)/2-1, strHex);
    sprintf(at_cmd,"AT+CMGS=%d\r",pdu_len);
    fprintf(stderr,"S");
    SendStrCmd(fd, at_cmd);
    do{
	len = ReadResp(fd);
    }while(!strstr(readbuf,">") && retry-- > 0);
    if(retry > 0)
    {
	fprintf(stderr,">");
	sprintf(at_cmd,"%s\032",strHex); //CTRL-Z
	SendStrCmd(fd, at_cmd);
	do{
	    fprintf(stderr,".");
	    len = ReadResp(fd);
	}while(!strstr(readbuf,"+CMGS:") && retry-- > 0);
    }
    if(retry > 0)
    {
	fprintf(stderr,"OK\n");
	return 0;//success
    }
    else{
	fprintf(stderr,"%s: %s\n",at_cmd,readbuf);
	free(strHex);
    }
    return -1;//failed
}

char * ReadSMSList(int fd)
{
  int len = 0;

  printf("Reading the SMS list (AT+CMGL)...");
  SendStrCmd(fd, "AT+CMGL\r");
  len = ReadResp(fd);
  if (!len || len < 9) {
    printf(" Error, empty or too short reply.\n");
    return NULL;
  }

  if (strstr(readbuf + len - 9, "\r\nERROR\r\n")) {
    printf(" ERROR while executing AT+CMGL.\n");
    return NULL;
  }
  if (strstr(readbuf + len - 6, "\r\nOK\r\n")) {
    printf(" OK.\n");
  } else {
    printf(" Unknown error while executing AT+CMGL.\n");
    return NULL;
  }

  return strdup(readbuf);
}

int DeleteSMS(int fd, int n)
{
  printf("Deleting SMS %d...", n);

  char *cmd = NULL;
  asprintf(&cmd, "AT+CMGD=%d\r", n);
  SendStrCmd(fd, cmd);
  int len = ReadResp(fd);
  free(cmd);

  if (!len) {
    printf("Error, empty reply.\n");
    return 0;
  }

  if (strstr(readbuf + len - 9, "\r\nERROR\r\n")) {
    printf(" ERROR while executing AT+CMGD.\n");
    return 0;
  }
  if (strstr(readbuf + len - 6, "\r\nOK\r\n")) {
    printf(" OK.\n");
  } else {
    printf(" Unknown error while executing AT+CMGL.\n");
    return 0;
  }

  return 1;
}

void DeleteAllSMS(int fd)
{
  char * sms = ReadSMSList(fd);
  if (sms == NULL)
    return;

  printf("Deleting the SMS list (AT+CMGD)...\n");
  int len = strlen(sms);
  char * current = NULL;
  for (current = strstr(sms, "\r\n+CMGL: ");
    current != NULL && current < sms + len - 1;
    current = strstr(current + 1, "\r\n+CMGL: ")) {

    int n = 0;
    sscanf(current, "\r\n+CMGL: %d", &n);
    if (n == 0) {
      printf("Could not read SMS number, skipping...\n");
    } else {
      DeleteSMS(fd, n);
    }
  }
  free(sms);

  printf("Completed.\nEnjoy!\n");
}

extern LPBYTE HexToBin(LPSTR p, int len);
extern unsigned char  *ucs2_to_utf8(unsigned short * ucs2_str, int len);
extern int DecodeSMS (LPBYTE pRecData);
void DecodeAllSMS(int fd)
{
  char * sms = ReadSMSList(fd);
  if (sms == NULL)
    return;

  printf("Decode the SMS list...\n");
  int len = strlen(sms);
  char * current = NULL;
  for (current = strstr(sms, "\r\n+CMGL: ");
    current != NULL && current < sms + len - 1;
    current = strstr(current + 1, "\r\n+CMGL: ")) {

    int n = 0;
    int s;
    int l;
    char hexStr[1024];
    sscanf(current, "\r\n+CMGL: %d,%d,,%d\r\n%s", &n, &s,&l,hexStr);
    if (n == 0) {
      printf("Could not read SMS number, skipping...\n");
    } else {
	printf("Decode %d, state = %d\n",n,s);
	unsigned char *bin = HexToBin(hexStr,l);
	DecodeSMS(bin);	
	free(bin);
    }
  }
  free(sms);

  printf("Completed.\nEnjoy!\n");
}



void ReadPB(int fd)
{
    //get pb entry number;
    //at+cpbr=?
    //+CPBR: (1-100),40,12
    unsigned char cmd[64];
    int end;
    int len;
    char * current = NULL;
  
    SendStrCmd(fd,"AT+CSCS=\"UCS2\"\r");
    ReadResp(fd); 
    SendStrCmd(fd,"at+cpbr=?\r");
    ReadResp(fd);
    sscanf(readbuf,"\r\n+CPBR: (%*d-%d),%*d,%*d",&end);
    fprintf(stderr,"%s, end=%d\n",readbuf,end);
    sprintf(cmd,"at+cpbr=1,%d\r",end);
    SendStrCmd(fd,cmd);
    len = ReadResp(fd);
    for (current = strstr(readbuf, "\r\n+CPBR: ");
	current != NULL && current < readbuf + len - 1;
	current = strstr(current + 1, "\r\n+CPBR: ")) {
	int n,i;
	unsigned char phone_num[32];
	unsigned char name[64];
	unsigned short *ucs2;
	unsigned char *utf8;	
	//"\r\n+CPBR: 1,"111111",129,"a"\r\n"
	sscanf(current, "\r\n+CPBR: %d,\"%[^\"]\",%*d,\"%[^\"]", &n,phone_num,name);
	fprintf(stderr,"%d, %s, %s\n",n,phone_num,name);
	for(i=0; i<strlen(phone_num); i++)
	    fprintf(stderr,"%02X ",phone_num[i]);
	fprintf(stderr,"\n");
	for(i=0; i<strlen(name); i++)
	    fprintf(stderr,"%02X ",name[i]);
	fprintf(stderr,"\n");
	ucs2 = (unsigned short *)HexToBin(name,strlen(name));
	fprintf(stderr,"ucs2: ");
	for(i=0; i<(strlen(name)/4); i++)
		fprintf(stderr,"%04X ",ucs2[i]);
	fprintf(stderr,"\n");
	utf8 = ucs2_to_utf8(ucs2,strlen(name)/4);
	free(ucs2);
	for(i=0; i<strlen(utf8); i++)
	    fprintf(stderr,"%02X ",utf8[i]);
	fprintf(stderr,"\n");
    }
     
}

int main(int argc, char **argv)
{
  const char * hehe = RE;
  int fd;
  
  credit();

  if (argc != 1)
    usage(argv[0]);
  
  //RestartBaseband();
  fd = InitConn(115200);

  AT(fd);
  SendStrCmd(fd,"ate0\r");
  ReadResp(fd);
  DeleteAllSMS(fd);
//  ReadPB(fd);
//  DecodeAllSMS(fd);
  close(fd);
  return 0;
}

