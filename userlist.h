#include "mail_list.h"

typedef struct user_node {
	char key[MAX_NAME_LEN];
	mail_node* value;
	struct user_node* next;
} user_node;


//Initialize a user list and return the dummy head
user_node* init_user_list();

// add an update to user list, return 1 if success, return -1 if not
int add_user_mail(update*, user_node*);

// Find whether a username exist or not. If exists, return the mail list dummy head of that user, if not, return null
mail_node* find_user(char*, user_node*);