#ifndef __PARSING_H__
#define __PARSING_H__

#include <sys/types.h>
#include <unistd.h>

typedef struct Req Req;

Req *get_new_request();

int delete_request(Req **r);

void get_fields(Req *r, char *request, ssize_t *content_length);

int parse_req(char *buffer, Req *r);

int is_valid_command(Req *r);

int get_put(Req *r);

int is_valid_version(Req *r);

int is_valid_path(Req *r);

int is_dir(char *path);

ssize_t length_file(Req *r);

int is_file_create(Req *r);

int get_fd(Req *r);

char *parse_handle(Req *r, char *inp);

void print_req(Req *r);

ssize_t get_content_len(Req *r);

void close_file(Req *r);

#endif
