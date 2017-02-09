#include "include.h"

typedef struct mail_node {
	lamport* key;
    update*  value;
    struct mail_node* next;
} mail_node;

//Initialize a mail list and return the dummy head
mail_node* init_mail_list();

//insert a mail to the list in order (new -> old)
int insert_mail(update*, mail_node*);

//compare two mail nodes
int compare(mail_node*, mail_node*);

//Delete a mail node, return 1 if success, return -1 if not
int delete_mail(int index, mail_node*);

//insert a mail to the list in order (old -> new)
int insert_updatemail(update*, mail_node*);