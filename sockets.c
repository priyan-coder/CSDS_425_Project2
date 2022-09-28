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
#define MEM_ERR "Unable to realloc memory for: "
#define PORT_POS 80
#define PROTOCOL "tcp"
#define BUFFER_SIZE 1024
#define REQ_TYPE "GET "
#define HTTP_VERSION " HTTP/1.0\r\n"
#define HOST "Host: "
#define CARRIAGE "\r\n"
#define CLIENT "User-Agent: CWRU CSDS 325 Client 1.0\r\n"

// int storeWebData(char *filepath, char *data) {
//     int r = 0;
//     FILE *fp = fopen(filepath, "w+");
//     char *body = strstr(data, "\r\n\r\n");
//     if (fp != NULL) {
//         // printf("%s\n", body);
//         if (fputs(body + 4, fp) != EOF) {
//             r = 1;
//         }
//         fclose(fp);
//     }
//     return r;
// }

/* Handler for incorrect syntax routine */
int usage(char *progname) {
    fprintf(stderr, "usage: %s -u URL [-i] [-c] [-s] -o filename\n", progname);
    exit(ERROR);
}

int errexit(char *format, char *arg) {
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}

int get_hostname_and_web_filename(char *url, char **hostname, char **web_filename) {
    /* Convert to lowercase */
    for (int i = 0; i < strlen(url); i++) {
        url[i] = tolower(url[i]);  // each elem is a character
    }

    /* creating a copy of the url for future web_filename re-creation */
    char *copy_url = malloc(strlen(url) + 1);
    strcpy(copy_url, url);

    /* Check for Http only */
    char *token = strtok(url, "/");
    if ((strcmp(token, "http:")) != 0) {
        printf(HTTP_VERSION_ERR);
        free(copy_url);
        exit(ERROR);
    }

    /* Tokenize the URL*/
    char **info = malloc(sizeof(char *) * BUFFER_SIZE);
    int i = 0;  // indicates the num of elems or tokens in info arr
    while (token != NULL) {
        info[i] = token;
        token = strtok(NULL, "/");
        i++;
    }

    // char *tmp = realloc(dest->hostname, strlen(info[1]) + 1);  // hostname of the server, info[0] is http:
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
            // last char is a /
            strncat(*web_filename, "/", strlen("/"));
        }
    }
    free(info);
    free(copy_url);
    return 1;
}

void generate_req(char **hostname, char **web_filename, char *request) {
    memset(request, 0, strlen(request));
    strncat(request, REQ_TYPE, strlen(REQ_TYPE));
    strncat(request, *web_filename, strlen(*web_filename));
    strncat(request, HTTP_VERSION, strlen(HTTP_VERSION));
    strncat(request, HOST, strlen(HOST));
    strncat(request, *hostname, strlen(*hostname));
    strncat(request, CARRIAGE, strlen(CARRIAGE));
    strncat(request, CLIENT, strlen(CLIENT));
    strncat(request, CARRIAGE, strlen(CARRIAGE));
}
void print_info(char *hostname, char *web_filename, char *ouput_file) {
    printf("INF: hostname = %s\n", hostname);
    printf("INF: web_filename = %s\n", web_filename);
    printf("INF: output_filename = %s\n", ouput_file);
}

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
int create_socket_and_send_request(char *host_name, char *request_to_server) {
    int sd;
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *protoinfo;
    /* lookup the hostname */
    hinfo = gethostbyname(host_name);
    if (hinfo == NULL)
        errexit("cannot find name: %s", host_name);

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

    int sent = send(sd, request_to_server, strlen(request_to_server), 0);
    if (sent < 0) {
        printf("cannot send GET request");
    }

    return sd;
}

int main(int argc, char *argv[]) {
    int opt;
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
    /* parse_url, generate hostname, web_filename and GET req */
    int host_file_status = get_hostname_and_web_filename(url, &HOSTNAME, &WEB_FILENAME);
    if (host_file_status < 0) {
        printf(MEM_ERR);
    }
    int length_needed = strlen(REQ_TYPE) + strlen(WEB_FILENAME) + strlen(HTTP_VERSION) + strlen(HOST) + strlen(HOSTNAME) + strlen(CARRIAGE) + strlen(CLIENT) + strlen(CARRIAGE);
    char REQUEST[length_needed + 1];
    generate_req(&HOSTNAME, &WEB_FILENAME, REQUEST);
    /* if -i is specified in cmd line, we print INF */
    if (PRINT_INFO)
        print_info(HOSTNAME, WEB_FILENAME, OUTPUT_FILENAME);
    /* If user specified -c on the command line, PRINT_REQ is set to true and the following will be executed to print the request*/
    if (PRINT_REQ)
        print_request(REQUEST);

    /* snarf whatever server provides */
    // char *buffer = malloc(BUFFER_SIZE);
    // memset(buffer, 0x0, BUFFER_SIZE);
    // FILE *fp;
    // fp = fdopen(sd, "r");
    // if (fp == NULL) {
    //     printf("Buggy fp\n");
    // }

    // while (fgets(buffer, BUFFER_SIZE, fp)) {
    //  printf("ret value: %d\n", ret);

    // if (fp != NULL) {
    //     // printf("%s\n", body);
    //     if (fputs(buffer, fp) != EOF) {
    //     }
    // }

    // fclose(fp);
    // n += ret;
    // new_size += BUFFER_SIZE;
    // char *temp = realloc(buffer, new_size);
    // if (temp != NULL) {
    //     buffer = temp;
    // } else {
    //     printf("cannot realloc buffer memory while reading data\n");
    //     exit(1);
    // }
    //}

    // printf("Last character in buffer: %c\n", buffer[strlen(buffer) - 1]);
    // printf("sizes\nn: %d\nbuffer_size: %lu\nbuffer_length: %lu\n", n, sizeof(buffer), strlen(buffer));

    // if (ret < 0) {
    //     errexit("reading error", NULL);
    // }

    // fprintf(stdout, "%s\n", buffer);

    /* Check if status 200 OK is in buffer and print RESP if -s present on cmd line */
    // char *ptr_to_buffer = buffer;
    // char *points_to_start_of_body = strstr(buffer, "\r\n\r\n");
    // char *header_copy = malloc((points_to_start_of_body - ptr_to_buffer + 1) * sizeof(char));
    // memset(header_copy, 0x0, (points_to_start_of_body - ptr_to_buffer + 1));
    // memcpy(header_copy, buffer, (points_to_start_of_body - ptr_to_buffer));
    // // printf("Length of header: %lu\n", strlen(header_copy));
    // char *each_header_token = strtok(header_copy, "\r\n");
    // if (strstr(each_header_token, "200 OK") != NULL) {
    //     SERVER_STATUS = true;
    // }
    // if (PRINT_RES) {
    //     while (each_header_token != NULL) {
    //         printf("RSP: %s\n", each_header_token);
    //         each_header_token = strtok(NULL, "\r\n");
    //     }
    // }

    // if (!SERVER_STATUS)
    //     printf(RESPONSE_CODE_ERR);
    // } else {
    //     // handle -o and write body of server response to file
    //     int res = storeWebData(OUTPUT_FILENAME, buffer);
    //     if (res == 0) {
    //         printf(IO_ERR);
    //         exit(1);
    //     }
    // }

    /* close & exit */
    free(WEB_FILENAME);
    free(HOSTNAME);
    // free(buffer);
    // free(header_copy);
    // fclose(fp);
    // close(sd);
    exit(0);
}
