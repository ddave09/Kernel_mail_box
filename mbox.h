#ifndef __MBOX
#define __MBOX
#define MAX_MAILBOX 256

extern unsigned long existing_mailboxes;

struct mailbox_listing{
	unsigned char exists;
	struct mailbox *mailbox_ptr;
} extern mailbox_list[256];


struct message{
	unsigned long size;
	char *msg;
	struct message *next;
};

struct mailbox{
	unsigned long existing_messages;
	unsigned long mxid;
	struct message *msg;
	struct message *last_node;
};

#endif







