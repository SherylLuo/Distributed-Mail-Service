#include "userlist.h"

user_node* init_user_list() {
   user_node *dummy_head = malloc(sizeof(user_node));
   if(dummy_head == NULL) {
      perror("linkedlist: malloc error");
      exit(1);
   }
   dummy_head->next = NULL;
   return dummy_head;
}


int add_user_mail(update* m, user_node* dummy_head) {
	mail_node *mail_head = find_user(m->receiver, dummy_head);
	if(mail_head == NULL) {
		user_node* new_user = malloc(sizeof(user_node));
	   	if(new_user == NULL) {
		  perror("linkedlist: malloc error");
		  exit(1);
	    }
	    strcpy(new_user->key, m->receiver);
	    new_user->value = init_mail_list();
	    new_user->next = dummy_head->next;
	    dummy_head->next = new_user;
	    mail_head = new_user->value;
	}

    return insert_mail(m, mail_head);
}


mail_node* find_user(char* username, user_node* dummy_head) {
	user_node* head = dummy_head->next;
	while(head != NULL) {
		if(strcmp(username, head->key) == 0) {
			return head->value;
		}
		head = head->next;
	}
	return NULL;
}