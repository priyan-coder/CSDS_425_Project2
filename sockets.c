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
#define MANDATORY_ERR "Mandatory args missing!\n"
#define HTTP_VERSION_ERR "Only Http is supported\n"
#define INCORRECT_URL "Please enter a valid URL!\n"
#define RESPONSE_CODE_ERR "ERROR: non-200 response code\n"
#define IO_ERR "Input Output Error while writing to a file!\n"
#define PORT_POS 80
#define PROTOCOL "tcp"
#define BUFFER_SIZE 1024
#define REQ_TYPE "GET "
#define HTTP_VERSION " HTTP/1.0\r\n"
#define SENDER "Host: "
#define CARRIAGE "\r\n"
#define CLIENT "User-Agent: CWRU CSDS 325 Client 1.0\r\n"

int storeWebData(char *filepath, char *data) {
    int r = 0;
    FILE *fp = fopen(filepath, "w+");
    char *body = strstr(data, "\r\n\r\n");
    if (fp != NULL) {
        // printf("%s\n", body);
        if (fputs(body + 4, fp) != EOF) {
            r = 1;
        }
        fclose(fp);
    }
    return r;
}
/* Incorrect syntax routine */
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
    int sd, ret, opt;
    char *url = NULL;              // to save the entire url which user enters on cmd line
    char *OUTPUT_FILENAME = NULL;  // write to file in local system
    bool PRINT_INFO = false;       // flag to check if user entered -i on the cmd line
    bool PRINT_REQ = false;        // flag to check if user entered -c on the cmd line
    bool PRINT_RES = false;        // flag to check if user entered -s on the cmd line
    bool FILENAME_PARSED = false;  // flag to check if user entered filename argument after -o
    bool URL_PARSED = false;       // flag to check if user entered URL argument after -u
    bool SERVER_STATUS = false;    // check if recv 200 OK resp from server

    /* Check for minimum num of arguments */
    if (argc < REQUIRED_ARGC) {
        usage(argv[0]);
    }

    while (optind < argc) {
        //  To disable the automatic error printing, simply put a colon as the first character in optstring:
        if ((opt = getopt(argc, argv, ":u:o:ics")) != -1) {
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
                case 'c':
                    PRINT_REQ = true;
                    break;
                case 's':
                    PRINT_RES = true;
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

    /* Mandatory arguments check */
    if (!FILENAME_PARSED || !URL_PARSED) {
        printf(MANDATORY_ERR);
        usage(argv[0]);
    }

    /* Convert to lowercase */
    for (int i = 0; i < strlen(url); i++) {
        url[i] = tolower(url[i]);  // each elem is a character
    }

    /* Check for Http only */
    char *token = strtok(url, "/");
    if ((strcmp(token, "http:")) != 0) {
        printf(HTTP_VERSION_ERR);
        exit(1);
    }

    /* Tokenize the URL*/
    char **info = malloc(sizeof(char *) * BUFFER_SIZE);
    int i = 0;
    while (token != NULL) {
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }

    // i indicates the number of elements or tokens in the info arr
    // A url needs to have http and hostname as a minimum
    if (i < 2) {
        printf(INCORRECT_URL);
        usage(argv[0]);
    }

    /* Set HOSTNAME and WEB_FILENAME */
    char *HOSTNAME = malloc(strlen(info[1]) + 1);  // hostname of the server, info[0] is http:
    char *WEB_FILENAME = malloc(2 * i);            // path to file in web server
    memcpy(HOSTNAME, info[1], strlen(info[1]));
    if (i == 2) {
        strcat(WEB_FILENAME, "/");
    } else {
        for (int j = 2; j < i; j++) {
            strcat(WEB_FILENAME, "/");
            strcat(WEB_FILENAME, info[j]);
        }
    }

    /* if -i is specified in cmd line, we print INF */
    if (PRINT_INFO) {
        printf("INF: hostname = %s\n", HOSTNAME);
        printf("INF: web_filename = %s\n", WEB_FILENAME);
        printf("INF: output_filename = %s\n", OUTPUT_FILENAME);
    }

    // need to refactor from here in order to implement f
    // while (1) {
    /* Initialising a GET request to the hostname and web_filename specified by user */
    int length_needed = strlen(REQ_TYPE) + strlen(WEB_FILENAME) + strlen(HTTP_VERSION) + strlen(SENDER) + strlen(HOSTNAME) + strlen(CARRIAGE) + strlen(CLIENT) + strlen(CARRIAGE);
    char *REQUEST = malloc(length_needed + 1);
    strcat(REQUEST, REQ_TYPE);
    strcat(REQUEST, WEB_FILENAME);
    strcat(REQUEST, HTTP_VERSION);
    strcat(REQUEST, SENDER);
    strcat(REQUEST, HOSTNAME);
    strcat(REQUEST, CARRIAGE);
    strcat(REQUEST, CLIENT);
    strcat(REQUEST, CARRIAGE);

    /* If user specified -c on the command line, PRINT_REQ is set to true and the following will be executed to print the request*/
    if (PRINT_REQ) {
        char req_copy[length_needed + 1];
        strcpy(req_copy, REQUEST);
        char *line;
        line = strtok(req_copy, "\r\n");
        while (line != NULL) {
            printf("REQ: %s\n", line);
            line = strtok(NULL, "\r\n");
        }
    }

    /* lookup the hostname */
    hinfo = gethostbyname(HOSTNAME);
    if (hinfo == NULL)
        errexit("cannot find name: %s", HOSTNAME);

    /* set endpoint information */
    memset((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT_POS);
    memcpy((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("cannot find protocol information for %s", PROTOCOL);

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* connect the socket */
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("cannot connect", NULL);

    int sent = send(sd, REQUEST, strlen(REQUEST), 0);
    if (sent < 0) {
        printf("cannot send GET request");
    }

    /* snarf whatever server provides */
    char *buffer = malloc(BUFFER_SIZE * sizeof(char));
    memset(buffer, 0x0, BUFFER_SIZE * sizeof(char));
    int n = 0;
    while ((ret = read(sd, buffer + n, BUFFER_SIZE - 1)) > 0) {
        n += ret;
        buffer = (char *)realloc(buffer, BUFFER_SIZE + BUFFER_SIZE);
        if (buffer == NULL) {
            printf("cannot realloc buffer memory\n");
        }
    }
    buffer[n] = '\0';
    printf("%s\n", buffer);

    if (ret < 0)
        errexit("reading error", NULL);

    // fprintf(stdout, "%s\n", buffer);

    /* Check if status 200 OK is in buffer and print RESP if -s present on cmd line*/
    char *copy_of_buffer = malloc(n * sizeof(char));
    memset(copy_of_buffer, 0x0, n);
    memcpy(copy_of_buffer, buffer, n);
    char *headerrow = strtok(copy_of_buffer, "\r\n");
    // while (strstr(headerrow, "301") != NULL) {

    // }

    if (strstr(headerrow, "200 OK") != NULL) {
        SERVER_STATUS = true;
    }

    // if (PRINT_RES) {
    //     while (headerrow != NULL) {
    //         printf("RSP: %s\n", headerrow);
    //         headerrow = strtok(NULL, "\r\n");
    //         if ((strstr(headerrow, "doctype") != NULL) || (strstr(headerrow, "DOCTYPE")) != NULL) {
    //             break;
    //         }
    //     }
    // }

    if (!SERVER_STATUS) {
        printf(RESPONSE_CODE_ERR);

    } else {
        // handle -o and write body of server response to file
        int res = storeWebData(OUTPUT_FILENAME, buffer);
        if (res == 0) {
            printf(IO_ERR);
            exit(1);
        }
    }
    // }

    /* close & exit */
    free(REQUEST);
    free(WEB_FILENAME);
    free(HOSTNAME);
    free(info);
    free(buffer);
    free(copy_of_buffer);
    close(sd);
    exit(0);
}
