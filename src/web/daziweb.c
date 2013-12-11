#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <fcntl.h>
#include "mdazibao.h"
#include "daziweb.h"
#include "utils.h"
#include "webutils.h"
#include "request.h"
#include "routing.h"
#include "routes.h"

/** @file
 * Main file for the Web server
 **/

/** the socket used by the server to listen for new clients */
static int listening_sock = -1;
/** the structure used to store the current request */
static struct http_request *req;
/** the current dazibao */
static dz_t dz;

/** initialize WSERVER */
static void init_wserver_infos(void);
/** free the memory used by WSERVER */
static void destroy_wserver_infos(void);

/**
 * Handler for signals used to close the listening socket.
 **/
static void clean_close(int s) {
        /* avoid 'unused parameter' warning */ s++;
        if (listening_sock >= 0 && close(listening_sock) == -1) {
            perror("close");
        }
        if (dz.fd > 0) {
                dz_close(&dz);
        }
        destroy_wserver_infos();
        destroy_http_request(req);
        destroy_routes();
        exit(EXIT_SUCCESS);
}


/**
 * parse command-line arguments, and fill relevant fields in WSERVER.
 * `-d <dazibao path> [-p <port>] [-v[v...]]`
 * @return 0 on success, -1 on error
 * @param argc the number of arguments (as received by main)
 * @param argv arguments (as received by main)
 * @param port result variable, will be filled with the port number
 **/
static int parse_args(int argc, char **argv, int *port) {
        char c;

        dz.fd = -1;
        WSERVER.dzname = NULL;
        WSERVER.dzpath = NULL;
        WSERVER.debug = 0;
        while ((c = getopt(argc, argv, "l:p:d:vD")) != -1) {
                switch (c) {
                case 'p':
                        *port = str2dec_positive(optarg);
                        if (!IN_RANGE(*port, 1, 65535)) {
                                LOGWARN("Wrong port: '%s'", optarg);
                                *port = DEFAULT_PORT;
                        }
                        WSERVER.port = *port;
                        break;
                case 'd':
                        if (dz.fd < 0) {
                                if (dz_open(&dz,  optarg,  O_RDWR) < 0) {
                                        LOGFATAL("Cannot open '%s'", optarg);
                                        return -1;
                                }
                                dz_close(&dz);
                                dz.fd = -2;
                                WSERVER.dzpath = strdup(optarg);
                                WSERVER.dzname = strdup(basename(optarg));
                        }
                        break;
                case 'D':
                        WSERVER.debug = 1;
                        break;
                case 'v':
                        _log_level += 10;
                        break;
                case ':':
                        LOGFATAL("-%c requires an argument", optopt);
                        return -1;
                case '?':
                        LOGWARN("Unrecognized option: '-%c'", optopt);
                }
        }
        return 0;
}

static void init_wserver_infos(void) {
        WSERVER.hostname = strdup("localhost"); /* should be ok for now */
        WSERVER.name = strdup("Daziweb/" DAZIWEB_VERSION);

        if (WSERVER.dzname == NULL) {
                WSERVER.dzname = strdup("<no dazibao>");
        } else {
                /* Remove the extension if there's one */
                char *tmp;
                for (int i=0; WSERVER.dzname[i] != '\0'; i++) {
                        if (WSERVER.dzname[i] == '.') {
                                tmp = (char*)malloc(sizeof(char)*(i+1));
                                strncpy(tmp, WSERVER.dzname, i);
                                tmp[i] = '\0';
                                free(WSERVER.dzname);
                                WSERVER.dzname = tmp;
                                break;
                        }
                }

                /* Capitalize the first word */
                if (WSERVER.dzname[0] >= 97 && WSERVER.dzname[0] <= 122) {
                        WSERVER.dzname[0] -= ('a' - 'A');
                }
        }
}

static void destroy_wserver_infos(void) {
        free(WSERVER.hostname);
        free(WSERVER.dzname);
        free(WSERVER.dzpath);
        free(WSERVER.name);
}

/**
 * main function for the Web server
 **/
int main(int argc, char **argv) {
        int status = 0,
            port = DEFAULT_PORT;

        struct sockaddr_in bindaddr,
                           addr;
        socklen_t len = sizeof(struct sockaddr_in);
        struct sigaction sig;

        LOGDEBUG("Parsing arguments...");

        if (parse_args(argc, argv, &port) != 0) {
                if (dz.fd > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (dz.fd == -1) {
                LOGFATAL("Starting with no dazibao");
                return -1;
        }
        init_wserver_infos();

        LOGDEBUG("Setting up signals traps...");

        memset(&sig, 0, sizeof(sig));
        sig.sa_handler = clean_close;
        sig.sa_flags = 0;

        if (sigaction(SIGINT, &sig, NULL) == -1) {
                perror("sigaction");
                LOGWARN("Cannot intercept SIGINT");
        }
        if (sigaction(SIGSEGV, &sig, NULL) == -1) {
                perror("sigaction");
                LOGWARN("Cannot intercept SIGSEGV");
        }

        LOGDEBUG("Opening a socket...");

        listening_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listening_sock == -1) {
                perror("socket");
                if (dz.fd > 0) {
                        dz_close(&dz);
                }
                destroy_wserver_infos();
                exit(EXIT_FAILURE);
        }

        memset(&bindaddr, 0, sizeof(bindaddr));
        bindaddr.sin_family = AF_INET;
        bindaddr.sin_port = htons(port);
        bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (setsockopt(listening_sock, SOL_SOCKET,
                                SO_REUSEADDR, &status, sizeof(int)) == -1) {
                perror("setsockopt");
        }

        status = bind(listening_sock,
                        (struct sockaddr *)&bindaddr, sizeof(bindaddr));
        if (status == -1) {
                perror("bind");
                close(listening_sock);
                clean_close(0);
        }

        LOGDEBUG("Listening on it...");

        if (listen(listening_sock, MAX_QUEUE) == -1) {
                perror("listen");
                close(listening_sock);
                if (dz.fd > 0) {
                        dz_close(&dz);
                }
                exit(EXIT_FAILURE);
        }

        if (WSERVER.debug) {
                LOGINFO("Serving files in debug mode");
        }

        LOGINFO("Listening on port %d...", port);
        LOGINFO("Press ^C to interrupt.");

        LOGDEBUG("Initializing routes & request struct...");

        register_routes();
        req = create_http_request();

        while (1) {
                int client;
                reset_http_request(req);

                if ((client = accept(listening_sock,
                                (struct sockaddr *)&addr, &len)) == -1) {
                        perror("accept");
                        continue;
                }

                LOGINFO("Got a connection.");
                status = parse_request(client, req);
                if (status != 0) {
                        LOGWARN("request parse error (status=%d)", status);
                        LOGWARN("with method %d, path %s, body length %d",
                                req->method, req->path, req->body_len);
                        if (error_response(client, status) < 0) {
                                LOGERROR("error_response failed");
                        }
                        LOGINFO("Connection closed.");
                        if (close(client) == -1) {
                            perror("close");
                        }
                        if (dz.fd > 0 && dz_close(&dz) < 0) {
                                LOGERROR("Cannot close the Dazibao.");
                        }
                        continue;
                }
                LOGDEBUG("Got method %d, path %s, body length %d",
                               req->method, req->path, req->body_len);
                LOGDEBUG("User-Agent: %s", REQ_HEADER(*req, HTTP_H_UA));

                /* opening the dazibao for use in responses */

                if (dz.fd < 0 && dz_open(&dz, WSERVER.dzpath, O_RDWR) < 0) {
                        LOGERROR("Cannot open the Dazibao.");
                }

                /* <routing+response>  */

                status = route_request(client, dz, req);
                dz_close(&dz);
                dz.fd = -1;

                if (status != 0) {
                        LOGWARN("route error, status=%d", status);
                        if (error_response(client, HTTP_S_NOTFOUND) < 0) {
                                LOGERROR("404 error response failed");
                        }
                }

                /* </routing+response> */

                LOGINFO("Connection closed.");
                if (close(client) == -1) {
                        perror("close");
                }
        }

        LOGINFO("Closing...");
        clean_close(0);
}
