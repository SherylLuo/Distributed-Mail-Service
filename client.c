#include "sp.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include "include.h"

static  char  User[80];
static  char  Spread_name[80];
static  mailbox Mbox;

static  char  private_group[MAX_GROUP_NAME];
static  char  *username_addr;
static  char  username_sscanf[MAX_NAME_LEN];
static  char  username[MAX_NAME_LEN];
static  char  client_server_group[MAX_GROUP_NAME];
static  char  *receiver_name;
static  char  current_group[MAX_NAME_LEN] = {'\0'};
static  int   server_index = -1;
static  bool  isConnected = NULL;
static  char  server_private_group[MAX_GROUP_NAME];

static  int   isLoggedWithUser = 0;
static  int   current_server_index = -1;

static update *list = NULL;
static update *mail = NULL; 
static update *delete = NULL;
static update *read = NULL;
static update *view = NULL;

/*parameters of read message()*/

static  int    list_index = 1;
static  char   Is_read[10] = "unread";

static  update* reply = NULL;


static void Usage(int argc, char const *argv[]);
static void Bye();
static void Print_menu();
static void User_command();
static void Read_message();

int main(int argc, char const *argv[])
{
	int     ret;
  int     mver, miver, pver;
  sp_time test_timeout;

  test_timeout.sec = 5;
  test_timeout.usec = 0;
  Usage( argc, argv );
  if (!SP_version( &mver, &miver, &pver)) 
  {
      printf("main: Illegal variables passed to SP_version()\n");
      Bye();
  }
  printf("Spread library version is %d.%d.%d\n", mver, miver, pver);


	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, private_group, test_timeout );
	if( ret != ACCEPT_SESSION ) 
	{
		SP_error( ret );
		exit(1);
	}

	printf("User: connected to %s with private group %s\n", Spread_name, private_group );

	Print_menu();
	printf("\nUser> ");

  	E_init();
  	E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );
  	E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY );
  	E_handle_events();
    return 0;
}
  static void User_command()
  {
    int    ret;
  	char   command[100];
    char   receiver_fgets[MAX_NAME_LEN];
    char   subject[MAX_SUB_LEN];
    char   message[MAX_MSG_LEN];
    int    delete_index;
    int    read_index;


  	for (int i = 0; i < sizeof(command); i++)	command[i] = '\0';

  	if (fgets(command, sizeof(command), stdin) == NULL)
  	{
  	Bye();
  	}

  	switch(command[0]){

  		case 'u':

        if(current_group[0] != '\0'){
        SP_leave(Mbox, current_group);
        }
        for (int i = 0; i < sizeof(username_sscanf); i++) username_sscanf[i] = '\0';

        ret = sscanf(&command[2], "%s", username_sscanf);
        if (ret <= 0)
        {
          printf("Invalid username.\n");
          break;
        }else
        {
          username_addr = strtok(username_sscanf, "\n");
          strcpy(username, username_addr);
          printf("Current user [%s]\n", username);
          sprintf(private_group, "client_%s", username);
          SP_join(Mbox, private_group);
          strcpy(current_group, private_group);
          isLoggedWithUser = 1;
          isConnected = 0;
          current_server_index = -1;
        } 
        break;

      case 'c':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        ret = sscanf(&command[2], "%d", &server_index);

        if (ret <= 0)
        {
          printf("Invalid server index\n");
          break;
        }else if (server_index < 1 || server_index > 5)
        {
          printf("Invalid server_index, plz choose from [1,5] \n");
          break;
        }

        if (current_server_index != -1)
        {
          if (current_server_index == server_index)
          {
            printf("Already connected to server %d\n", server_index );
            break;
          }else{
          SP_leave(Mbox, client_server_group);
          current_server_index = server_index;
          }
        }
        sprintf(client_server_group,"client_server%d", server_index);
        SP_join(Mbox, client_server_group);
        printf("Connected to the new server%d\n", server_index);
        current_server_index = server_index;
        sprintf(server_private_group, "server%d", server_index);
        isConnected = 1;
        break;

      case 'l':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        if (isConnected == 0)
        {
          printf("Please connect to a server\n");
          break;
        }

        list = malloc(sizeof(update));
        list->type = LIST_TYPE;
        list->source = FROM_CLIENT;
        strncpy(list->receiver, username, MAX_NAME_LEN);
        strncpy(list->subject, private_group, MAX_GROUP_NAME);

        ret = SP_multicast( Mbox, AGREED_MESS, server_private_group, 1, sizeof(update),(char *)list);

        if( ret <= 0 )
        {
          SP_error( ret );
          exit(1);
        }
        break;

      case 'm':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        if (isConnected == 0)
        {
          printf("Please connect to a server\n");
          break;
        }

        printf("to:\n");

        if (fgets(receiver_fgets, MAX_NAME_LEN, stdin) == NULL) 
          {
            printf("Invalid receiver_name.\n");
            break;
          }
        receiver_name = strtok(receiver_fgets, "\n");

        printf("subject:\n");
        if (fgets(subject, MAX_SUB_LEN, stdin) == NULL) 
          {
            printf("Invalid subject.\n");
            break;
          }

        printf("to: %s\n", receiver_name);
        printf("subject: %s\n", subject);
        printf("enter message:\n");

        if (fgets(message, MAX_MSG_LEN, stdin) == NULL) 
          {
            printf("Invalid subject.\n");
            break;
          }

        mail = (update*)malloc(sizeof(update));
        mail->type = MAIL_TYPE;
        mail->source = FROM_CLIENT;
        strncpy(mail->sender, username, MAX_NAME_LEN);
        strncpy(mail->receiver, receiver_name, MAX_NAME_LEN);
        strncpy(mail->subject, subject, MAX_SUB_LEN);
        strncpy(mail->msg, message, MAX_MSG_LEN);

        ret = SP_multicast(Mbox, AGREED_MESS, server_private_group, 1, sizeof(update),(char *)mail);
        if( ret <= 0 ) 
        {
          SP_error( ret );
          exit(1);
        }
        break;

      case 'd':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        if (isConnected == 0)
        {
          printf("Please connect to a server\n");
          break;
        }

        ret = sscanf( &command[2], "%d", &delete_index);

        if (ret <= 0)
        {
          printf("Invalid delete_index.\n");
          break;
        }
        delete = malloc(sizeof(update));
        delete->type = DELETE_TYPE;
        delete->source = FROM_CLIENT;
        delete->update_index = delete_index;
        strncpy(delete->receiver, username, MAX_NAME_LEN);
        strncpy(delete->subject, private_group, MAX_GROUP_NAME);

        ret = SP_multicast(Mbox, AGREED_MESS, server_private_group, 1, sizeof(update),(char *)delete);
        if( ret <= 0 ) 
        {
          SP_error( ret );
          exit(1);
        }
        break;
        
      case 'r':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        if (isConnected == 0)
        {
          printf("Please connect to a server\n");
          break;
        }

        ret = sscanf(&command[2], "%d", &read_index);
        if (ret <= 0)
        {
          printf("Invalid read_index.\n");
          break;
        }
        read = malloc(sizeof(update));
        read->type = READ_TYPE;
        read->source = FROM_CLIENT;
        read->update_index = read_index;
        strncpy(read->receiver, username, MAX_NAME_LEN);
        strncpy(read->subject, private_group, MAX_GROUP_NAME);

        ret = SP_multicast(Mbox, AGREED_MESS, server_private_group,1,sizeof(update),(char *)read);
        if( ret <= 0 ) 
        {
          SP_error( ret );
          exit(1);
        }
        break;

      case 'v':

        if (isLoggedWithUser == 0)
        {
          printf("Please login with an username\n");
          break;
        }

        if (isConnected == 0)
        {
          printf("Please connect to a server\n");
          break;
        }

        view = malloc(sizeof(update));
        view->type = VIEW_TYPE;
        view->source = FROM_CLIENT;
        strncpy(view->receiver, username, MAX_NAME_LEN);
        strncpy(view->subject, private_group, MAX_GROUP_NAME);
        // printf("subject for view%s\n", view->subject);

        ret = SP_multicast(Mbox, AGREED_MESS, server_private_group, 1, sizeof(update),(char *)view);
        if( ret <= 0 ) 
        {
          SP_error( ret );
          exit(1);
        }
        break;

      default:
        Print_menu();
        break;
      }
      fflush(stdout);
  }

  static void Read_message(){

    int     ret;
    int     service_type;
    char    sender[MAX_GROUP_NAME];
    int     num_groups;
    char    target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    int16   mess_type;
    int     endian_mismatch;
    membership_info memb_info;
    int              num_vs_sets;
    vs_set_info      vssets[MAX_VSSETS];
    unsigned int     my_vsset_index;
    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
    int              i,j;

    

    service_type = 0;

    reply = malloc(sizeof(update));

    ret = SP_receive(Mbox, &service_type, sender, MAX_GROUPS, &num_groups, target_groups, &mess_type, &endian_mismatch, sizeof(update),(char *)reply);

    if (Is_regular_mess(service_type))
    {
     //printf("reply type%d\n", reply->type);

      if (reply->type == LIST_TYPE)
      {
        if (reply->read == 1)
        {
          strcpy(Is_read, "read");
        }else
        {
          strcpy(Is_read, "unread");
        }
        if (list_index == 1)
        {
          printf("%s's inbox:\n", username);
        }
        printf("%d -%s -From: %s -Subject: %s\n", list_index++, Is_read, reply->sender, reply->subject);
        printf("-------------------------------------------------------\n");
      }else if (reply->type == FINAL_TYPE)
      {
        printf("Toal %d mails in the inbox\n", list_index - 1);
        list_index = 1;
      }else if (reply->type == DELETE_TYPE)
      {
        printf("Delete the %d mail.\n", reply->update_index);
      }else if (reply->type == READ_TYPE)
      {
        printf("%d. :\n", reply->update_index);
        printf("From: %s\n", reply->sender);
        printf("Subject: %s\n", reply->subject);
        printf("%s\n", reply->msg);
        printf("--------------------------------------------------------\n");
      }else if (reply->type == VIEW_TYPE)
      {
        printf("membership: \n");
        for (i = 0; i < 5; i++)
        {
          if (reply->membership[i] != -1)
          {
            printf("server%d\n", reply->membership[i] );
          }
        }
      }
    }else if( Is_membership_mess( service_type ) )
    {
        ret = SP_get_memb_info((char *)reply, service_type, &memb_info );
        if (ret < 0) {
            printf("BUG: membership message does not have valid body\n");
            SP_error( ret );
            exit( 1 );
        }

        /* parse the membership message*/

        if( Is_reg_memb_mess( service_type ) )
        {
            printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                sender, num_groups, mess_type );
            
            for(int i=0; i < num_groups; i++ )
                printf("\t%s\n", &target_groups[i][0] );

            printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
            if (sender[0] == 'c' && num_groups == 1)
            {
              printf("[PLEASE CHANGE ANOTHRE SERVER, this one is in the diff partition.^.^]\n");
            }

            if( Is_caused_join_mess( service_type ) )
            {
                printf("Due to the JOIN of %s\n", memb_info.changed_member );
                printf( "\n================================\n");
            }else if( Is_caused_leave_mess( service_type ) ){
                printf("Due to the LEAVE of %s\n", memb_info.changed_member );
                printf( "\n================================\n");
            }else if( Is_caused_disconnect_mess( service_type ) ){
                printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
                printf("server_index: %d\n", server_index);
                if ((memb_info.changed_member[1] - '0') == server_index)
                {
                  printf("The connected server crashed, plz connect to a new one.\n");
                }
                printf( "\n================================\n");
            }else if( Is_caused_network_mess( service_type ) ){
                printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
                num_vs_sets = SP_get_vs_sets_info((char *)reply, &vssets[0], MAX_VSSETS, &my_vsset_index );
                if (num_vs_sets < 0) {
                    printf("BUG: membership message has more then %d vs sets. Recompile with larger MAX_VSSETS\n", MAX_VSSETS);
                    SP_error( num_vs_sets );
                    exit( 1 );
                }
                for(i = 0; i < num_vs_sets; i++ )
                {
                    printf("%s VS set %d has %u members:\n",
                        (i  == my_vsset_index) ?
                        ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
                    ret = SP_get_vs_set_members((char *)reply, &vssets[i], members, MAX_MEMBERS);
                    if (ret < 0) {
                        printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
                        SP_error( ret );
                        exit( 1 );
                    }
                    for( j = 0; j < vssets[i].num_members; j++ )
                        printf("\t%s\n", members[j] );
                }
            }
        }else if( Is_transition_mess(   service_type ) ) {
            printf("received TRANSITIONAL membership for group %s\n", sender );
        }else if( Is_caused_leave_mess( service_type ) ){
            printf("received membership message that left group %s\n", sender );
        }else 
          printf("received incorrecty membership message of type 0x%x\n", service_type );
    } else if ( Is_reject_mess( service_type ) )
    {
        printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
          sender, service_type, mess_type, endian_mismatch, num_groups, ret,(char *)reply );
    }else 
        printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

  }

static  void Usage(int argc, char const *argv[])
{
    sprintf( User, "hbqy" );
    sprintf( Spread_name, SPREAD_NAME);
}
static  void  Bye()
{

    printf("\nBye.\n");

    SP_disconnect( Mbox );

    exit( 0 );
}
static void Print_menu()
{
  printf( "\n================================\n");
  printf( "Client Usage: \n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
      "\t[-u <username>]: login with a username ",
      "\t[-c <server>]: connect to a specific server",
      "\t[-l]:      list the received mails",
      "\t[-m]:      mail a massgae",
      "\t[d <index>]:   delete that mail in the current list",
      "\t[r <index>]:   read that mail in the current list",
      "\t[v]:       print the membership of servers in the current network");
  printf("================================\n");
    fflush(stdout);
}


