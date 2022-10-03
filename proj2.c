/* Name: BALASHANMUGA PRIYAN RAJAMOHAN
 * Case ID: bxr261
 * Filename: proj2.c
 * Date created: 3 Oct 2022
 * Description: The following code is used to read HTTP response from a server and write received data to a output file.
 */

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
#define IO_ERR "Unable to create a file descriptor to read socket\n"
#define MEM_ERR "Unable to realloc memory"
#define PORT 80
#define PROTOCOL "tcp"
#define BUFFER_SIZE 1024
#define REQ_TYPE "GET "
#define HTTP_VERSION " HTTP/1.0\r\n"
#define HOST "Host: "
#define CARRIAGE "\r\n"
#define CLIENT "User-Agent: CWRU CSDS 325 Client 1.0\r\n"
#define STATUS_OK "200 OK"
#define MOVED "301 Moved"

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
    /* Creating a copy of the url for future web_filename re-creation */
    // char *copy_url = malloc(strlen(url) + 1);
    char copy_url[BUFFER_SIZE];
    strcpy(copy_url, url);

    /* Check for Http only */
    char *token = strtok(url, "/");
    if ((strcmp(token, "http:")) != 0) {
        printf(HTTP_VERSION_ERR);
        // free(copy_url);
        exit(ERROR);
    }

    /* Tokenize the URL */
    // char **info = malloc(sizeof(char *) * BUFFER_SIZE);
    char *info[BUFFER_SIZE];
    int i = 0;  // indicates the num of elems or tokens in info arr
    while (token != NULL) {
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }

    // hostname of the server, info[0] is http:
    *hostname = realloc(*hostname, strlen(info[1]) + 1);
    *web_filename = realloc(*web_filename, (2 * i));
    if (*hostname == NULL)
        return -1;
    if (*web_filename == NULL)
        return -1;

    strncpy(*hostname, info[1], strlen(info[1]));

    if (i == 2) {
        strcat(*web_filename, "/");
    } else {
        for (int j = 2; j < i; j++) {
            strcat(*web_filename, "/");
            strcat(*web_filename, info[j]);
        }
        if (strcmp(&copy_url[strlen(copy_url) - 1], "/") == 0) {
            // last char is a '/'
            strcat(*web_filename, "/");
        }
    }
    // free(info);
    // free(copy_url);
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
    strcat(request, REQ_TYPE);
    strcat(request, *web_filename);
    strcat(request, HTTP_VERSION);
    strcat(request, HOST);
    strcat(request, *hostname);
    strcat(request, CARRIAGE);
    strcat(request, CLIENT);
    strcat(request, CARRIAGE);
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

/* Reads response header, prints if -s is specified on the command line. Returns status_200 and also updates status_301 */
bool read_resp_header(FILE *fp, bool print_res, bool *status_301, char *URL, char **hostname, char **web_filename) {
    /* Prints header of server response if -s is specified on the cmd line. Reads header only */
    bool status_code_check_done = false;
    bool status_200 = false;
    // char *buffer = malloc(BUFFER_SIZE);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0x0, BUFFER_SIZE);
    while (strcmp(fgets(buffer, BUFFER_SIZE, fp), CARRIAGE) != 0) {
        if (print_res)
            printf("RSP: %s", buffer);
        if (!status_code_check_done) {
            if (strstr(buffer, STATUS_OK) != NULL) {
                status_200 = true;
                *status_301 = false;
            }
            if (strstr(buffer, MOVED) != NULL) {
                *status_301 = true;
            }
            status_code_check_done = true;
        }
        if (*status_301 && (strstr(buffer, "Location") != NULL)) {
            buffer[strcspn(buffer, "\r\n")] = 0;
            // *URL = realloc(*URL, strlen(strstr(buffer, " ") + 1) + 1);
            memset(URL, 0x0, BUFFER_SIZE);
            strncpy(URL, strstr(buffer, " ") + 1, strlen(strstr(buffer, " ") + 1));
            *hostname = NULL;
            *web_filename = NULL;
        }
    }
    // free(buffer);
    return status_200;
}
/* Writes data to the output_filename */
void write_data_to_file(char *output_filename, FILE *fp) {
    FILE *stream = fopen(output_filename, "w+");
    if (stream == NULL) {
        printf(IO_ERR);
        exit(ERROR);
    }
    int N = 0;
    // char *buffer = malloc(BUFFER_SIZE);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0x0, BUFFER_SIZE);
    while (!feof(fp)) {
        N = fread(buffer, 1, BUFFER_SIZE * sizeof(char), fp);
        // printf("%d\n", N);
        // printf("%s", buffer);
        fwrite(buffer, sizeof(buffer[0]), N * sizeof(buffer[0]), stream);
        memset(buffer, 0x0, BUFFER_SIZE);
    }
    fclose(stream);
    // free(buffer);
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
    bool ENABLE_REDIRECT = false;  // flag to check if user entered -f on the cmd line
    bool FILENAME_PARSED = false;  // flag to check if user entered filename argument after -o
    bool URL_PARSED = false;       // flag to check if user entered URL argument after -u
    /* Check for minimum num of arguments */
    if (argc < REQUIRED_ARGC) {
        usage(argv[0]);
    }

    while (optind < argc) {
        //  To disable the automatic error printing, a colon is added as the first character in optstring:
        if ((opt = getopt(argc, argv, ":u:o:icsf")) != -1) {
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
                case 'f':
                    ENABLE_REDIRECT = true;
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

    // char *URL = malloc(strlen(url) + 1);
    char URL[BUFFER_SIZE];
    memset(URL, 0x0, BUFFER_SIZE);
    strncpy(URL, url, strlen(url));
    while (true) {
        bool STATUS_301 = false;  // check if received 301 resp from server
        /* Parse_url, generate hostname, web_filename and GET req. Send GET req */
        int host_file_status = get_hostname_and_web_filename(URL, &HOSTNAME, &WEB_FILENAME);
        if (host_file_status < 0)
            printf(MEM_ERR);

        int length_needed = strlen(REQ_TYPE) + strlen(WEB_FILENAME) + strlen(HTTP_VERSION) + strlen(HOST) + strlen(HOSTNAME) + strlen(CARRIAGE) + strlen(CLIENT) + strlen(CARRIAGE);
        char REQUEST[length_needed + 1];
        memset(REQUEST, 0, length_needed + 1);
        generate_req(&HOSTNAME, &WEB_FILENAME, REQUEST, OUTPUT_FILENAME, PRINT_INFO, PRINT_REQ);  // printing logic is also handled in here
        int sd = create_socket_and_send_request(HOSTNAME, REQUEST);

        /* Create buffer and ensure file pointer is created to read socket */
        FILE *fp = fdopen(sd, "r");
        if (fp == NULL) {
            printf(IO_ERR);
            exit(ERROR);
        }
        bool S_200 = read_resp_header(fp, PRINT_RES, &STATUS_301, URL, &HOSTNAME, &WEB_FILENAME);
        if (S_200) {
            write_data_to_file(OUTPUT_FILENAME, fp);
            ENABLE_REDIRECT = false;
        } else {
            if (!ENABLE_REDIRECT) {
                printf(RESPONSE_CODE_ERR);
            }
            if (!(STATUS_301 && ENABLE_REDIRECT)) {
                exit(ERROR);
            }
        }

        fclose(fp);
        close(sd);
        if (!ENABLE_REDIRECT)
            break;
    }
    free(WEB_FILENAME);
    free(HOSTNAME);
    // free(URL);
    exit(0);
}
