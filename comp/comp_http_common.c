/*
 * comp_http_common.c
 *
 *  Created on: Dec 9, 2024
 *      Author: karel
 */

#include "_r500_config.h"

#define _COMP_HTTP_COMMON_C_
#include "comp_http_common.h"

#include "r500_defs.h"



#pragma GCC diagnostic ignored "-Wwrite-strings"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
    #pragma comment(lib, "Ws2_32.lib")
//#elif _LINUX
#elif __linux__
    #include <sys/socket.h>
//karel
    #include <netinet/in.h> // for IPPROTO_TCP
//karel
#elif __FreeBSD__
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#else
    #error Platform not suppoted.
#endif

#include <errno.h>
#include "stringx.h";
#include "urlparser.h"


/*
    Represents an HTTP html response
*/
struct http_response
{
    struct parsed_url *request_uri;
    char *body;
    char *status_code;
    int status_code_int;
    char *status_text;
    char *request_headers;
    char *response_headers;
};


/*
    Prototype functions
*/
struct http_response* http_req(char *http_headers, struct parsed_url *purl);
struct http_response* http_get(char *url, char *custom_headers);
struct http_response* http_head(char *url, char *custom_headers);
struct http_response* http_post(char *url, char *custom_headers, char *post_data);


/*
    Makes a HTTP request and returns the response
*/
struct http_response* http_req(char *http_headers, struct parsed_url *purl)
{
    /* Parse url */
    if(purl == NULL)
    {
        printf("Unable to parse url");
        return NULL;
    }

    /* Declare variable */
    int sock;
    int tmpres;
    char buf[BUFSIZ+1];
    struct sockaddr_in *remote;

    /* Allocate memeory for htmlcontent */
    struct http_response *hresp = (struct http_response*)malloc(sizeof(struct http_response));
    if(hresp == NULL)
    {
        printf("Unable to allocate memory for htmlcontent.");
        return NULL;
    }
    hresp->body = NULL;
    hresp->request_headers = NULL;
    hresp->response_headers = NULL;
    hresp->status_code = NULL;
    hresp->status_text = NULL;

    /* Create TCP socket */
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Can't create TCP socket");
        return NULL;
    }

    /* Set remote->sin_addr.s_addr */
    remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
    remote->sin_family = AF_INET;
    tmpres = inet_pton(AF_INET, purl->ip, (void *)(&(remote->sin_addr.s_addr)));
    if( tmpres < 0)
    {
        printf("Can't set remote->sin_addr.s_addr");
        return NULL;
    }
    else if(tmpres == 0)
    {
        printf("Not a valid IP");
        return NULL;
    }
    remote->sin_port = htons(atoi(purl->port));

    /* Connect */
    if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
    {
        printf("Could not connect");
        return NULL;
    }

    /* Send headers to server */
    int sent = 0;
    while(sent < strlen(http_headers))
    {
        tmpres = send(sock, http_headers+sent, strlen(http_headers)-sent, 0);
        if(tmpres == -1)
        {
            printf("Can't send headers");
            return NULL;
        }
        sent += tmpres;
     }

    /* Recieve into response*/
    char *response = (char*)malloc(0);
    char BUF[BUFSIZ];
    size_t recived_len = 0;
    while((recived_len = recv(sock, BUF, BUFSIZ-1, 0)) > 0)
    {
        BUF[recived_len] = '\0';
        response = (char*)realloc(response, strlen(response) + strlen(BUF) + 1);
        sprintf(response, "%s%s", response, BUF);
//karel
printf("RXlen = %d\n", recived_len);
break; // ASSUMING WE GOT ALL THE SHIT
//karel
    }
    if (recived_len < 0)
    {
        free(http_headers);
        #ifdef _WIN32
            closesocket(sock);
        #else
            close(sock);
        #endif
        printf("Unabel to recieve");
        return NULL;
    }

    /* Reallocate response */
    response = (char*)realloc(response, strlen(response) + 1);

    Printf("=================================================\n");
    Printf("%s", response);
    Printf("=================================================\n");

    /* Close socket */
    #ifdef _WIN32
        closesocket(sock);
    #else
        close(sock);
    #endif

    /* Parse status code and text */
    char *status_line = get_until(response, "\r\n");
    status_line = str_replace("HTTP/1.1 ", "", status_line);
    char *status_code = str_ndup(status_line, 4);
    status_code = str_replace(" ", "", status_code);
    char *status_text = str_replace(status_code, "", status_line);
    status_text = str_replace(" ", "", status_text);
    hresp->status_code = status_code;
    hresp->status_code_int = atoi(status_code);
    hresp->status_text = status_text;

    /* Parse response headers */
    char *headers = get_until(response, "\r\n\r\n");
    hresp->response_headers = headers;

    /* Assign request headers */
    hresp->request_headers = http_headers;

    /* Assign request url */
    hresp->request_uri = purl;

    /* Parse body */
    char *body = strstr(response, "\r\n\r\n");
    body = str_replace("\r\n\r\n", "", body);
    hresp->body = body;

    /* Return response */
    return hresp;
}

void do_webStorage_Action(char* pH, char* pB);
void do_webStorage_Action(char* pH, char* pB)
{
    //https://api.webstorage.jp/ping.php
    //http://api.webstorage.jp/ping.php

    // OK
    //struct http_response *hresp = http_get("http://api.webstorage.jp/ping.php", "User-agent:MyUserAgent\r\n");

/* OK. get a response (an error response)
    struct http_response *hresp = http_get("http://develop.webstorage.jp/api/rtr500/device/", "User-agent:MyUserAgent\r\n");
 [body]
{"error":{"code":400,"message":"Bad Request: The requested contents contain a format error."}}
 */

/* this was NG, because it needs https (I guess)
    //https://os.mbed.com/media/uploads/mbed_official/hello.txt
    //struct http_response *hresp = http_get("http://os.mbed.com/media/uploads/mbed_official/hello.txt", "User-agent:MyUserAgent\r\n");
*/
    //struct parsed_url *purl = parse_url("http://api.webstorage.jp/api/dev-7wf-1.php");

    //struct parsed_url *purl = parse_url("http://develop.webstorage.jp/api/dev-7wf-1.php");

    struct parsed_url *purl = parse_url("http://develop.webstorage.jp/api/rtr500/device/");
    //struct parsed_url *purl = parse_url("http://develop.webstorage.jp/api/rtr500/device/index.php"); //??NG??
    if(purl == NULL)
    {
        printf("Unable to parse url");
        return; // NULL;
    }


    /* Declare variable */
    //char *http_headers = (char*)malloc(1024);
    char *http_headers = (char*)malloc(8192);

    sprintf(http_headers, "%s", pH);
    sprintf(http_headers, "%s%s\r\n", http_headers, pB);

    http_headers = (char*)realloc(http_headers, strlen(http_headers) + 1);


    struct http_response *hresp = http_req(http_headers, purl);


    printf("\r\n");
    printf("[request_headers]\r\n%s\r\n", hresp->request_headers);
    //char *request_headers;

    printf("\r\n");
    printf("[response_headers]\r\n%s\r\n", hresp->response_headers);
    //char *response_headers;

    printf("\r\n");
    printf("[body]\r\n%s\r\n", hresp->body);
    //char *body;

    free(http_headers);
    free(hresp);
}

/*
"POST /api/rtr500/device/ HTTP/1.1\r\nHost: develop.webstorage.jp\r\nContent-Type: application/xml; charset=UTF-8\r\nContent-Length: 1399\r\nUser-Agent: RTR500BW/R.01.00.29\r\nX-Tandd-API-Version: 1.0\r\nX-Tandd-B"...
    Default:0x55555569aa20
     *
 */
