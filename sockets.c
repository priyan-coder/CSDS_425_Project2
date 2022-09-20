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
#define MAXSIZE 1024

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
    bool FILENAME_PARSED = false;  // flag to check if user entered filename argument after -o
    bool URL_PARSED = false;       // flag to check if user entered URL argument after -u
    char *url = NULL;              // to save the entire url which user enters on cmd line
    char *HOSTNAME = NULL;         // hostname of the server
    char *WEB_FILENAME = NULL;     // path to file in web server
    char *OUTPUT_FILENAME = NULL;  // write to file in local system
    bool PRINT_INFO = false;       // flag to check if user entered -i on the cmd line

    if (argc < REQUIRED_ARGC) {
        usage(argv[0]);
    }

    while (optind < argc) {
        //  To disable the automatic error printing, simply put a colon as the first character in optstring:
        if ((opt = getopt(argc, argv, ":u:o:i")) != -1) {
            switch (opt) {
                case 'o':
                    FILENAME_PARSED = true;
                    OUTPUT_FILENAME = optarg;
                    break;
                case 'u':
                    URL_PARSED = true;
                    url = optarg;
                    break;
                case 'i':
                    PRINT_INFO = true;
                    break;
                case '?':
                    printf("Unknown option: %c\n", optopt);
                    break;
                case ':':
                    printf("Missing arg for %c\n", optopt);
                    usage(argv[0]);
                    break;
            }
        } else {
            optind += 1;
        }
    }

    if (!FILENAME_PARSED || !URL_PARSED) {
        printf("Mandatory args missing!\n");
        usage(argv[0]);
    }

    /* Convert to lowercase */
    char *endpoint = malloc((sizeof(url) + 1) * sizeof(char));
    strcpy(endpoint, url);
    for (int i = 0; i < strlen(url); i++) {
        endpoint[i] = tolower(endpoint[i]);  // each elem is a character
    }

    /* Check for Http only */
    char *token = strtok(endpoint, "/");
    if ((strcmp(token, "http:")) != 0) {
        printf("Only Http is supported\n");
        exit(1);
    }

    /* Tokenize the URL*/
    char *info[MAXSIZE];
    int i = 0;
    while (token != NULL) {
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }

    // i indicates the number of elements or tokens in the info arr
    // A url needs to have http and hostname as a minimum
    if (i < 2) {
        printf("Please enter a valid URL!\n");
        usage(argv[0]);
    }

    /* Set HOSTNAME and WEB_FILENAME */
    HOSTNAME = info[1];  // info[0] contains http:
    char temp[MAXSIZE];
    strcat(temp, "/");
    for (int j = 2; j < i; j++) {
        strcat(temp, info[j]);
        strcat(temp, "/");
    }
    WEB_FILENAME = temp;

    /* if -i is specified in cmd line, we print INF */
    if (PRINT_INFO) {
        printf("INF: hostname = %s\n", HOSTNAME);
        printf("INF: web_filename = %s\n", WEB_FILENAME);
        printf("INF: output_filename = %s\n", OUTPUT_FILENAME);
    }

    // -----
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
