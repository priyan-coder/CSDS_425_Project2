#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "stdbool.h"

#define ERROR 1
#define REQUIRED_ARGC 5
#define HOST_POS 1
#define PORT_POS 2
#define PROTOCOL "tcp"
#define BUFLEN 1024

int usage(char *progname) {
    fprintf(stderr, "usage: %s -u URL [-i] [-c] [-s] -o filename\n", progname);
    exit(ERROR);
}

int errexit(char *format, char *arg) {
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    char buffer[BUFLEN];
    int sd, ret, opt;
    bool FILENAME_PARSED = false;
    bool URL_PARSED = false;
    char *url = NULL;

    if (argc < REQUIRED_ARGC)
        usage(argv[0]);

    // for (int i = 1; i < argc; i++) {
    //     printf("%s\n", argv[i]);
    // }

    while (optind < argc) {
        if ((opt = getopt(argc, argv, ":u:o:")) != -1) {
            switch (opt) {
                case 'o':
                    FILENAME_PARSED = true;
                    printf("filename %s\n", optarg);
                    break;
                case 'u':
                    URL_PARSED = true;
                    // printf("%lu", strlen(optarg));
                    url = optarg;
                    printf("url: %s\n", optarg);
                    break;
                case '?':
                    printf("Unknown option: %c\n", optopt);
                    break;
                case ':':
                    printf("Missing arg for %c\n", optopt);
                    usage(argv[0]);
            }
        } else {
            optind += 1;
        }
    }

    if (!FILENAME_PARSED || !URL_PARSED) {
        printf("Mandatory args missing!\n");
        usage(argv[0]);
    }

    char *endpoint = malloc(strlen(url) * sizeof(char));
    strcpy(endpoint, url);
    for (int i = 0; i < strlen(url); i++) {
        endpoint[i] = tolower(endpoint[i]);
    }
    // printf("%s\n", endpoint);

    // printf("%s\n", token);
    char *info[2];
    // int result = strcmp(token, "http");
    // printf("%d\n", result);
    // if (result != 0) {
    //     printf("Only Http is supported\n");
    //     exit(1);
    // }
    int i = 0;
    char *token = strtok(endpoint, "/");
    while (token != NULL && i < 2) {
        // printf("%s\n", token);
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }
    // printf("%s\n", endpoint);
    // for (int i = 0; i < strlen(url); i++) {
    //     printf("%c\n", endpoint[i]);
    // }
    int j = 0;
    while (j <= 2) {
        printf("%s\n", info[j]);
        j++;
    }

    /* Get all of the non-option arguments */
    // if (optind < argc) {
    //     printf("Non-option args: ");
    //     while (optind < argc)
    //         printf("%s ", argv[optind++]);
    //     printf("\n");
    // }

    // /* lookup the hostname */
    // hinfo = gethostbyname(argv[HOST_POS]);
    // if (hinfo == NULL)
    //     errexit("cannot find name: %s", argv[HOST_POS]);

    // /* set endpoint information */
    // memset((char *)&sin, 0x0, sizeof(sin));
    // sin.sin_family = AF_INET;
    // sin.sin_port = htons(atoi(argv[PORT_POS]));
    // memcpy((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    // if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
    //     errexit("cannot find protocol information for %s", PROTOCOL);

    // /* allocate a socket */
    // /*   would be SOCK_DGRAM for UDP */
    // sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    // if (sd < 0)
    //     errexit("cannot create socket", NULL);

    // /* connect the socket */
    // if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    //     errexit("cannot connect", NULL);

    // /* snarf whatever server provides and print it */
    // memset(buffer, 0x0, BUFLEN);
    // ret = read(sd, buffer, BUFLEN - 1);
    // if (ret < 0)
    //     errexit("reading error", NULL);
    // fprintf(stdout, "%s\n", buffer);

    // /* close & exit */
    // close(sd);
    // exit(0);
}
