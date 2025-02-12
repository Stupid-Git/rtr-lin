/*
 * comp_acm0_thread.c
 *
 *  Created on: Dec 16, 2024
 *      Author: karel
 */


#include "_r500_config.h"


#define _COMP_ACM0_THREAD_C_
#include "comp_acm0_thread.h"

#include "r500_defs.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stropts.h>
#include <unistd.h>
#include <linux/termios.h>
#include <netinet/in.h>

//-------------------------------------------------------------
//#include "check.h"
#ifndef _CHECK_H
#define _CHECK_H

#include <stdio.h>
#include <stdlib.h>

#define check(test, msg) \
  do {           \
    if (test) {      \
      perror(msg);   \
      exit(1);       \
    }            \
  } while(0)

#endif // _CHECK_H
//-------------------------------------------------------------

//-------------------------------------------------------------
//#include "speed.h"
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
//#include <stropts.h>
#include <linux/termios.h>

//#include "check.h"
//#include "speed.h"

void setspeed(int fd, int speed) {
  struct termios2 t;
  check(ioctl(fd, TCGETS2, &t), "TCGETS2");
  t.c_cflag = (t.c_cflag & ~CBAUD) | BOTHER;
  t.c_ispeed = t.c_ospeed = speed;
  check(ioctl(fd, TCSETS2, &t), "TCSETS2");
}
//-------------------------------------------------------------

extern void cfmakeraw(struct termios *);

static int verbose = 0;

static void makeraw(int fd) {
  struct termios t;
  check(ioctl(fd, TCGETS, &t), "TCGETS");
  cfmakeraw(&t);
  check(ioctl(fd, TCSETS, &t), "TCSETS");
}

static int serialsetup(const char *name, int speed) {
  int fd = open(name, O_RDWR);
  check(fd < 0, "open");
  setspeed(fd, speed);
  makeraw(fd);
  return fd;
}

static int socketsetup(int port) {
  int listening = socket(AF_INET6, SOCK_STREAM, 0);
  check(listening < 0, "socket");
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof addr);
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port);
  int one = 1;
  check(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one), "SO_REUSEADDR");
  check(bind(listening, (const struct sockaddr *)&addr, sizeof addr), "bind");
  check(listen(listening, 5), "listen");
  return listening;
}

static void dump16(char prefix, const char *buffer, int length) {
  putchar(prefix);
  for (int i = 0; i < length; i++)
    printf(" %02x", (uint8_t) buffer[i]);
  for (int i = length; i < 16; i++)
    printf("   ");
  printf(" %c ", prefix);
  for (int i = 0; i < length; i++) {
    uint8_t c = buffer[i];
    putchar(c >= 32 && c <= 127 ? c : '.');
  }
  putchar('\n');
}

static void dump(char prefix, const char *buffer, int length) {
  for (; length >= 0; length -= 16, buffer += 16)
    dump16(prefix, buffer, length > 16 ? 16 : length);
}

static int relay(int from, int to, char prefix) {
  static char buffer[1024];
  int n = read(from, buffer, sizeof buffer);
  if (n > 0) {
    write(to, buffer, n);
    if (verbose)
      dump(prefix, buffer, n);
  }
  return n;
}

static void bridge(int sockfd, int serfd) {
  struct pollfd fds[] = {
    { sockfd, POLLIN, 0 },
    { serfd, POLLIN, 0 }
  };
  for (;;) {
    poll(fds, 2, -1);
    if (fds[0].revents & POLLIN)
      if (relay(sockfd, serfd, '>') <= 0)
    return;
    if (fds[1].revents & POLLIN)
      if (relay(serfd, sockfd, '<') <= 0) {
    fprintf(stderr, "Serial port closed\n");
    exit(0);
      }
  }
}

__attribute__((noreturn))
static void usage(int error, const char *progname) {
  fprintf(error ? stderr : stdout, "Usage: %s device speed port\n", progname);
  exit(error);
}


int acm0_main(void);
int acm0_main(void)
{
    int rtn = 0;

    int serial_fd;

    serial_fd = serialsetup("/dev/ttyUSB0", 115200);
    //serial_fd = serialsetup("/dev/ttyACM0", 115200);
    if(serial_fd < 0)
    {
        printf("Serial NG\n");
        return -1;
    }

    uint8_t rx_buf[128];

    write(serial_fd, "Hello\r\n", 7);
    printf("Wrote Hello\n");

    verbose = 1;
    for(;;)
    {
      int n = read(serial_fd, rx_buf, sizeof(rx_buf));
      if (n > 0) {
        write(serial_fd, rx_buf, n);
        if (verbose)
          dump("", rx_buf, n);
      }
    }
    return rtn;
}



int serial_bridge_main(int argc, char * const argv[])
{
  int opt;
  while ((opt = getopt(argc, argv, "hv")) != -1)
    switch(opt) {
    case 'v':
      verbose = 1;
      break;
    case 'h':
      usage(0, argv[0]);
    default:
      usage(1, argv[0]);
    }
  if (argc - optind != 3)
    usage(1, argv[0]);
  int speed = atoi(argv[optind + 1]);
  int port = atoi(argv[optind + 2]);
  if (speed == 0 || port == 0)
    usage(1, argv[0]);
  int serial = serialsetup(argv[optind], speed);
  int listening = socketsetup(port);
  for (;;) {
    if (verbose)
      printf("Waiting for incoming connection\n");
    int accepted = accept(listening, NULL, NULL);
    if (verbose)
      printf("Connection accepted\n");
    bridge(accepted, serial);
    close(accepted);
    if (verbose)
      printf("Connection closed\n");
  }
  exit(0);
}



// Reference https://github.com/samueltardieu/serialbridge/
// https://askubuntu.com/questions/112568/how-do-i-allow-a-non-default-user-to-use-serial-device-ttyusb0
//

#if 0

/*
 serialbridge - build bridges between serial ports and TCP sockets
 Copyright (c) 2013 Samuel Tardieu <sam@rfc1149.net>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <linux/termios.h>
#include <netinet/in.h>

#include "check.h"
#include "speed.h"

extern void cfmakeraw(struct termios *);

static int verbose = 0;

static void makeraw(int fd) {
  struct termios t;
  check(ioctl(fd, TCGETS, &t), "TCGETS");
  cfmakeraw(&t);
  check(ioctl(fd, TCSETS, &t), "TCSETS");
}

static int serialsetup(const char *name, int speed) {
  int fd = open(name, O_RDWR);
  check(fd < 0, "open");
  setspeed(fd, speed);
  makeraw(fd);
  return fd;
}

static int socketsetup(int port) {
  int listening = socket(AF_INET6, SOCK_STREAM, 0);
  check(listening < 0, "socket");
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof addr);
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port);
  int one = 1;
  check(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one), "SO_REUSEADDR");
  check(bind(listening, (const struct sockaddr *)&addr, sizeof addr), "bind");
  check(listen(listening, 5), "listen");
  return listening;
}

static void dump16(char prefix, const char *buffer, int length) {
  putchar(prefix);
  for (int i = 0; i < length; i++)
    printf(" %02x", (uint8_t) buffer[i]);
  for (int i = length; i < 16; i++)
    printf("   ");
  printf(" %c ", prefix);
  for (int i = 0; i < length; i++) {
    uint8_t c = buffer[i];
    putchar(c >= 32 && c <= 127 ? c : '.');
  }
  putchar('\n');
}

static void dump(char prefix, const char *buffer, int length) {
  for (; length >= 0; length -= 16, buffer += 16)
    dump16(prefix, buffer, length > 16 ? 16 : length);
}

static int relay(int from, int to, char prefix) {
  static char buffer[1024];
  int n = read(from, buffer, sizeof buffer);
  if (n > 0) {
    write(to, buffer, n);
    if (verbose)
      dump(prefix, buffer, n);
  }
  return n;
}

static void bridge(int sockfd, int serfd) {
  struct pollfd fds[] = {
    { sockfd, POLLIN, 0 },
    { serfd, POLLIN, 0 }
  };
  for (;;) {
    poll(fds, 2, -1);
    if (fds[0].revents & POLLIN)
      if (relay(sockfd, serfd, '>') <= 0)
    return;
    if (fds[1].revents & POLLIN)
      if (relay(serfd, sockfd, '<') <= 0) {
    fprintf(stderr, "Serial port closed\n");
    exit(0);
      }
  }
}

__attribute__((noreturn))
static void usage(int error, const char *progname) {
  fprintf(error ? stderr : stdout, "Usage: %s device speed port\n", progname);
  exit(error);
}

int main(int argc, char * const argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "hv")) != -1)
    switch(opt) {
    case 'v':
      verbose = 1;
      break;
    case 'h':
      usage(0, argv[0]);
    default:
      usage(1, argv[0]);
    }
  if (argc - optind != 3)
    usage(1, argv[0]);
  int speed = atoi(argv[optind + 1]);
  int port = atoi(argv[optind + 2]);
  if (speed == 0 || port == 0)
    usage(1, argv[0]);
  int serial = serialsetup(argv[optind], speed);
  int listening = socketsetup(port);
  for (;;) {
    if (verbose)
      printf("Waiting for incoming connection\n");
    int accepted = accept(listening, NULL, NULL);
    if (verbose)
      printf("Connection accepted\n");
    bridge(accepted, serial);
    close(accepted);
    if (verbose)
      printf("Connection closed\n");
  }
  exit(0);
}
#endif // 0
