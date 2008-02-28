#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "config.h"
#include "client_daemon.h"

#define DRCOMC_LOGIN 1
#define DRCOMC_LOGOUT 2
#define DRCOMC_PASSWD 3

void usage(void);
int print_result(const unsigned char *, const char *);

int main(int argc, char **argv)
{
  struct drcomcd_hdr cd_hdr;
  struct drcomcd_login cd_login;
  struct drcomcd_logout cd_logout;
  struct drcomcd_passwd cd_passwd;
  int s, r = 0, mode = 0;
  struct sockaddr_un un_daemon;
  unsigned char d[DRCOMCD_BUF_LEN], e[DRCOMCD_BUF_LEN];

  if (argc < 2 || argc > 3)
    usage();

  if (!strcmp("login", argv[1]) && argc == 2)
    mode = DRCOMC_LOGIN;
  else if (!strcmp("logout", argv[1]) && argc == 2)
    mode = DRCOMC_LOGOUT;
  else if (!strcmp("passwd", argv[1]) && argc == 3)
    mode = DRCOMC_PASSWD;
  else
    usage();

  /* If all is ok, we can proceed */

  memset(&un_daemon, 0x00, sizeof(struct sockaddr_un));
  un_daemon.sun_family = AF_UNIX;
  /* abstract namespace */
  strncpy(&un_daemon.sun_path[1], DRCOMCD_SOCK, sizeof(un_daemon.sun_path)-1);

#ifdef DRCOM_DEBUG
  fprintf(stderr, "%10u DEBUG drcomc: Creating socket...\n", (unsigned) time(NULL));
#endif
  s = socket(PF_UNIX, SOCK_STREAM, 0);
  if (s == -1)
  {
    perror("drcomc: Socket creation");
    exit(EXIT_FAILURE);
  }
#ifdef DRCOM_DEBUG
  fprintf(stderr, "%10u DEBUG drcomc: Connecting...\n", (unsigned) time(NULL));
#endif
  r = connect(s, (struct sockaddr *) &un_daemon, sizeof(un_daemon));
  if (r)
  {
    perror("drcomc: Connect");
    exit(EXIT_FAILURE);
  }

  /* This is used to check for the signature, so we have to initialize it */
  memset(e, 0, sizeof(e));

  switch (mode)
  {
    case DRCOMC_LOGIN:
      /* Sending part */
      cd_hdr.signature = DRCOM_SIGNATURE;
      cd_hdr.type = DRCOMCD_LOGIN;
      /* login options */
      cd_login.authenticate = 1;
      cd_login.timeout = -1;
      memcpy(d, &cd_hdr, sizeof(cd_hdr));
      memcpy(d + sizeof(cd_hdr), &cd_login, sizeof(cd_login));
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Sending login command...\n", (unsigned) time(NULL));
#endif
      r = send(s, d, sizeof(cd_hdr) + sizeof(cd_login), 0);
      if (r < 0)
      {
        perror("drcomc: send");
        r = EXIT_FAILURE;
        break;
      }
      /* Receiving part */
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Receiving reply (login)...\n", (unsigned) time(NULL));
#endif
      r = recv(s, e, DRCOMCD_BUF_LEN, 0);
      if (r < 0)
        perror("drcomc: recv");
      if (r < 1 || ((struct drcomcd_hdr *) e)->signature != DRCOM_SIGNATURE)
      {
#ifdef DRCOM_DEBUG
        fprintf(stderr, "%10u DEBUG drcomc: r=%d signature=0x%08x\n", (unsigned) time(NULL), r, ((struct drcomcd_hdr *) e)->signature);
#endif
        printf("drcomc: Unknown signature\n");
        r = EXIT_FAILURE;
        break;
      }
      r = print_result(e, "Login");
      break;
    case DRCOMC_LOGOUT:
      /* Sending part */
      cd_hdr.signature = DRCOM_SIGNATURE;
      cd_hdr.type = DRCOMCD_LOGOUT;
      /* logout options */
      cd_logout.timeout = -1;
      memcpy(d, &cd_hdr, sizeof(cd_hdr));
      memcpy(d + sizeof(cd_hdr), &cd_logout, sizeof(cd_logout));
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Sending logout command...\n", (unsigned) time(NULL));
#endif
      r = send(s, d, sizeof(cd_hdr) + sizeof(cd_logout), 0);
      if (r < 0)
      {
        perror("drcomc: send");
        r = EXIT_FAILURE;
        break;
      }
      /* Receiving part */
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Receiving reply (logout)...\n", (unsigned) time(NULL));
#endif
      r = recv(s, e, DRCOMCD_BUF_LEN, 0);
      if (r < 0)
        perror("daemon: recv");
      if (r < 1 || ((struct drcomcd_hdr *) e)->signature != DRCOM_SIGNATURE)
      {
#ifdef DRCOM_DEBUG
        fprintf(stderr, "%10u DEBUG drcomc: r=%d signature=0x%08x\n", (unsigned) time(NULL), r, ((struct drcomcd_hdr *) e)->signature);
#endif
        printf("drcomc: Unknown signature\n");
        r = EXIT_FAILURE;
        break;
      }
      r = print_result(e, "Logout");
      break;
    case DRCOMC_PASSWD:
      /* Sending part */
      cd_hdr.signature = DRCOM_SIGNATURE;
      cd_hdr.type = DRCOMCD_PASSWD;
      /* password change options */
      memset(cd_passwd.newpasswd, 0, sizeof(cd_passwd.newpasswd));
      strncpy(cd_passwd.newpasswd, argv[2], sizeof(cd_passwd.newpasswd));
      cd_passwd.timeout = -1;
      memcpy(d, &cd_hdr, sizeof(cd_hdr));
      memcpy(d + sizeof(cd_hdr), &cd_passwd, sizeof(cd_passwd));
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Sending password change command...\n", (unsigned) time(NULL));
#endif
      r = send(s, d, sizeof(cd_hdr) + sizeof(cd_passwd), 0);
      if (r < 0)
      {
        perror("drcomc: send");
        r = EXIT_FAILURE;
        break;
      }
      /* Receiving part */
#ifdef DRCOM_DEBUG
      fprintf(stderr, "%10u DEBUG drcomc: Receiving reply (password change)...\n", (unsigned) time(NULL));
#endif
      r = recv(s, e, DRCOMCD_BUF_LEN, 0);
      if (r < 0)
        perror("daemon: recv");
      if (r < 1 || ((struct drcomcd_hdr *) e)->signature != DRCOM_SIGNATURE)
      {
#ifdef DRCOM_DEBUG
        fprintf(stderr, "%10u DEBUG drcomc: r=%d signature=0x%08x\n", (unsigned) time(NULL), r, ((struct drcomcd_hdr *) e)->signature);
#endif
        printf("drcomc: Unknown signature\n");
        r = EXIT_FAILURE;
        break;
      }
      r = print_result(e, "Password change");
      break;
  }

  close(s);

  return r;
}

void usage()
{
  puts("drcomc, client part of the drcomc-drcomcd client-daemon programs\n\n"
       "  usage: drcomc ( login | logout | passwd \"newpasswd\" )\n\n"
       "  login                tell drcomcd to login\n"
       "  logout               tell drcomcd to logout\n"
       "  passwd \"newpasswd\"   tell drcomcd to change "
         "password to \"newpasswd\"\n\n"
       "  At least one option must be specified.\n");

  exit(EXIT_FAILURE);

  return;
}

int print_result(const unsigned char *e, const char *s_mode)
{
  struct drcomcd_hdr *dc_hdr = (struct drcomcd_hdr *) e;
  struct drcomcd_result *dc_result = (struct drcomcd_result *) (e + sizeof(*dc_hdr));

  if (dc_hdr->type == DRCOMCD_SUCCESS)
  {
    printf("%s succeeded\n", s_mode);
    return EXIT_SUCCESS;
  }
  else
  {
    printf("%s failed\n", s_mode);
    printf("Reason: %d\n", dc_result->reason);
    return EXIT_FAILURE;
  }
}

