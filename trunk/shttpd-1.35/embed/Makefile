smsd: sms.c serial.c
	gcc -o smsd -I. serial.c sms.c ../shttpd.c  -DEMBEDDED -ldl
clean:
	rm -rf *.o smsd
