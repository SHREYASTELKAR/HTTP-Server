#include "parser.h"
#include "asgn2_helper_funcs.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BLOCK 4096

int main(int argc, char **argv) {

    if (argc != 2) {
        return 1;
    }
    int port;
    sscanf(argv[1], "%d", &port);

    Listener_Socket sock;
    int sockfd = listener_init(&sock, port);
    if (sockfd == -1) {
        write(STDERR_FILENO, "Invalid Port\n", 13);
        return 1;
    }

    ssize_t rd;
    char buffer[BLOCK]; //buffer may contain stuff after \r\n\r\n
    char a[BLOCK] = ""; //free
    char b[BLOCK] = "";
    char code[10] = "";
    char type[10] = "";
    char msg[48] = "";
    char bad_req[500] = "";
    char get_resp[BLOCK] = "";
    char put_resp[BLOCK] = "";
    char resp[32];
    char cont[128];
    char m[2];
    char *token;
    char *b_temp;

    while (1) {

        memset(&buffer, 0, BLOCK);
        memset(&a, 0, BLOCK);
        memset(&b, 0, 256);
        memset(&code, 0, 10);
        memset(&type, 0, 10);
        memset(&msg, 0, 48);
        memset(&bad_req, 0, 500);
        memset(&get_resp, 0, BLOCK);
        memset(&put_resp, 0, BLOCK);
        memset(&resp, 0, 32);
        memset(&cont, 0, 128);
        memset(&m, 0, 2);

        int listenfd = listener_accept(&sock);
        if (listenfd == -1) { //get next fd
            continue;
        }
        rd = read_until(listenfd, buffer, 3000, "\r\n\r\n"); //stops until string or end
        if (rd == -1) { //timeout,
            //400, 500
            char rd_fail[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                             "22\r\n\r\nInternal Server Error\n";
            write_all(listenfd, rd_fail, 80);
            continue;
        }

        //maybe null terminate strings.
        //int fd = open("foo.txt", (O_RDWR | O_TRUNC | O_CREAT), 0644);

        b_temp = (strstr(buffer, "\r\n\r\n")
                  + 4); //string after \r\n\r\n, buffer may contain stuff after \r\n\r\n
        //printf("%s\n", buffer);
        size_t point = b_temp - buffer;
        strncat(a, buffer, point); //parse input
        //strcat(a, '\0');
        strcpy(b, b_temp); //has extra chars
        //write(fd, b, 15);

        Req *r = get_new_request();

        //parsing
        strcat(resp, parse_handle(r, a));

        token = strtok(resp, " ");
        strcat(code, token); //code
        //write(fd, code, 3);
        token = strtok(NULL, " ");
        strcat(type, token); //type (good, bad) resp
        token = strtok(NULL, "");
        strcat(msg, token); //msg

        if (strcmp(type, "bad") == 0) { //its a bad request
            sprintf(cont, "%lu", strlen(msg) + 1);
            sprintf(bad_req, "HTTP/1.1 %s %s\r\nContent-Length: %lu\r\n\r\n%s\n", code, msg,
                strlen(msg) + 1, msg);
            write_all(
                listenfd, bad_req, 33 + strlen(code) + strlen(msg) + strlen(cont) + strlen(msg));
        }

        else { //read msg_body
            if (get_put(r) == 0) { //get
                //need to read file and write contents to listenfd;
                sprintf(get_resp, "HTTP/1.1 %s %s\r\nContent-Length: %zd\r\n\r\n", code, msg,
                    length_file(r));

                //write_all(listenfd, get_resp, length_file(r) + 248);
                sprintf(cont, "%zd", length_file(r));
                write_all(listenfd, get_resp, (strlen(code) + strlen(msg) + strlen(cont) + 32));
                pass_bytes(get_fd(r), listenfd, length_file(r));
            }

            else { //put
                //printf("hi\n");
                if (rd > (b_temp
                          - buffer)) { //write extra stuff after \r\n\r\n for put, ignore for get
                    write_all(get_fd(r), b, strlen(b_temp));
                }

                pass_bytes(listenfd, get_fd(r), get_content_len(r)); //write content to txt file

                sprintf(put_resp, "HTTP/1.1 %s %s\r\nContent-Length: %lu\r\n\r\n%s\n", code, msg,
                    strlen(msg) + 1, msg);

                sprintf(m, "%lu", strlen(msg) + 1);
                write_all(listenfd, put_resp,
                    (strlen(code) + strlen(msg) + strlen(m) + strlen(cont) + strlen(msg)
                        + 33)); //write resp
            }
            //check if no content_length present, then return bad
            //does not ensure that content_length is present
        }
        close_file(r);
        close(listenfd);
        delete_request(&r);
    }
    return 0;
}
