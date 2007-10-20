/*
 * This file is an example of how to embed web-server functionality
 * into existing application.
 * Compilation line:
 * cc example.c shttpd.c -DEMBEDDED
 */

#ifdef _WIN32
#include <winsock.h>
#define	snprintf			_snprintf
#pragma comment(lib,"ws2_32")
#else
#include <sys/types.h>
#include <sys/select.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "shttpd.h"

extern char *sqlite_exec(const char *db_name, const char *exec_str);
extern int InitConn(int speed);
extern int SendSMS(int fd, char *to, char *text);
extern int SetPDUMode(int fd);
static int gsm_modem = -1;
static int
send_sms (const char *phone, const char *text)
{
    return SendSMS(gsm_modem, phone, text);	
    return -1;
}

static int
index_html (struct shttpd_arg_t *arg)
{
  int n = 0, ret = 0;
  int *p = arg->user_data;	/* integer passed to us */
  const char *phone_num = NULL, *text = NULL;
  const char *ajax = NULL;
  int isajax = 0;
  
  if((ajax = shttpd_get_var(arg, "ajax")) != NULL)
    isajax = 1;
  if(!isajax)
  { 
    n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>Send SMS To Phone</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"/>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "</head><body>");
  }
  phone_num = shttpd_get_var (arg, "phone_num");
  text = shttpd_get_var (arg, "text");
  if(phone_num != NULL && text != NULL)
  {   
    if ((ret = send_sms (phone_num, text)) < 0) {
	n += snprintf (arg->buf + n, arg->buflen - n,
	     "<p>Could not Send SMS: %d</p>", ret);
	} else {
	     char sql[1024];
	     char *sr;
	     n += snprintf (arg->buf + n, arg->buflen - n, "<p>Sent Ok</p>");
	     sprintf(sql,"insert into message (address,text,date,flags) values('%s','%s','%d','3');",
			     phone_num,text,time(NULL));
	     sr = sqlite_exec("/var/root/Library/SMS/sms.db",sql);
	     free(sr);
	}
      if(isajax)
      {
	arg->last = 1;
	return (n);
      }
  }
 
  n += snprintf (arg->buf + n, arg->buflen - n,
		 "<form  method=\"post\">"
		 "<p>To:"
		 "<input style=\"font-size:18px\" type=\"text\" size=\"14\" maxlength=\"14\" name=\"phone_num\" value=\"%s\"/>"
		 "&nbsp;<a href=\"/simpb.html\">PhoneBook</a></p>"
		 "<textarea style=\"font-size:15px\" cols=30 rows=10 name=\"text\">%s</textarea><br/>"
		 "<input style=\"font-size:15px\" value=\"Send\" type=\"submit\"/></form>",
		 (phone_num != NULL) ? phone_num : "", (text != NULL) ? text : "");

  n += snprintf (arg->buf + n, arg->buflen - n, "<div style=\"text-align: right\"><a href=\"/setting.html\">Setting</a>&nbsp;&nbsp;<a href=\"/sms_sim.html\">SMS(SIM)</a>&nbsp;&nbsp;<a href=\"/sms.html?box=inbox\">Inbox</a>&nbsp;&nbsp;<a href=\"/sms.html?box=outbox\">Outbox</a></div>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<br/><div><a href=\"/credit.html\">Credit</a>");
  n += snprintf (arg->buf + n, arg->buflen - n, "&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/api.html\">API</a></dev>");
  n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");

  assert (n < (int) arg->buflen);
  arg->last = 1;

  return (n);
}


extern unsigned char *HexToBin(unsigned char *p, int len);
extern unsigned char  *ucs2_to_utf8(unsigned short * ucs2_str, int len);
extern int ReadPB(int fd);
extern unsigned char readbuf[];
static unsigned char cached_pb[64*1024];
static int cached_pb_len = 0;
static int
simpb_html (struct shttpd_arg_t *arg)
{
    int n = 0, ret = 0;
    char * current = NULL;
    char *sr;
    char *line;
    int sr_len;
    
    n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>PhoneBook in SIM Card</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"/>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "</head><body>");
    sr = sqlite_exec("/var/root/Library/AddressBook/AddressBook.sqlitedb","select ROWID,first from ABPerson");
    sr_len = strlen(sr);
    n += snprintf(arg->buf + n, arg->buflen - n, "<table border=\"0\" cellpadding=\"5\" align=\"center\">");
    for(line = sr; line < (sr + sr_len-1); line = strstr(line,"|\n")+2)
    {
	    int id;
	    char name[128];
	    char *phone;
	    char sql[256];
	    sscanf(line,"%d|%[^|]",&id,name);
	    fprintf(stderr,"id:%d,name:%s\n",id,name);
	    sprintf(sql,"select value from ABMultiValue where record_id=%d",id);
	    phone = sqlite_exec("/var/root/Library/AddressBook/AddressBook.sqlitedb",sql);
	    fprintf(stderr,"phone:%s\n",phone);
	    if(phone != NULL)
	    {
		phone[strlen(phone)-2] = 0;
		n += snprintf(arg->buf + n, arg->buflen - n, "<tr><td><a href=\"/?phone_num=%s\">%s</a></td><td>%s</td></tr>",
			name,name,phone
			);	
		free(phone);
	    }
    }
    if(sr)
	free(sr);
    n += snprintf (arg->buf + n, arg->buflen - n, "</table>");
    n += snprintf (arg->buf + n, arg->buflen - n, "<p align=\"center\">--------- SIM Card ---------</p>");
    if(cached_pb_len == 0)
    {
	int pbs_len = ReadPB(gsm_modem);
	memcpy(cached_pb,readbuf,pbs_len);
	cached_pb_len = pbs_len;
    }
    fprintf(stderr,"cached_pb_len: %d\n",cached_pb_len);
    n += snprintf(arg->buf + n, arg->buflen - n, "<table border=\"0\" cellpadding=\"5\" align=\"center\">");
    for (current = strstr(cached_pb, "\r\n+CPBR: ");
	current != NULL && current < cached_pb + cached_pb_len - 1;
	current = strstr(current + 1, "\r\n+CPBR: ")) {
	int pos,i;
	unsigned char phone_num[32];
	unsigned char name[64];
	unsigned short *ucs2;
	unsigned char *utf8;	
	//"\r\n+CPBR: 1,"111111",129,"a"\r\n"
	sscanf(current, "\r\n+CPBR: %d,\"%[^\"]\",%*d,\"%[^\"]", &pos,phone_num,name);
//	fprintf(stderr,"%d, %s, %s\n",pos,phone_num,name);
//	for(i=0; i<strlen(phone_num); i++)
//	    fprintf(stderr,"%02X ",phone_num[i]);
//	fprintf(stderr,"\n");
//	for(i=0; i<strlen(name); i++)
//	    fprintf(stderr,"%02X ",name[i]);
//	fprintf(stderr,"\n");
	ucs2 = (unsigned short *)HexToBin(name,strlen(name));
//	fprintf(stderr,"ucs2: ");
//	for(i=0; i<(strlen(name)/4); i++)
//		fprintf(stderr,"%04X ",ucs2[i]);
//	fprintf(stderr,"\n");
	utf8 = ucs2_to_utf8(ucs2,strlen(name)/4);
	free(ucs2);
//	for(i=0; i<strlen(utf8); i++)
//	    fprintf(stderr,"%02X ",utf8[i]);
//	fprintf(stderr,"\n");
	n += snprintf(arg->buf + n, arg->buflen - n, "<tr><td><a href=\"/?phone_num=%s\">%s</a></td><td>%s</td></tr>",
			phone_num,utf8,phone_num
			);	
    }
    n += snprintf (arg->buf + n, arg->buflen - n, "</table></body></html>");
    assert (n < (int) arg->buflen);
    arg->last = 1;

    return (n);
}

typedef unsigned char UCHAR;
typedef unsigned char *LPSTR;
typedef unsigned char *LPBYTE;
extern unsigned char  *urlencode(unsigned char *s, unsigned char *t);
extern char * ReadSMSList(int fd);
extern int DecodeSMS (LPBYTE pRecData, LPSTR *smsc, LPSTR *oa, LPSTR *text, LPSTR *date);

static int
sms_sim_html (struct shttpd_arg_t *arg)
{
    int n = 0, ret = 0;
    
    n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>SMS in SIM Card</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"/>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "</head><body>");
    
    n += snprintf(arg->buf + n, arg->buflen - n, "<div>");
    char *cmgr0 = ReadSMS(gsm_modem,0);
    if(cmgr0 != NULL)
    {
	/*+CMGR: 0,,22
	* 0891683108100005F0040D91683186517521F8000870015030224423026D4B
	*/
	char hexStr[1024];
	int state;
	int ret = sscanf(cmgr0, "\r\n+CMGR: %d,,%*d\r\n%s",&state,hexStr);
	if (ret == 2) {
		printf("Decode cmgr0, state = %d\n",state);
		unsigned char *smsc=NULL,*oa=NULL,*text=NULL,*date=NULL;
		unsigned char *bin = HexToBin(hexStr,strlen(hexStr));
		DecodeSMS(bin,&smsc,&oa,&text,&date);	
		if(oa != NULL && text != NULL)
		{
		    char encoded_url[1024];
		    urlencode(oa, encoded_url);
		    n += snprintf(arg->buf + n, arg->buflen - n, "<p>LastReceived:<br/><a href=\"/?phone_num=%s\">%s</a>&nbsp;&nbsp;%s&nbsp;&nbsp;",encoded_url,oa,date);
		    n += snprintf(arg->buf + n, arg->buflen - n, "<span>%s</span>",text);
		    n += snprintf(arg->buf + n, arg->buflen - n, "&nbsp;&nbsp;<a href=\"/?text=%s\">smsto</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"mailto:?body=%s\">mailto</a><br/>------------------------------</p>",text,text);
		}
		if(smsc != NULL)    
		    free(smsc);
		if(oa != NULL)
		    free(oa);
		//text don't need free;
		if(date != NULL)
		    free(date);
		free(bin);
	    }else{
		n += snprintf(arg->buf + n, arg->buflen - n, "<p>LastReceived:<br/>Read Error!</p>");
	    }
	free(cmgr0);
	
    }else
	n += snprintf(arg->buf + n, arg->buflen - n, "<p>LastReceived:<br/>Read Error!</p>");
	
    char *sms =  ReadSMSList(gsm_modem);
    if(sms)
    {
	int len = strlen(sms);
	char * current = NULL;
	for (current = strstr(sms, "\r\n+CMGL: ");
	    current != NULL && current < sms + len - 1;
	    current = strstr(current + 1, "\r\n+CMGL: ")) {

	    int idx = 0;
	    int s;
	    int l;
	    char hexStr[1024];
	    sscanf(current, "\r\n+CMGL: %d,%d,,%d\r\n%s", &idx, &s,&l,hexStr);
	    if (idx == 0) {
		printf("Could not read SMS number, skipping...\n");
	    } else {
		printf("Decode %d, state = %d\n",idx,s);
		unsigned char *smsc=NULL,*oa=NULL,*text=NULL,*date=NULL;
		unsigned char *bin = HexToBin(hexStr,strlen(hexStr));
		DecodeSMS(bin,&smsc,&oa,&text,&date);	
		if(oa != NULL && text != NULL)
		{
		    char encoded_url[1024];
		    urlencode(oa, encoded_url);
		    n += snprintf(arg->buf + n, arg->buflen - n, "<p><a href=\"/?phone_num=%s\">%s</a>&nbsp;&nbsp;%s&nbsp;&nbsp;",encoded_url,oa,date);
		    n += snprintf(arg->buf + n, arg->buflen - n, "<span>%s</span>",text);
		    n += snprintf(arg->buf + n, arg->buflen - n, "&nbsp;&nbsp;<a href=\"/?text=%s\">smsto</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"mailto:?body=%s\">mailto</a></p>",text,text);
		}
		if(smsc != NULL)    
		    free(smsc);
		if(oa != NULL)
		    free(oa);
		//text don't need free;
		if(date != NULL)
		    free(date);
		free(bin);
	    }
	}
	free(sms);
    }else
	n += snprintf (arg->buf + n, arg->buflen - n, "<h1>empty</h1>");
    n += snprintf (arg->buf + n, arg->buflen - n, "</div></body></html>");
    assert (n < (int) arg->buflen);
    arg->last = 1;

    return (n);
}


static int
sms_html (struct shttpd_arg_t *arg)
{
    int n = 0, ret = 0;
    char *line;
    int  sr_len;
    char *sr,*sr1;
    char *box = NULL; 
    char sql[256];
    
    n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>SMS in iPhone DataBase</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"/>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "</head><body>");
    
    sprintf(sql,"select address,text,date from message order by date desc");
    box = shttpd_get_var(arg, "box");
    if(box != NULL)
    {
	if(!strcmp(box,"inbox"))
	    sprintf(sql,"select address,text,date from message where flags<3 order by date desc");
	else if(!strcmp(box,"outbox"))
	    sprintf(sql,"select address,text,date from message where flags=3 order by date desc");
    }
    n += snprintf(arg->buf + n, arg->buflen - n, "<div>");
    sr = sqlite_exec("/var/root/Library/SMS/sms.db",sql);
    sr_len = strlen(sr);
    for(line = sr; line < (sr + sr_len-1); line = strstr(line,"|\n")+2)
    {
	    char addr[64];
	    char text[1024];
	    time_t t;
	    char sql[128];
	    int id = -1;
	    char *name;
	    char *time_str;
	    char encoded_url[1024];
	    sscanf(line,"%[^|]|%[^|]|%d",addr,text,&t);
	    sprintf(sql,"select multivalue_id from ABPhoneLastFour where value=%s",&addr[strlen(addr)-4]);
	    sr1 = sqlite_exec("/var/root/Library/AddressBook/AddressBook.sqlitedb",sql);
	    sscanf(sr1,"%d",&id);
	    free(sr1);
	    sprintf(sql,"select record_id from ABMultiValue where UID=%d",id);
	    sr1 = sqlite_exec("/var/root/Library/AddressBook/AddressBook.sqlitedb",sql);
	    sscanf(sr1,"%d",&id);
	    free(sr1);
	    sprintf(sql,"select first from ABPerson where ROWID=%d",id);
	    sr1 = sqlite_exec("/var/root/Library/AddressBook/AddressBook.sqlitedb",sql);
	    fprintf(stderr,"sr1:%s",sr1);
	    if(sr1[0] != 0)
	    {
		sr1[strlen(sr1)-2] = 0;
		name = sr1;
	    }else
		name = addr;
	    time_str = ctime(&t);
	    time_str[strlen(time_str)-2] = 0;
	    //printf("%s",line);
	    //printf("%s\n%s\n",time_str,text);
	    urlencode(addr,encoded_url);
	    n += snprintf(arg->buf + n, arg->buflen - n, "<p><a href=\"/?phone_num=%s\">%s</a>&nbsp;&nbsp;%s",encoded_url,name,ctime(&t));
	    n += snprintf(arg->buf + n, arg->buflen - n, "<span>%s</span>",text);
	    //urlencode(text, encoded_text);
	    n += snprintf(arg->buf + n, arg->buflen - n, "&nbsp;&nbsp;<a href=\"/?text=%s\">smsto</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"mailto:?body=%s\">mailto</a></p>",text,text);
	    free(sr1);

    }
    free(sr);
    n += snprintf (arg->buf + n, arg->buflen - n, "</div></body></html>");
    assert (n < (int) arg->buflen);
    arg->last = 1;

    return (n);
}

static int
credit_html (struct shttpd_arg_t *arg)
{
  int n = 0, ret = 0;

  n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>Credit</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "<link media=\"only screen and (max-device-width: 480px)\" href=\"small-device.css\" type=\"text/css\" rel=\"stylesheet\" />"
		 "</head><body>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<ul><li> iPhone dev.team (toolchain, Turbo-smsreset sample code) </li>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<li><a href=\"http://www.weiphone.com/thread-10508-1-1.html\">hulihutu</a> (find /dev/tty.debug can send SMS successfully. I try /dev/tty.baseband all day,but failed.) </li>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<li>special thanks to<a href=\"http://www.iphone.org.hk/cgi-bin/ch/topic_show.cgi?id=1005&pg=1&age=0&bpg=1#6018\">gary</a></li>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<li><a href=\"http://shttpd.sourceforge.net\">shttpd</a></li>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<li>and others...</li>");
  n += snprintf (arg->buf + n, arg->buflen - n, "</ul>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<a href=mailto:yliqiang@gmail.com>me</a>");
  n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
  arg->last = 1;
  return n;
}

extern void md5(char *buf, ...);
static int
setting_html(struct shttpd_arg_t *arg)
{
    int n = 0, ret = 0;
    const char *username=NULL;
    const char *pass1=NULL;
    const char *pass2=NULL;
   
    n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>Setting</title>"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"/>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "</head><body>");
    if((username = shttpd_get_var(arg, "user")) != NULL)
    {
	if((pass1 = shttpd_get_var(arg, "pass1")) != NULL)
 	{
		if((pass2 = shttpd_get_var(arg, "pass2")) != NULL)
		{
			fprintf(stderr,"user:%s\n",username);
			fprintf(stderr,"pass1:%s\n",pass1);
			fprintf(stderr,"pass2:%s\n",pass2);
			if(strcmp(pass1,pass2))
			{
				n += snprintf (arg->buf + n, arg->buflen - n, "Password Mismatch");
  				n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
				arg->last = 1;
  				return n;
			}
			char buf[128];
			md5(buf,username,":","mydomain.com",":",pass1);
			fprintf(stderr,"md5:%s\n",buf);
			FILE *fp = fopen("/usr/local/etc/htpasswd","w+");
			if(fp)
			{
			    char htpasswd[128];
			    sprintf(htpasswd,"%s:%s:%s",username,"mydomain.com",buf);
			    fprintf(fp,htpasswd);
			    fclose(fp);
			    n += snprintf (arg->buf + n, arg->buflen - n, "Reset Password Successfully.");
			    n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
			}else{
			    n += snprintf (arg->buf + n, arg->buflen - n, "Reset Password Error.");
			    n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
			}
			arg->last = 1;
			return n;
		}
	}
    }
    n += snprintf (arg->buf + n, arg->buflen - n,
		 "<form  method=\"post\">"
		 "<p>Reset Username and Password:</p>"
		 "<p>User Name:<input type=\"text\" maxlength=\"14\" name=\"user\" value=\"\"/></p>"
		 "<p>Password:<input type=\"password\" maxlength=8 name=\"pass1\" value=\"\"/></p>"
		 "<p>Confirm Password:<input type=\"password\" maxlength=8 name=\"pass2\" value=\"\"/></p>"
		 "<input value=\"Generate\" type=\"submit\"/></form>");
  n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
  arg->last = 1;
  return n;

	
}

static int
api_html(struct shttpd_arg_t *arg)
{
  int n = 0, ret = 0;

  n += snprintf (arg->buf + n, arg->buflen - n, "%s",
		 "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n\r\n"
		 "<html><head><title>API</title>"
		 "<meta name=\"viewport\" content=\"width=320, initial-scale=1.0\" />"
		 "<link media=\"only screen and (max-device-width: 480px)\" href=\"small-device.css\" type=\"text/css\" rel=\"stylesheet\" />"
		 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\"></head><body>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<p>[https://ip]/?ajax=1&phone_num=xxx...&text=(utf8 string)&nbsp;&nbsp;<a href=\"test.html\">example</a>,POST is also support.</p>");
  n += snprintf (arg->buf + n, arg->buflen - n, "<p>[https://ip]/sqlite.html?db=/path/db_name&sql_exec=sql&nbsp;&nbsp;<a href=\"sqlite.html\">example</a></p>");
  n += snprintf (arg->buf + n, arg->buflen - n, "</body></html>");
  arg->last = 1;
  return n;
    
}

extern int sqlite_html(struct shttpd_arg_t *arg);

#ifndef CONFIG
#define	CONFIG		"/usr/local/etc/shttpd.conf"	/* Configuration file		*/
#endif /* CONFIG */
extern time_t	current_time;	/* No need to keep per-context time */
extern int	fork_background;
extern struct shttpd_ctx *do_init(const char *, int argc, char *argv[]);

int
main (int argc, char *argv[])
{
  int sock, data = 1234567;
  struct shttpd_ctx *ctx;
  const char	*config_file;	/* Configuration file */

#ifndef _WIN32
  signal (SIGPIPE, SIG_IGN);
#endif /* !_WIN32 */

  ctx = shttpd_init (NULL, "document_root", "/usr/local/isms", "aliases", "/=/iPhone", "global_htpasswd", "/usr/local/etc/htpasswd","ssl_certificate", "/usr/local/etc/shttpd.pem", NULL);
  
  current_time = time(NULL);
  config_file = CONFIG;
  if (argc > 1 && argv[argc - 2][0] != '-' && argv[argc - 1][0] != '-')
    config_file = argv[argc - 1];
  ctx = do_init(config_file, argc, argv);

  if(fork_background)
  {
    int i = fork();
    if (i<0) exit(1); /* fork error */
    if (i>0) exit(0); /* parent exits */
  }
  /*
   * Initialize SHTTPD context.
   * Attach folder c:\ to the URL /my_c  (for windows), and
   * /etc/ to URL /my_etc (for UNIX). These are Apache-like aliases.
   * Set WWW root to current directory.
   */
//    if(gsm_modem < 0)
//   {
//	gsm_modem = InitConn(115200);
//	SetPDUMode(gsm_modem);
//    }

  /* Let pass 'data' variable to callback, user can change it */
  shttpd_register_url (ctx, "/", &index_html, (void *) &data);
  shttpd_register_url (ctx, "/simpb.html", &simpb_html,NULL);
  shttpd_register_url (ctx, "/credit.html", &credit_html, NULL);
  shttpd_register_url (ctx, "/api.html", &api_html, NULL);
  shttpd_register_url (ctx, "/sqlite.html", &sqlite_html, NULL);
  shttpd_register_url (ctx, "/sms.html", &sms_html, NULL);
  shttpd_register_url (ctx, "/sms_sim.html", &sms_sim_html, NULL);
  shttpd_register_url (ctx, "/setting.html", &setting_html, NULL);
  /* Open listening socket */
  sock = shttpd_open_port (443);
  shttpd_listen (ctx, sock);

  /* Serve connections infinitely until someone kills us */
  for (;;)
    shttpd_poll (ctx, 1000);

  /* Probably unreached, because we will be killed by signal */
  shttpd_fini (ctx);
  if(gsm_modem > 0)
	close(gsm_modem);
  return (EXIT_SUCCESS);
}

