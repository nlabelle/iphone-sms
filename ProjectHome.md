# aSMS - another SMS program for iPhone. #
**If you want a Native SMS App,please try [ChinSMS](http://www.iphone.org.hk/cgi-bin/ch/topic_show.cgi?id=457&h=1&bpg=1&age=0) or [WeSMS](http://www.weiphone.com/attachment.php?aid=11995)**
## Why: ##
iPhone's SMS program is cool, but lose some features: forward, batch send.

## What: ##
aSMS provide forward, mailto,batch send(todo) features and can read the PhoneBook from SIM card. aSMS is a browser based program, so you can use it  on a pc remotely. Important For CJK language users, they can use a javascript IME to input their characters.

## How: ##
AT command(/dev/tty.debug) + (shttpd) ----> web browser interface


![http://iphone-sms.googlecode.com/files/sms3.png](http://iphone-sms.googlecode.com/files/sms3.png)
![http://iphone-sms.googlecode.com/files/api.png](http://iphone-sms.googlecode.com/files/api.png)

## Install: ##
### Method 1: ###
  * Using ibrickr. Select aSMS, install it.
### Method 2: ###
  * Install AppTapp installer into your iPhone
  * Use iPhone Safari, click  [here](http://playgphone.com/iPhone/repository.php),when you click, AppTapp Installer will prompt you "Add new package source" ,Select Yes.
  * Open installer  -> Sources, push Refresh.
  * You may find "aSMS" in Installer Utilities Categories, install it.
### Methor 3: ###
  1. scp aSMS-0.2.6.tgz root@<iPhone's ip>:.
  1. ssh root@<iPhone's ip>
  1. cd /
  1. tar -zxvf /var/root/aSMS-0.2.6.tgz
  1. launchctl load -w /Library/LaunchDaemons/com.googlecode.aSMS.plist

> Using it: Use Safari open the address: https://127.0.0.1; Using it remotely https://<iPhone's ip>

## Source Code ##
The source code can be downloaded via svn:
_svn checkout http://iphone-sms.googlecode.com/svn/trunk/ iphone-sms_
## Howto build ##
  * Install the toolchain. [Installing the iPhone Developer Toolchain: A simple How-To](http://www.tuaw.com/2007/09/11/installing-the-iphone-developer-toolchain-a-simple-how-to/)
  * 安装开发环境中文指南：http://playgphone.com/viewthread.php?tid=5&extra=page%3D1
## ChangeLog ##
  * 0.2.6 support  reset password for remote access. ; https://<iPhone's ip>/iPhone, browser and downalod files from iPhone; Can install using AppTapp installer.
  * 0.2.4 not always open /dev/tty.debug(don't confilict with ChinSMS); PhoneBook add iPhone's PhoneBook; fixed the [issue](http://code.google.com/p/iphone-sms/issues/detail?id=6).
  * 0.2.3\_2 list the Last Received SMS http://www.hackint0sh.org/forum/showpost.php?p=74329&postcount=5.
  * 0.2.3\_1 fix bug for parse Alphanumeric address.
  * 0.2.3 add support for read SMS from SIM card.
## Donations ##
> If you would like to donate something, you can send it via PayPal to yliqiang AT gmail.com