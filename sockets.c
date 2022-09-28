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
#define MEM_ERR "Unable to realloc memory"
#define PORT 80
#define PROTOCOL "tcp"
#define BUFFER_SIZE 1024
#define REQ_TYPE "GET "
#define HTTP_VERSION " HTTP/1.0\r\n"
#define HOST "Host: "
#define CARRIAGE "\r\n"
#define CLIENT "User-Agent: CWRU CSDS 325 Client 1.0\r\n"

/* Incorrect Syntax Routine */
int usage(char *progname) {
    fprintf(stderr, "usage: %s -u URL [-i] [-c] [-s] -o filename\n", progname);
    exit(ERROR);
}
/* Terminates execution */
int errexit(char *format, char *arg) {
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}
/* Grabs hostname and web_filename from a given url. Returns 1 or -1 to indicate success or failure */
int get_hostname_and_web_filename(char *url, char **hostname, char **web_filename) {
    /* Convert to lowercase */
    for (int i = 0; i < strlen(url); i++) {
        url[i] = tolower(url[i]);  // each elem is a character
    }

    /* Creating a copy of the url for future web_filename re-creation */
    char *copy_url = malloc(strlen(url) + 1);
    strcpy(copy_url, url);

    /* Check for Http only */
    char *token = strtok(url, "/");
    if ((strcmp(token, "http:")) != 0) {
        printf(HTTP_VERSION_ERR);
        free(copy_url);
        exit(ERROR);
    }

    /* Tokenize the URL */
    char **info = malloc(sizeof(char *) * BUFFER_SIZE);
    int i = 0;  // indicates the num of elems or tokens in info arr
    while (token != NULL) {
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }

    // hostname of the server, info[0] is http:
    *hostname = realloc(*hostname, strlen(info[1]));
    *web_filename = realloc(*web_filename, (2 * i));
    if (*hostname == NULL)
        return -1;
    if (*web_filename == NULL)
        return -1;

    strncpy(*hostname, info[1], strlen(info[1]));

    if (i == 2) {
        strncat(*web_filename, "/", strlen("/"));
    } else {
        for (int j = 2; j < i; j++) {
            strncat(*web_filename, "/", strlen("/"));
            strncat(*web_filename, info[j], strlen(info[j]));
        }
        if (strcmp(&copy_url[strlen(copy_url) - 1], "/") == 0) {
            // last char is a '/'
            strncat(*web_filename, "/", strlen("/"));
        }
    }
    free(info);
    free(copy_url);
    return 1;
}
/* Prints info if -i is specified on the cmd line */
void print_info(char *hostname, char *web_filename, char *ouput_file) {
    printf("INF: hostname = %s\n", hostname);
    printf("INF: web_filename = %s\n", web_filename);
    printf("INF: output_filename = %s\n", ouput_file);
}
/* Prints GET request if -c is specified on the cmd line */
void print_request(char *request) {
    char *copy_req = malloc(strlen(request) + 1);
    memset(copy_req, 0, strlen(request) + 1);
    memcpy(copy_req, request, strlen(request));
    char *line;
    line = strtok(copy_req, "\r\n");
    while (line != NULL) {
        printf("REQ: %s\n", line);
        line = strtok(NULL, "\r\n");
    }
    free(copy_req);
}
/* Creates GET request and calls print handlers for info and req if conditions are met */
void generate_req(char **hostname, char **web_filename, char *request, char *output_filename, bool info_print, bool req_print) {
    memset(request, 0, strlen(request));
    strncat(request, REQ_TYPE, strlen(REQ_TYPE));
    strncat(request, *web_filename, strlen(*web_filename));
    strncat(request, HTTP_VERSION, strlen(HTTP_VERSION));
    strncat(request, HOST, strlen(HOST));
    strncat(request, *hostname, strlen(*hostname));
    strncat(request, CARRIAGE, strlen(CARRIAGE));
    strncat(request, CLIENT, strlen(CLIENT));
    strncat(request, CARRIAGE, strlen(CARRIAGE));
    /* if -i is specified on the cmd line, we print INF */
    if (info_print)
        print_info(*hostname, *web_filename, output_filename);
    /* If -c is specified on the cmd line, PRINT_REQ is set to true and we print REQ */
    if (req_print)
        print_request(request);
}
/* Creates socket, connects and sends GET request to server. Returns socket file descriptor */
int create_socket_and_send_request(char *hostname, char *request_to_server) {
    int sd;
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    /* Lookup the hostname */
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errexit("cannot find name: %s", hostname);

    /* Set endpoint information */
    memset((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    memcpy((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("Cannot find protocol information for %s", PROTOCOL);

    /* Allocate a socket */
    /* Would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("Cannot create socket", NULL);

    /* Connect the socket */
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("Cannot connect!", NULL);

    int sent = send(sd, request_to_server, strlen(request_to_server), 0);
    if (sent < 0) {
        printf("Cannot send GET request!\n");
        exit(ERROR);
    }

    return sd;
}

int main(int argc, char *argv[]) {
    int opt;  // option
    char *HOSTNAME = NULL;
    char *WEB_FILENAME = NULL;
    char *url = NULL;              // to save the entire url which user enters on cmd line
    char *OUTPUT_FILENAME = NULL;  // write to this file in local system
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
        //  To disable the automatic error printing, a colon is added as the first character in optstring:
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

    /* Parse_url, generate hostname, web_filename and GET req. Send GET req */
    int host_file_status = get_hostname_and_web_filename(url, &HOSTNAME, &WEB_FILENAME);
    if (host_file_status < 0)
        printf(MEM_ERR);
    int length_needed = strlen(REQ_TYPE) + strlen(WEB_FILENAME) + strlen(HTTP_VERSION) + strlen(HOST) + strlen(HOSTNAME) + strlen(CARRIAGE) + strlen(CLIENT) + strlen(CARRIAGE);
    char REQUEST[length_needed + 1];
    generate_req(&HOSTNAME, &WEB_FILENAME, REQUEST, OUTPUT_FILENAME, PRINT_INFO, PRINT_REQ);  // printing logic is also handled in here
    int sd = create_socket_and_send_request(HOSTNAME, REQUEST);

    /* Grab server response */
    char *buffer = malloc(BUFFER_SIZE);
    memset(buffer, 0x0, BUFFER_SIZE);
    FILE *fp = fdopen(sd, "r");
    if (fp == NULL) {
        printf("Unable to create a file descriptor to read socket\n");
        exit(ERROR);
    }
    /* Prints header of server response if -s is specified on the cmd line. Reads header only */
    while (strcmp(fgets(buffer, BUFFER_SIZE, fp), CARRIAGE) != 0) {
        // if not 200 OK yet
        if (!SERVER_STATUS) {
            if (strstr(buffer, "200 OK") != NULL) {
                SERVER_STATUS = true;
            }
        }
        if (PRINT_RES) {
            printf("RSP: %s", buffer);
        }
    }
    /* Read data */
    if (SERVER_STATUS) {
        FILE *stream = fopen(OUTPUT_FILENAME, "w+");
        while (fread(buffer, BUFFER_SIZE, sizeof(char), fp)) {
            if (fp != NULL) {
                printf("%s", buffer);
                fputs(buffer, stream);
                memset(buffer, 0, BUFFER_SIZE);
            }
        }
        fclose(stream);
    }

    if (!SERVER_STATUS)
        printf(RESPONSE_CODE_ERR);

    /* close & exit */
    free(WEB_FILENAME);
    free(HOSTNAME);
    free(buffer);
    fclose(fp);
    close(sd);
    exit(0);
}
