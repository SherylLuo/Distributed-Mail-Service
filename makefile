CC=gcc
LD=gcc
CFLAGS=-g -Wall -Wno-unused-function
CPPFLAGS=-I. -I include
SP_LIBRARY_DIR=include

all: server client

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

server:  $(SP_LIBRARY_DIR)/libspread-core.a server.o mail_list.o userlist.o
	$(LD) -o $@ server.o mail_list.o userlist.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

client:  $(SP_LIBRARY_DIR)/libspread-core.a client.o
	$(LD) -o $@ client.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean:
	rm -f *.o server client
	rm -rf backups1/*
	rm -rf backups2/*
	rm -rf backups3/*
	rm -rf backups4/*
	rm -rf backups5/*
