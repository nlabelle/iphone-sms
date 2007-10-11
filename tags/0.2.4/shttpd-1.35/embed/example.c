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

#include "shttpd.h"

static int
index_html(struct shttpd_arg_t *arg)
{
	int		n = 0;
	int		*p = arg->user_data;	/* integer passed to us */
	FILE		*fp;
	const char	*value, *fname, *data, *host;

	/* Change the value of integer variable */
	if ((value = shttpd_get_var(arg, "name1")) != NULL)
		*p = atoi(value);

	n += snprintf(arg->buf + n, arg->buflen - n, "%s",
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/html\r\n\r\n<html><body>");
	
	n += snprintf(arg->buf + n, arg->buflen - n, "<h1>Welcome to embedded"
	    " example of SHTTPD v. %s </h1>", shttpd_version());

	n += snprintf(arg->buf + n, arg->buflen - n,
	    "<h2>Internal int variable value: [%d]</h2>", *p);
	
	n += snprintf(arg->buf + n, arg->buflen - n, "%s",
	    "<form method=\"POST\">"
	    "Enter new value: "
	    "<input type=\"text\" name=\"name1\"/>"
	    "<input type=\"submit\">"
	    "</form><ul>"
	    "<li><a href=\"/secret\">Protected page</a> (guest:guest)"
	    "<li><a href=\"/huge\">Output 1Mb of data</a>"
	    "<li><a href=\"/myetc/\">Aliased /etc directory</a>"
	    "<li><a href=\"/Makefile\">On-disk file (Makefile)</a>");

	host = shttpd_get_header(arg, "Host");
	n += snprintf(arg->buf + n, arg->buflen - n,
	    "<li>'Host' header value: [%s]", host ? host : "NOT SET");

	n += snprintf(arg->buf + n, arg->buflen - n,
	    "<li>This example shows that it is possible to POST "
	    "big volumes of data, and save it into a file");

	/* Save POSTed data */
	if ((fname = shttpd_get_var(arg, "filename")) != NULL) {
		if ((fp = fopen(fname, "w+")) != NULL) {
			if ((data = shttpd_get_var(arg, "data")) == NULL) {
				n += snprintf(arg->buf + n, arg->buflen - n,
			    	    "<p>Could not get data</p>");
			} else if (fwrite(data, strlen(data), 1, fp) != 1) {
				n += snprintf(arg->buf + n, arg->buflen - n,
			    	    "<p>Could not write data: %s</p>",
				    strerror(errno));
			}
			(void) fclose(fp);
		} else {
			n += snprintf(arg->buf + n, arg->buflen - n,
			    "<p>Error opening %s: %s</p>",
			    fname, strerror(errno));
		}
	}
	
	n += snprintf(arg->buf + n, arg->buflen - n,
	    "<form method=\"post\">"
	    "<p>Enter file name to store POSTed data: "
	    "<input type=\"text\" name=\"filename\" value=\"%s\">"
	    "<p>Data to be stored:<br>"
	    "<textarea cols=68 rows=25 name=\"data\">hello</textarea><br>"
	    "<input type=\"submit\"></form>",
	    fname ? fname : "");

	n += snprintf(arg->buf + n, arg->buflen - n, "</body></html>");

	assert(n < (int)arg->buflen);
	arg->last = 1;

	return (n);
}

static int
secret_html(struct shttpd_arg_t *arg)
{
	int		n = 0;

	n = snprintf(arg->buf, arg->buflen, "%s",
	    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
	    "\r\n<html><body><p>This is a protected page</body></html>");
	arg->last = 1;
	
	return (n);
}

static int
huge_html(struct shttpd_arg_t *arg)
{
	int		state = (int) arg->state, n = 0;
#define	BYTES_TO_OUTPUT	100000

	if (state == 0) {
		n = snprintf(arg->buf, arg->buflen, "%s",
		"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
	}

	for (; n < (int) arg->buflen; n++, state++) {
		arg->buf[n] = state % 72 ? 'A' : '\n';
		if (state > BYTES_TO_OUTPUT) {
			arg->last = 1;
			break;
		}
	}
	arg->state = (void *) state;
	
	return (n);
}

int main(int argc, char *argv[])
{
	int			sock, data = 1234567;
	struct shttpd_ctx	*ctx;
	
	/* Get rid of warnings */
	argc = argc;
	argv = argv;

#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif /* !_WIN32 */

	/*
	 * Initialize SHTTPD context.
	 * Attach folder c:\ to the URL /my_c  (for windows), and
	 * /etc/ to URL /my_etc (for UNIX). These are Apache-like aliases.
	 * Set WWW root to current directory.
	 */
	ctx = shttpd_init(NULL,
		"aliases", "c:\\/->/my_c,/etc/->/my_etc",
		"document_root", ".", 
		"ssl_certificate", "shttpd.pem", NULL);

	/* Let pass 'data' variable to callback, user can change it */
	shttpd_register_url(ctx, "/", &index_html, (void *) &data);
	shttpd_register_url(ctx, "/abc.html", &index_html, (void *) &data);

	/* Show how to use password protection */
	shttpd_register_url(ctx, "/secret", &secret_html, (void *) &data);
	shttpd_protect_url(ctx, "/secret", "passfile");

	/* Show how to use stateful big data transfer */
	shttpd_register_url(ctx, "/huge", &huge_html, NULL);

	/* Open listening socket */
	sock = shttpd_open_port(9000);
	shttpd_listen(ctx, sock);

	/* Serve connections infinitely until someone kills us */
	for (;;)
		shttpd_poll(ctx, 1000);

	/* Probably unreached, because we will be killed by signal */
	shttpd_fini(ctx);

	return (EXIT_SUCCESS);
}
