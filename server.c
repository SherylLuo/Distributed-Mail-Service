#include "userlist.h"


static int 	    my_id;
static mailbox  Mbox;
static char     Private_group[MAX_GROUP_NAME];
static int      To_exit = 0;
static int 		local_vector[5] = {0, 0, 0, 0, 0};
static int      syn_vectors[5][5];
static int      server_group_count;
static char     target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
static int      current_servers[5];
static int      syn_message_count;
static int      received[5] = {0, 0, 0, 0, 0};
static int      time_stamp = 0;
static char     User[MAX_NAME_LEN];
user_node*      userlist;
mail_node*      updatelist;

static status   my_status;

static void     Read_message();
static update*  readMailFromFile(char*);
static update*  readUpdateFromFile(char*);
static void     readBackupInMemory();
static void     writeUpdateToFile(update*);
static void     deliverToFile(update*);

int main (int argc, char* argv[]) {
    
    int         ret;    
    char        public_server_group[7] = "server";
    char        private_client_group[13] = "client_server";

    if (argc != 2) {
        printf("Usage: server <server_index>\n");
        return 0;
    } else {
        my_id = atoi(argv[1]);
    }

    public_server_group[6] = *argv[1];
    private_client_group[13] = *argv[1];

    sprintf(User, "%d", my_id);

    ret = SP_connect( SPREAD_NAME, User, 0, 1, &Mbox, Private_group);
    if (ret != ACCEPT_SESSION) {
        printf("Initial connection error\n");
        To_exit = 1;
        SP_error(ret);
        SP_disconnect(Mbox);
        exit(0);
    }

    printf("Connected to <%s> with private group <%s>\n", SPREAD_NAME, Private_group);

    // Join group with all servers
    ret = SP_join(Mbox, GROUP_NAME);
    if (ret < 0) {
        printf("Error joining server update group <%s>\n", GROUP_NAME);
        To_exit = 1;
        SP_error(ret);
        SP_disconnect(Mbox);
        exit(0);
    }

    // Join the public group with only itself
    ret = SP_join(Mbox, public_server_group);
    if (ret < 0) {
        printf("Error joining private server group <%s>\n", public_server_group);
        To_exit = 1;
        SP_error(ret);
        SP_disconnect(Mbox);
        return 0;
    }

    // Join the group for membership
    ret = SP_join(Mbox, private_client_group);
    if (ret < 0) {
        printf("Error joining private client group <%s>\n", private_client_group);
        To_exit = 1;
        SP_error(ret);
        SP_disconnect(Mbox);
        return 0;
    }

    userlist = init_user_list();
    updatelist = init_mail_list();

    readBackupInMemory();

    E_init();
    E_attach_fd(Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);
    E_handle_events();

    return 0;
}

static void Read_message() {
    static  char     mess[MAX_MESS_LEN];
    char             sender[MAX_GROUP_NAME];    
    membership_info  memb_info;
    vs_set_info      vssets[MAX_VSSETS];
    unsigned int     my_vsset_index;
    int              num_vs_sets;
    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
    int              num_groups;
    int              service_type;
    int16            mess_type;
    int              endian_mismatch;
    int              i,j;
    int              ret;
    update*			 update_buffer;

    service_type = 0;

    ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups, 
        &mess_type, &endian_mismatch, sizeof(mess), mess );

    if(ret < 0) {
        if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
            service_type = DROP_RECV;
            printf("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups, 
                &mess_type, &endian_mismatch, sizeof(mess), mess );
        }
    }

    if(ret < 0) {
        if(!To_exit)
        {
            printf("%d\n", ret );
            SP_error( ret );
            printf("\n============================\n");
            printf("\nBye.\n");

        }
        exit(0);
    }

    if( Is_regular_mess( service_type ) )
    {       
        update_buffer = (update*)mess;

        // printf("Current number of servers is %d\n", server_group_count);

        // Merge message
        if(update_buffer->type == SYN_TYPE) {
            status* received_status = (status*)update_buffer;
            for(i = 0; i < 5; i++) {
                syn_vectors[received_status->server_id - 1][i] = received_status->vector[i];
            }
            // printf("receive syn type message from %d with vectorss {%d %d %d %d %d}\n", received_status->server_id, received_status->vector[0],
                // received_status->vector[1], received_status->vector[2], received_status->vector[3], received_status->vector[4]);

            // when receive syn message from a new server, increment counter
             if(received[received_status->server_id - 1] == 0) {
                received[received_status->server_id - 1] = 1;
                syn_message_count++;
            }

            // printf("current syn msg count = %d\n", syn_message_count);

            // upon receiving all the vectors in current partition
            if(syn_message_count == server_group_count) {
                for(i = 0; i < server_group_count; i++) {
                    int min = local_vector[current_servers[i] - 1];
                    int max = min;
                    int updater = my_id;
                    for(j = 0; j < server_group_count; j++){
                        if(syn_vectors[current_servers[j] - 1][current_servers[i] - 1] < min) {
                            min = syn_vectors[current_servers[j] - 1][current_servers[i] - 1];
                        }
                        if(syn_vectors[current_servers[j] - 1][current_servers[i] - 1] > max) {
                            max = syn_vectors[current_servers[j] - 1][current_servers[i] - 1];
                            updater = current_servers[j];
                        } else if(syn_vectors[current_servers[j] - 1][current_servers[i] - 1] == max && current_servers[j] < updater) {
                            updater = current_servers[j];
                        }
                    }

                    if(min == max) {
                        continue;
                    }

                    if(updater == my_id) {
                        mail_node* temp = updatelist;
                        while(temp->next != NULL) {
                            if(temp->next->key->server_id == current_servers[i] && temp->next->key->time_stamp > min) {
                                break;
                            }

                            // If all the servers merged, delete update mail node and relative files
                            if(server_group_count == 5) {
                                mail_node* to_be_free = temp->next;
                                temp->next = temp->next->next;
                                char to_be_delete[2 * MAX_NAME_LEN] = "backups";
                                sprintf(&to_be_delete[7], "%d%s%d%s%d", my_id, "/up_", to_be_free->key->server_id, "_", to_be_free->key->time_stamp);
                                remove(to_be_delete);
                                free(to_be_free);
                            } else {
                                temp = temp->next;
                            }
                        }

                        // Resend the updates
                        while(temp->next != NULL) {
                            printf("server %d resending updates %d_%d\n", my_id, temp->next->key->server_id, temp->next->key->time_stamp);
                            ret = SP_multicast(Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(update), (char *)temp->next->value);
                            if(ret < 0) {
                                SP_error(ret);
                                exit(0);
                            }
                            if(server_group_count == 5) {
                                mail_node* to_be_free = temp->next;
                                temp->next = temp->next->next;
                                char to_be_delete[2 * MAX_NAME_LEN] = "backups";
                                sprintf(&to_be_delete[7], "%d%s%d%s%d", my_id, "/up_", to_be_free->key->server_id, "_", to_be_free->key->time_stamp);
                                remove(to_be_delete);
                                free(to_be_free);
                            } else {
                                temp = temp->next;
                            }
                        }
                    }
                }
            }
        } else if(update_buffer->source == FROM_CLIENT) {

            // new mail
        	if(update_buffer->type == MAIL_TYPE) {
                printf("Receiving new mail from client at time %d\n", time_stamp);

                local_vector[my_id - 1] = time_stamp;

                update_buffer->stamp.server_id = my_id;
                update_buffer->stamp.time_stamp = time_stamp++;
                update_buffer->source = FROM_SERVER;

                if(server_group_count != 5) {
                    writeUpdateToFile(update_buffer);
                    insert_updatemail(update_buffer, updatelist);
                }

        		deliverToFile(update_buffer);

                add_user_mail(update_buffer, userlist);

        		ret = SP_multicast( Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(update), (char *)update_buffer );
                if( ret < 0 ) {
                    SP_error( ret );
                    exit(0);
                }

        	} else if(update_buffer->type == READ_TYPE) {
                // read type, find the node first
                mail_node* pending_processing = find_user(update_buffer->receiver, userlist);
                if(pending_processing == NULL) {
                    printf("user doesn't exist!\n");
                    return;
                }

                for(i = 0; i < update_buffer->update_index; i++) {
                    pending_processing = pending_processing->next;
                    if(pending_processing == NULL) {
                        printf("Mail doesn't exist!\n");
                        return;
                    }
                }

                local_vector[my_id - 1] = time_stamp;

                pending_processing->value->read = 1;
                deliverToFile(pending_processing->value);

                char file_name[2 * MAX_NAME_LEN] = "backups";
                sprintf(&file_name[7], "%d%s%d%s%d", my_id, "/", pending_processing->key->server_id, "_", pending_processing->key->time_stamp);
                strcpy(&update_buffer->msg[0], &file_name[0]);

                update_buffer->source = FROM_SERVER;
                update_buffer->stamp.server_id = my_id;
                update_buffer->stamp.time_stamp = time_stamp++;

                if(server_group_count != 5) {
                    writeUpdateToFile(update_buffer);
                    insert_updatemail(update_buffer, updatelist);
                }

                // Send update to other servers, with msg field set to file name pending change
                ret = SP_multicast( Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(update), (char *)update_buffer );
                if( ret < 0 ) {
                    SP_error( ret );
                    exit(0);
                }
                // Send response to user
                pending_processing->value->type = READ_TYPE;
                pending_processing->value->update_index = update_buffer->update_index;
                ret = SP_multicast(Mbox, AGREED_MESS, update_buffer->subject, 1, sizeof(update), (char *)pending_processing->value);
                if(ret < 0) {
                    SP_error(ret);
                    exit(0);
                }

        	} else if(update_buffer->type == DELETE_TYPE) {
                mail_node* pending_processing = find_user(update_buffer->receiver, userlist);
                if(pending_processing == NULL) {
                    printf("user doesn't exist!\n");
                    return;
                }
                for(i = 1; i < update_buffer->update_index; i++) {
                    if(pending_processing == NULL) {
                        printf("Mail doesn't exist!\n");
                        return;
                    }
                    pending_processing = pending_processing->next;
                }

                local_vector[my_id - 1] = time_stamp;
                update_buffer->source = FROM_SERVER;
                update_buffer->stamp.server_id = my_id;
                update_buffer->stamp.time_stamp = time_stamp++;

                char file_name[2 * MAX_NAME_LEN] = "backups";
                mail_node* temp = pending_processing->next;
                sprintf(&file_name[7], "%d%s%d%s%d", my_id, "/", temp->key->server_id, "_", temp->key->time_stamp);
                strcpy(update_buffer->msg, file_name);

                if(server_group_count != 5) {
                    writeUpdateToFile(update_buffer);
                    insert_updatemail(update_buffer, updatelist);
                    printf("In partition, write delete to update\n");
                }

                // Send update to other servers, with msg field set to file name pending change
                ret = SP_multicast( Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(update), (char *)update_buffer );
                if( ret < 0 ) {
                    SP_error( ret );
                    exit(0);
                }

                remove(file_name);
                pending_processing->next = pending_processing->next->next;
                free(temp);

                // Send response to user
                ret = SP_multicast(Mbox, AGREED_MESS, update_buffer->subject, 1, sizeof(update), (char *)update_buffer);
                if(ret < 0) {
                    SP_error(ret);
                    exit(0);
                }
            } else if(update_buffer->type == LIST_TYPE) {
                mail_node* head = find_user(update_buffer->receiver, userlist);
                if(head == NULL) {
                    printf("user doesn't exist!\n");
                    return;
                }
                while(head->next != NULL) {
                    // Send response to user
                    head = head->next;
                    head->value->type = LIST_TYPE;
                    ret = SP_multicast(Mbox, AGREED_MESS, update_buffer->subject, 1, sizeof(update), (char *)head->value);
                    if(ret < 0) {
                        SP_error(ret);
                        exit(0);
                    }
                }

                // A message indicates list sending over
                printf("finished sending list to %s\n", update_buffer->subject);
                update_buffer->type = FINAL_TYPE;
                ret = SP_multicast(Mbox, AGREED_MESS, update_buffer->subject, 1, sizeof(update), (char *)update_buffer);
                if(ret < 0) {
                    SP_error(ret);
                    exit(0);
                }

        	} else if(update_buffer->type == VIEW_TYPE) {

                for(i = 0; i < 5; i++) {
                    update_buffer->membership[i] = current_servers[i];
                }
                printf("sending membership to %s\n", update_buffer->subject);
                ret = SP_multicast(Mbox, AGREED_MESS, update_buffer->subject, 1, sizeof(update), (char *)update_buffer);
                if(ret < 0) {
                    SP_error(ret);
                    exit(0);
                }
            }
        } else if(update_buffer->source == FROM_SERVER) {
    		int source_server = update_buffer->stamp.server_id;
	    	
	    	if(update_buffer->stamp.time_stamp <= local_vector[source_server - 1]) {
                // printf("received old message\n");
                // If receive old message when all the servers together, delete relative update file if applicable
                if(server_group_count == 5) {
                    mail_node* temp = updatelist;
                    while(temp->next != NULL) {
                        if(temp->next->key->server_id == source_server && temp->next->key->time_stamp == update_buffer->stamp.time_stamp) {
                            mail_node* to_be_free = temp->next;
                            char to_be_delete[2 * MAX_NAME_LEN] = "backups";
                            sprintf(&to_be_delete[7], "%d%s%d%s%d", my_id, "/up_", temp->next->key->server_id, "_", temp->next->key->time_stamp);
                            remove(to_be_delete);
                            temp->next = temp->next->next;
                            free(to_be_free);
                            break;
                        }
                        temp = temp->next;
                    }
                }
	    		return;
	    	}

            if(source_server == my_id) {
                // printf("received my own message\n");
                return;
            }

            local_vector[source_server - 1] = update_buffer->stamp.time_stamp;

            // Adopt time stamp
            if(update_buffer->stamp.time_stamp >= time_stamp) {
                time_stamp = update_buffer->stamp.time_stamp + 1;
            }

            if(server_group_count != 5) {
                writeUpdateToFile(update_buffer);
                insert_updatemail(update_buffer, updatelist);
            }

            if(update_buffer->type == MAIL_TYPE) {

                printf("received new mail from server %d\n", update_buffer->stamp.server_id);
                deliverToFile(update_buffer);
                add_user_mail(update_buffer, userlist);

            } else if(update_buffer->type == READ_TYPE || update_buffer->type == DELETE_TYPE) {
                printf("received update from server %d\n", update_buffer->stamp.server_id);

                int ser = update_buffer->msg[9] - '0';
                int ts = atoi(&update_buffer->msg[11]);

                mail_node* head = find_user(update_buffer->receiver, userlist);
                if(head == NULL) {
                    printf("user doesn't exists\n");
                    return;
                }
                while(head->next != NULL) {
                    if(head->next->key->server_id == ser && head->next->key->time_stamp == ts) {
                        if(update_buffer->type == READ_TYPE) {
                            head->next->value->read = 1;
                            deliverToFile(head->next->value);
                        } else {
                            head->next = head->next->next;
                            update_buffer->msg[7] = my_id + '0';
                            remove(update_buffer->msg);
                        }
                        break;
                    }
                    head = head->next;
                }
        	}
        	
        }


    } else if( Is_membership_mess( service_type ) ) {
        ret = SP_get_memb_info( mess, service_type, &memb_info );
        if (ret < 0) {
            printf("BUG: membership message does not have valid body\n");
            SP_error( ret );
            exit( 1 );
        }

        /* deal with membership message*/

        if( Is_reg_memb_mess( service_type ) )
        {
            printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                sender, num_groups, mess_type );
            for( i=0; i < num_groups; i++ )
                printf("\t%s\n", &target_groups[i][0] );
            printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );

            // Update current servers being together
            if(strcmp(sender, "servers_all") == 0) {
                server_group_count = num_groups;
                for(i = 0; i < server_group_count; i++) {
                    current_servers[i] = atoi(&target_groups[i][1]);
                }
                for(; i < 5; i++) {
                    current_servers[i] = -1;
                }
            }

            if( Is_caused_join_mess( service_type ) )
            {
                printf("Due to the JOIN of %s\n", memb_info.changed_member );

                if (strcmp(sender, "servers_all") == 0) {
                    my_status.type = SYN_TYPE;
                    my_status.server_id = my_id;
                    for (int i = 0; i < 5; i++) {
                        my_status.vector[i] = local_vector[i];
                    }

                    // Initialize counter and tracker preparing for new merge
                    syn_message_count = 0;
                    for(i = 0; i < 5; i++) {
                        received[i] = 0;
                    }

                    ret = SP_multicast(Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(status), (char *)&my_status);
                    if(ret != sizeof(status))
                    {
                        if( ret < 0 )
                        {
                            SP_error( ret );
                            exit(1);
                        }
                    }
                }
            } else if( Is_caused_leave_mess( service_type ) ){
                printf("Due to the LEAVE of %s\n", memb_info.changed_member );
            } else if( Is_caused_disconnect_mess( service_type ) ){
                printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
            } else if( Is_caused_network_mess( service_type ) ){
                printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
                num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
                if (num_vs_sets < 0) {
                    printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
                    SP_error( num_vs_sets );
                    exit( 1 );
                }
                for( i = 0; i < num_vs_sets; i++ )
                {
                    printf("%s VS set %d has %u members:\n",
                        (i  == my_vsset_index) ?
                        ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
                    ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
                    if (ret < 0) {
                        printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
                        SP_error( ret );
                        exit( 1 );
                    }
                    for( j = 0; j < vssets[i].num_members; j++ )
                        printf("\t%s\n", members[j] );
                }

                if (strcmp(sender, "servers_all") == 0 && num_vs_sets > 1) {
                    my_status.type = SYN_TYPE;
                    my_status.server_id = my_id;
                    for (int i = 0; i < 5; i++) {
                        my_status.vector[i] = local_vector[i];
                    }

                    // Initialize counter and tracker preparing for new merge
                    syn_message_count = 0;
                    for(i = 0; i < 5; i++) {
                        received[i] = 0;
                    }

                    ret = SP_multicast(Mbox, AGREED_MESS, GROUP_NAME, 1, sizeof(status), (char *)&my_status);
                    if(ret != sizeof(status))
                    {
                        if( ret < 0 )
                        {
                            SP_error( ret );
                            exit(1);
                        }
                    }
                }
            }
        } else if( Is_transition_mess(   service_type ) ) {
            printf("received TRANSITIONAL membership for group %s\n", sender );
        } else if( Is_caused_leave_mess( service_type ) ){
            printf("received membership message that left group %s\n", sender );
        } else 
            printf("received incorrecty membership message of type 0x%x\n", service_type );
    } else if ( Is_reject_mess( service_type ) ) {
        printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
          sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
    } else 
        printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

}


static update* readMailFromFile(char* filename) {
    update* result = malloc(sizeof(update));
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", &result->stamp.server_id);
    fscanf(file, "%d", &result->stamp.time_stamp);
    fscanf(file, "%d", &result->read);
    fscanf(file, "%s", &result->sender[0]);
    fscanf(file, "%s", &result->receiver[0]);
    fscanf(file, "%s", &result->subject[0]);
    fscanf(file, "%s", &result->msg[0]);
    fclose(file);
    printf("reading file from backup: server: %d | ts: %d | read: %d | sender: %s | receiver: %s | sub: %s | msg: %s\n", 
        result->stamp.server_id, result->stamp.time_stamp, result->read, result->sender, result->receiver,
            result->subject, result->msg);
    return result;
}

static update* readUpdateFromFile(char* filename) {
    update* result = malloc(sizeof(update));
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", &result->type);
    fscanf(file, "%d", &result->stamp.server_id);
    fscanf(file, "%d", &result->stamp.time_stamp);
    fscanf(file, "%d", &result->read);
    fscanf(file, "%d", &result->update_index);
    fscanf(file, "%s", &result->sender[0]); 
    fscanf(file, "%s", &result->receiver[0]);
    fscanf(file, "%s", &result->subject[0]);
    fscanf(file, "%s", &result->msg[0]);
    fclose(file);
    printf("reading update file from backup: type: %d | server: %d | ts: %d | read: %d | index: %d | sender: %s | receiver: %s | sub: %s | msg: %s\n", result->type,
        result->stamp.server_id, result->stamp.time_stamp, result->read, result->update_index, result->sender, result->receiver,
            result->subject, result->msg);
    return result;
}

static void readBackupInMemory() {
    DIR* dir;
    struct dirent* infile;
    int currmax = 0;

    char file_name[2 * MAX_NAME_LEN] = "backups";
    sprintf(&file_name[7], "%d%s", my_id, "/");
    if((dir = opendir(file_name)) != NULL) {
        while((infile = readdir(dir))) {
            if (!strncmp(infile->d_name, ".", 1))
                continue;
            if (!strncmp (infile->d_name, "..", 2))    
                continue;
            sprintf(&file_name[9], "%s", infile->d_name);
            printf("restore from backup %s\n", file_name);
            if(strncmp(infile->d_name, "up", 2) == 0) {
                update* curr = readUpdateFromFile(file_name);
                insert_updatemail(curr, updatelist);
                int curr_server = curr->stamp.server_id;
                if(curr_server == my_id && curr->stamp.time_stamp > currmax) {
                    currmax = curr->stamp.time_stamp;
                }
                if(curr->stamp.time_stamp > local_vector[curr_server - 1]) {
                    local_vector[curr_server - 1] = curr->stamp.time_stamp;
                }
            } else {
                update* curr = readMailFromFile(file_name);
                add_user_mail(curr, userlist);
                int curr_server = curr->stamp.server_id;
                if(curr_server == my_id && curr->stamp.time_stamp > currmax) {
                    currmax = curr->stamp.time_stamp;
                }
                if(curr->stamp.time_stamp > local_vector[curr_server - 1]) {
                    local_vector[curr_server - 1] = curr->stamp.time_stamp;
                }
            }
        }
        closedir(dir);
        time_stamp = currmax + 1;
    } else {
        printf("Invalid input! \n");
        exit(0);
    }
}

static void writeUpdateToFile(update* up) {
    char file_name[2 * MAX_NAME_LEN] = "backups";
    sprintf(&file_name[7], "%d%s%d%s%d", my_id, "/up_", up->stamp.server_id, "_", up->stamp.time_stamp);
    FILE* update_file;
    if((update_file = fopen(file_name, "w")) == NULL) {
        printf("Cannot open file %s\n", file_name);
        exit(0);
    }
    fprintf(update_file, "%d %d %d %d %d %s %s %s %s\n", up->type, up->stamp.server_id, up->stamp.time_stamp, up->update_index,
        up->read, up->sender, up->receiver, up->subject, up->msg);
    fclose(update_file);
}

static void deliverToFile(update* up) {
    char file_name[2 * MAX_NAME_LEN] = "backups";
    sprintf(&file_name[7], "%d%s%d%s%d", my_id, "/", up->stamp.server_id, "_", up->stamp.time_stamp);
    printf("deliver to file %s\n", file_name);
    FILE* update_file;
    if((update_file = fopen(file_name, "w")) == NULL) {
        printf("Cannot open file %s\n", file_name);
        exit(0);
    }
    fprintf(update_file, "%d %d %d %s %s %s %s\n", up->stamp.server_id, up->stamp.time_stamp, up->read, 
        up->sender, up->receiver, up->subject, up->msg);
    fclose(update_file);
}
