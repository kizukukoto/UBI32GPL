#!/usr/bin/perl

use Socket qw(IPPROTO_TCP TCP_NODELAY);
use IO::Socket;

die "Can not connect to $ARGV[0]:$!"
	unless my $sk = IO::Socket::INET->new($ARGV[0]);

my ($host) = split(":", $ARGV[0]);
my $args = "WANMAC=00:41:71:44:55:99&LANMAC=22:22:22:22:22:22&IWMAC=44:44:44:44:44:44&DOMAIN=48";
my $arglen = length($args);

my $post = "POST /tmp/mpp HTTP/1.1
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, */*
Referer: http://${host}/tmp/mpp
Accept-Language: zh-tw
Content-Type: application/x-www-form-urlencoded
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)
Host: ${host}
Content-Length: ${arglen}
Connection: Keep-Alive
Cache-Control: no-cache

${args}";

#Begin socket session
print $post;
$sk->send($post, 1024);
#setsockopt($sk, IPPROTO_TCP, TCP_NODELAY, 1) or die "opt fail $!";
#exit 0;

$sk->recv($data, 1024);
print $data;

