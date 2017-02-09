# Distributed-Mail-Service
A reliable and efficient mail service for users over a  network of computers. The service comprise a mail server and
a mail client programs.

The service:
------------
Every user can connect via a mail client to any one of the replicated servers. 

The mail service provides the following capabilities; each capability
is one option of a client program's top level menu:

1. Login with a user name. Example: "u Sheryl". 

2. Connecting to a specific mail server. Example: "c 3" will connect to server 3.

3. List the headers of received mail: "l"
   Result: 
	The result displays the user name, the responding mail server index and 
	a list of messages. The messages in the list will be numbered (1, 2, 3, ...) 
    and the sender and subject of each message will be displayed.

4. Mail a message to a user: "m"

   	to:  (user name)
    
    subject:  (subject string)
    
    (msg string)         

5. Delete a mail message (read or unread): Example "d 10" will delete the
   10th message in the current list of messages.

6. Read a received mail message (read or unread): Example "r 5" will display
   the content of the 5th message in the current list of messages. 

7. Print the membership (identities) of the mail servers in the current mail server's 
   network component. Example "v".
