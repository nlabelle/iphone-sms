SRCS=sms.c sqlite.c shttpd-1.35/shttpd.c sms_codec.c at.c utf8.c
OBJS=$(SRCS:%.c=%.o)
CC=arm-apple-darwin-gcc
CFLAGS=-fsigned-char -DEMBEDDED -DNO_CGI -Ishttpd-1.35/embed
LDFLAGS=-Wl,-syslibroot,/usr/local/arm-apple-darwin/heavenly -lsqlite3
LD=$(CC)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

iSMS: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -rf $(OBJS) iSMS

