#include "mail_list.h"

mail_node* init_mail_list() {
    mail_node *dummy_head = malloc(sizeof(mail_node));
    if(dummy_head == NULL) {
        perror("linkedlist: malloc error");
        exit(1);
    }
    dummy_head->next = NULL;
    return dummy_head;
}


int insert_mail(update* m, mail_node* dummy_head) {
  	mail_node *curr = malloc(sizeof(mail_node));
    if(curr == NULL) {
  	    perror("linkedlist: malloc error");
  	    exit(1);
    }
    update* to_be_insert = malloc(sizeof(update));
    if(to_be_insert == NULL) {
        perror("linkedlist: malloc error");
        exit(1);
    }
    memcpy(to_be_insert, m, sizeof(update));
    curr->key = &to_be_insert->stamp;
    curr->value = to_be_insert;
    mail_node* head = dummy_head;
    while(head->next != NULL) {
      	if(compare(curr, head->next) > 0) {
        		break;
      	} else if(compare(curr, head->next) == 0) {
      		  return -1;
      	}
      	head = head->next;
    }
    curr->next = head->next;
    head->next = curr;
    return 1;
}

// Compare two mail_nodes, return -1 if the first argument is smaller, return 0 if equal, return 1 if the first argument is larger
int compare(mail_node* node1, mail_node* node2) {
  	if(node1->key->time_stamp < node2->key->time_stamp) {
  		  return -1;
  	} else if(node1->key->time_stamp > node2->key->time_stamp) {
  		  return 1;
  	} else {
    		if(node1->key->server_id < node2->key->server_id) {
    			return -1;
    		} else if(node1->key->server_id > node2->key->server_id) {
    			return 1;
    		} else {
    			return 0;
    		}
  	}
}

int insert_updatemail(update* m, mail_node* dummy_head) {
    mail_node *curr = malloc(sizeof(mail_node));
    if(curr == NULL) {
        perror("linkedlist: malloc error");
        exit(1);
    }
    update* to_be_insert = malloc(sizeof(update));
    if(to_be_insert == NULL) {
        perror("linkedlist: malloc error");
        exit(1);
    }
    memcpy(to_be_insert, m, sizeof(update));
    curr->key = &to_be_insert->stamp;
    curr->value = to_be_insert;
    mail_node* head = dummy_head;
    while(head->next != NULL) {
        if(compare(curr, head->next) < 0) {
            break;
        } else if(compare(curr, head->next) == 0) {
            return -1;
        }
        head = head->next;
    }
    curr->next = head->next;
    head->next = curr;
    return 1;
}