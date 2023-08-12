#include "parser.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdbool.h>

#define FORMAT                                                                                     \
    "^([a-zA-Z]{1,8}) (/[a-zA-Z0-9.-]{1,63}) (HTTP/[0-9]\\.[0-9]\r\n)([a-zA-Z0-9.-]{1,128}: [ "    \
    "-~]{0,128}\r\n)*\r\n$"

//"PUT foo.txt HTTP/1.1\r\nType: asd\r\nContent-Length: 120\r\nMore: asd\r\n\r\n";

struct Req {
    int in_fd;
    char command[20];
    char uri[128];
    char version[20];
    char content[20];
    bool file_created;
    bool directory_found;
    ssize_t content_length;
};

Req *get_new_request() { //maybe in_fd
    Req *r = (Req *) malloc(sizeof(Req));
    if (r) {
        r->content_length = 0;
        r->in_fd = -1;
        r->file_created = false;
        r->directory_found = false;
    }
    return r;
}

int delete_request(Req **r) {
    if (*r) {
        free(*r);
        *r = NULL;
    }
    return 0;
}

void get_fields(Req *r, char *request, ssize_t *content_length) {
    char *token;
    token = strtok(request, "\r\n");
    sscanf(token, "%s %s %s", r->command, r->uri, r->version);
    while (token != NULL) {
        if (strstr(token, "Content-Length:") != NULL) {
            sscanf(token, "%s %zd", r->content, content_length);
            break;
        }
        token = strtok(NULL, "\r\n");
    }
    return;
}

int parse_req(char *buffer, Req *r) { //int file_fd(maybe)
    regex_t re;
    regmatch_t groups[5];
    char request[2048];

    if (regcomp(&re, FORMAT, REG_EXTENDED | REG_NEWLINE) != 0) {
        return -1;
    }

    if (regexec(&re, buffer, 5, groups, 0) != 0) { //successful format
        return -1; //400
    }

    strcpy(request, buffer);
    get_fields(r, request, &(r->content_length));
    regfree(&re);
    return 0;
}

int is_valid_command(Req *r) { //501
    if (strcmp(r->command, "GET") == 0 | strcmp(r->command, "PUT") == 0) {
        return 0;
    } else {
        return -1;
    }
}

int get_put(Req *r) {
    if (strcmp(r->command, "GET") == 0) {
        return 0;
    } else { //PUT
        return -1;
    }
}

int is_valid_version(Req *r) { //505
    //get numbers  HTTP/1.1

    if (strstr(r->version, "1.1") != NULL) {
        return 0;
    } else {
        return -1;
    }
}

int is_dir(char *path) { //403
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

int is_valid_path(Req *r) {
    char ur[128];
    strcpy(ur, r->uri + 1); //everything after the slash

    if (is_dir(ur)) {
        r->directory_found = true;
        return 0;
    }

    if (strcmp(r->command, "GET") == 0) {
        r->in_fd = open(ur, O_RDWR, 0644);
    }

    else { //PUT
        r->in_fd = open(ur, (O_RDWR | O_TRUNC), 0644);
        if (r->in_fd < 0) { //create file, 201
            r->in_fd = open(ur, (O_RDWR | O_TRUNC | O_CREAT), 0644);
            r->file_created = true;
        } else { //file exists, 200
            r->file_created = false;
        }
    }

    if (r->in_fd < 0) { //404
        return -1;
    } else {
        return 0;
    }
}

ssize_t length_file(Req *r) {
    struct stat *statbuf;
    statbuf = malloc(sizeof(struct stat));
    if (fstat(r->in_fd, statbuf) == 0) {
        return statbuf->st_size;
    } else {
        return -1;
    }
}

int is_file_create(Req *r) {
    if (r->file_created == true) {
        return 0;
    } else {
        return -1;
    }
}

int get_fd(Req *r) {
    return r->in_fd;
}

char *parse_handle(Req *r, char *inp) { //returns "code message"
    char *resp;
    if (parse_req(inp, r) != 0) { //format wrong 400
        resp = "400 bad Bad Request";
        return (resp);
    }
    if (is_valid_command(r) != 0) { //501
        resp = "501 bad Not Implemented";
        return (resp);
    }
    if (is_valid_version(r) != 0) { //505
        resp = "505 bad Version Not Supported";
        return (resp);
    }
    if (is_valid_path(r) != 0) { //404
        resp = "404 bad Not Found";
        return (resp);
    }
    if (r->directory_found == true) { //403
        resp = "403 bad Forbidden";
        return (resp);
    }
    if (is_file_create(r) == 0) {
        resp = "201 good Created";
        return (resp);
    }

    resp = "200 good OK";
    return (resp);
}

void print_req(Req *r) {
    printf("%s %s %s %zd %d\n", r->command, r->uri, r->version, r->content_length, r->file_created);
}

ssize_t get_content_len(Req *r) {
    return r->content_length;
}

void close_file(Req *r) {
    close(r->in_fd);
}
