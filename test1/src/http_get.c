/*
 * http_get.c
 *
 *  Created on: Oct 16, 2024
 *      Author: karel
 */

// https://github.com/langhai/http-client-c
#include "http-client-c.h"

void http_get_1(void)
{
	struct parsed_url *purl = parse_url("http://www.google.com/");
	struct http_response *hrep = http_req("GET / HTTP/1.1\r\nHostname:www.google.com\r\nConnection:close\r\n\r\n", purl);

	printf("hrep = %s\r\n", hrep->body);
	printf("hrep = %s\r\n", hrep->request_headers);
	printf("hrep = %s\r\n", hrep->request_uri->uri);
	printf("hrep = %s\r\n", hrep->response_headers);
	printf("hrep = %s\r\n", hrep->status_code);
	printf("hrep = %d\r\n", hrep->status_code_int);
	printf("hrep = %s\r\n", hrep->status_text);

}

static char scratch[1024];
void remove_CR(char * in, char* out)
{
	while(*in != '\0')
	{
		if(*in == '\r')
			in++;
		else
			*out++ = *in++;
	}
	*out = '\0';
}

char * get_c1(void);
void test_push(void);
void test_push(void)
{
	char * bb;

	bb = get_c1();
//	struct http_response *hresp = http_post("http://mywebsite.com/login.php", "User-agent:MyuserAgent\r\n", "username=Kirk&password=lol123");
//	struct http_response *hresp = http_post("http://api.webstorage.jp/api/dev-7wf-1.php", bb, NULL);

  //struct parsed_url *purl = parse_url("http://api.webstorage.jp/api/dev-7wf-1.php");
	struct parsed_url *purl = parse_url("http://develop.webstorage.jp/api/dev-7wf-1.php");

	if(purl == NULL)
	{
		printf("Unable to parse url");
		return; // NULL;
	}

	struct http_response *hresp = http_req(bb, purl);

    printf("\n");
    //printf("[request_headers]\n%s\n", hresp->request_headers);
	remove_CR(hresp->request_headers, scratch);
    printf("[request_headers]\n%s\n", scratch);
    //char *request_headers;

    printf("\n");
    //printf("[response_headers]\n%s\n", hresp->response_headers);
    //char *response_headers;
	remove_CR(hresp->response_headers, scratch);
    printf("[response_headers]\n%s\n", scratch);

    printf("\n");
    //printf("[body]\n%s\n", hresp->body);
    //char *body;
	remove_CR(hresp->body, scratch);
    printf("[body]\n%s\n", scratch);


    /*
      OK. get a response (an error response)
		 struct http_response *hresp = http_get("http://develop.webstorage.jp/api/rtr500/device/", "User-agent:MyUserAgent\r\n");
		 [body]
		{"error":{"code":400,"message":"Bad Request: The requested contents contain a format error."}}
    */

    /* this was NG, because it needs https (I guess)
		    //https://os.mbed.com/media/uploads/mbed_official/hello.txt
		    //struct http_response *hresp = http_get("http://os.mbed.com/media/uploads/mbed_official/hello.txt", "User-agent:MyUserAgent\r\n");
    */
}


void test1(void)
{
    struct http_response *hresp = http_get("http://www.google.com", "User-agent:MyUserAgent\r\n");

	//struct parsed_url *request_uri;

    //printf("[body]\r\n%s\r\n", hresp->body);
	//char *body;
    //printf("[status_code]\r\n%s\r\n", hresp->status_code);
	//char *status_code;
    //printf("[status_code_int]\r\n%d\r\n", hresp->status_code_int);
	//int status_code_int;
    //printf("[status_text]\r\n%s\r\n", hresp->status_text);
	//char *status_text;

    printf("\r\n");
    printf("[request_headers]\r\n%s\r\n", hresp->request_headers);
	//char *request_headers;
    printf("\r\n");
    printf("[response_headers]\r\n%s\r\n", hresp->response_headers);
	//char *response_headers;

}


void test_edgecastcdn(void)
{
    //http://93.184.216.34/index.html

    /* http https etc is stripped, so this is only for http i.e. port 80
    struct http_response *hresp = http_get("https://edgecastcdn.net/index.html", "User-agent:MyUserAgent\r\n");
    */
    struct http_response *hresp = http_get("http://edgecastcdn.net/index.html", "User-agent:MyUserAgent\r\n");
  //struct http_response *hresp = http_get("http://93.184.216.34/index.html", "User-agent:MyUserAgent\r\n");

	//struct parsed_url *request_uri;

    //printf("[status_code]\r\n%s\r\n", hresp->status_code);
	//char *status_code;
    //printf("[status_code_int]\r\n%d\r\n", hresp->status_code_int);
	//int status_code_int;
    //printf("[status_text]\r\n%s\r\n", hresp->status_text);
	//char *status_text;

    printf("\r\n");
    printf("[request_headers]\r\n%s\r\n", hresp->request_headers);
	//char *request_headers;

    printf("\r\n");
    printf("[response_headers]\r\n%s\r\n", hresp->response_headers);
	//char *response_headers;

    printf("\r\n");
    printf("[body]\r\n%s\r\n", hresp->body);
	//char *body;

}

void test_tandd(void)
{
    //https://api.webstorage.jp/ping.php
    //http://api.webstorage.jp/ping.php

    // OK
    struct http_response *hresp = http_get("http://api.webstorage.jp/ping.php", "User-agent:MyUserAgent\r\n");

/* OK. get a response (an error response)
    struct http_response *hresp = http_get("http://develop.webstorage.jp/api/rtr500/device/", "User-agent:MyUserAgent\r\n");
 [body]
{"error":{"code":400,"message":"Bad Request: The requested contents contain a format error."}}
 */

/* this was NG, because it needs https (I guess)
    //https://os.mbed.com/media/uploads/mbed_official/hello.txt
    //struct http_response *hresp = http_get("http://os.mbed.com/media/uploads/mbed_official/hello.txt", "User-agent:MyUserAgent\r\n");
*/

    printf("\r\n");
    printf("[request_headers]\r\n%s\r\n", hresp->request_headers);
	//char *request_headers;

    printf("\r\n");
    printf("[response_headers]\r\n%s\r\n", hresp->response_headers);
	//char *response_headers;

    printf("\r\n");
    printf("[body]\r\n%s\r\n", hresp->body);
	//char *body;

/*

karel@DEV10 MINGW64 /c/home/public/prog/posix/http-client-c
$ ./hc.exe
Hello - 0
Hello - 2

[request_headers]
GET /ping.php HTTP/1.1
Host:api.webstorage.jp
Connection:close
User-agent:MyUserAgent



[response_headers]
P▒HTTP/1.1 200 OK
Content-Type: text/html
Date: Fri, 30 Sep 2022 01:39:44 GMT
Server: nginx
transfer-encoding: chunked
Connection: Close

[body]
a9
<html>
<head>
        <meta http-equiv="Content-Language" content="ja">
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
</head>
<body>Health</body>
</html>
0

karel@DEV10 MINGW64 /c/home/public/prog/posix/http-client-c
*/
}



void http_get_main(void);
void http_get_main(void)
{
	//http_get_1();
    //test1();
    //test_edgecastcdn();
    test_tandd();

}
