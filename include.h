#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "sp.h"

#define SPREAD_NAME "10350"
#define GROUP_NAME "servers_all"
#define MAX_GROUPS 100
#define MAX_MESS_LEN  1400
#define MAX_MEMBERS   100
#define MAX_VSSETS 10
#define MAX_NAME_LEN 20
#define MAX_SUB_LEN 100
#define MAX_MSG_LEN 1000

#define MAIL_TYPE 0
#define READ_TYPE 1
#define DELETE_TYPE 2
#define LIST_TYPE 3
#define VIEW_TYPE 4
#define SYN_TYPE 5
#define FINAL_TYPE 6

#define FROM_CLIENT 0
#define FROM_SERVER 1

typedef struct {
    int server_id;
    int time_stamp;
} lamport;

typedef struct {
    int  type;
    char source;
    lamport stamp;
    int  update_index;
    int  read;
    int  membership[5];
    char sender[MAX_NAME_LEN];
    char receiver[MAX_NAME_LEN];
    char subject[MAX_SUB_LEN];
    char msg[MAX_MSG_LEN];
} update;

typedef struct {
    int type;
    int server_id;
    int vector[5];
} status;