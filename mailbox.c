#include<linux/kernel.h>
#include<linux/syscalls.h>
#include<linux/string.h>
#include<linux/slab.h>
#include<linux/seqlock.h>
#include<linux/mutex.h>
#include<linux/linkage.h>
#include<linux/errno.h>
#include<asm/uaccess.h>
#include"mbox.h"

unsigned long existing_mailboxes = 0;
struct mailbox_listing mailbox_list[256] = {{0, NULL}};
struct mutex lock;

DEFINE_MUTEX(lock);

/* ----------------------------------------------------- Helper Functions ------------------------------------------------------- */

long free_messages(struct message *msg){
	struct message *temp;
	while(msg->next!=NULL){
		temp = msg->next;
		kfree(msg);
		msg = temp;
	}
	kfree(msg);
return 0;
}

/* ------------------------------------------------------------------------------------------------------------------------------ */

asmlinkage long sys_mkMbox421(unsigned long mbxID){
	mutex_lock(&lock);
	if(!mailbox_list[mbxID].exists && mbxID < 256){
		if((mailbox_list[mbxID].mailbox_ptr = (struct mailbox*)kmalloc(sizeof(struct mailbox), GFP_KERNEL)) != NULL){
			mailbox_list[mbxID].mailbox_ptr->mxid = mbxID;
			mailbox_list[mbxID].mailbox_ptr->existing_messages = 0;
			mailbox_list[mbxID].mailbox_ptr->msg = NULL;
			mailbox_list[mbxID].mailbox_ptr->last_node = NULL;
			existing_mailboxes+=1;
			mailbox_list[mbxID].exists = 1;
			mutex_unlock(&lock);
			return 0;
		}
	}
mutex_unlock(&lock);
return -1;
}

asmlinkage long sys_rmMbox421(unsigned long mbxID){
	mutex_lock(&lock);
	if(mbxID<256 && mailbox_list[mbxID].exists && mailbox_list[mbxID].mailbox_ptr!=NULL){
		mailbox_list[mbxID].exists = 0;
		if(mailbox_list[mbxID].mailbox_ptr->msg!=NULL){
			if(!free_messages(mailbox_list[mbxID].mailbox_ptr->msg)){
				mailbox_list[mbxID].mailbox_ptr->last_node = NULL;
				mailbox_list[mbxID].mailbox_ptr->msg = NULL;
			}
			else{
				printk(KERN_ALERT "Mailbox messages are currupted\n");
				mutex_unlock(&lock);
				return -1;
			}
		}	
		mailbox_list[mbxID].mailbox_ptr->mxid = 0;		
		kfree(mailbox_list[mbxID].mailbox_ptr);
		mailbox_list[mbxID].mailbox_ptr = NULL;
		existing_mailboxes-=1;
		mutex_unlock(&lock);
		return 0;
	}
mutex_unlock(&lock);		
return -1;
}

asmlinkage long sys_countMbox421(void){
	return existing_mailboxes;
}

asmlinkage long sys_listMbox421(unsigned long *mbxList, unsigned long K){
	unsigned long i = 0,j = 0;
	mutex_lock(&lock);
	if(existing_mailboxes>=K){
		while(j<K){
			while(mailbox_list[i].exists==0){
				i++;
			}
			mbxList[j] = i;
			j++;
			i++;
		}
	mutex_unlock(&lock);
	return K;
	}
	else{
		if(!existing_mailboxes){
			mutex_unlock(&lock);
			return 0;
		}
		while(j<existing_mailboxes){
			while(mailbox_list[i].exists==0){
				i++;
			}
			mbxList[j] = i;
			j++;
			i++;
		}
	mutex_unlock(&lock);
	return existing_mailboxes;
	}
mutex_unlock(&lock);
return -1;
}

asmlinkage long sys_sendMsg421(unsigned long mbxID, char *msg, unsigned long N){
	mutex_lock(&lock);
	if(mailbox_list[mbxID].exists){
		if(!access_ok(VERIFY_READ, msg, N)){
				mutex_unlock(&lock);
				printk(KERN_ALERT "Invalid memory access");
				return -1;
		}

		if(mailbox_list[mbxID].mailbox_ptr->msg==NULL){
			if((mailbox_list[mbxID].mailbox_ptr->msg = (struct message*)kmalloc(sizeof(struct message), GFP_KERNEL))==NULL){
				mutex_unlock(&lock);
				printk(KERN_ALERT "Memory allocation to mailbox message failed in sendMsg421\n");
				return -1;
			}
			mailbox_list[mbxID].mailbox_ptr->msg->next = NULL;
			mailbox_list[mbxID].mailbox_ptr->last_node = mailbox_list[mbxID].mailbox_ptr->msg;
			if((mailbox_list[mbxID].mailbox_ptr->msg->msg = (char*)kmalloc(N*sizeof(char), GFP_KERNEL)) == NULL){
				mutex_unlock(&lock);
				printk(KERN_ALERT "Memory allocation to message failed in sendMsg421\n");
				return -1;
			}
			
			if(strncpy_from_user(mailbox_list[mbxID].mailbox_ptr->msg->msg, (const char*) msg, (long) N) == -EFAULT){
				printk(KERN_ALERT "ERROR while copying message to mailbox");
				mailbox_list[mbxID].mailbox_ptr->msg->size = strlen(mailbox_list[mbxID].mailbox_ptr->msg->msg);
				mutex_unlock(&lock);
				return strlen(mailbox_list[mbxID].mailbox_ptr->msg->msg);
			}
			mailbox_list[mbxID].mailbox_ptr->msg->size = N;
			mailbox_list[mbxID].mailbox_ptr->existing_messages+=1;
			mutex_unlock(&lock);
			return N;
		}
		if((mailbox_list[mbxID].mailbox_ptr->last_node->next = (struct message*)kmalloc(sizeof(struct message), GFP_KERNEL)) == NULL){
			mutex_unlock(&lock);
			printk(KERN_ALERT "Memory allocation to mailbox message failed while appending message in sendMsg421\n");
			return -1;
		}
		if((mailbox_list[mbxID].mailbox_ptr->last_node->next->msg = (char*)kmalloc(N*sizeof(char), GFP_KERNEL)) == NULL){
			mutex_unlock(&lock);
			printk(KERN_ALERT "Memory allocation while appending message failed in sendMSg421\n");
			return -1;
		}
		if(strncpy_from_user(mailbox_list[mbxID].mailbox_ptr->last_node->next->msg, (const char*)msg, (long)N) == -EFAULT){
			printk(KERN_ALERT "ERROR while copying message to mailbox at the end");
			mailbox_list[mbxID].mailbox_ptr->last_node->size = strlen(mailbox_list[mbxID].mailbox_ptr->last_node->next->msg);
			mutex_unlock(&lock);
			return mailbox_list[mbxID].mailbox_ptr->last_node->size;
		}
		mailbox_list[mbxID].mailbox_ptr->last_node->size = N;
		mailbox_list[mbxID].mailbox_ptr->last_node->next->next = NULL;
		mailbox_list[mbxID].mailbox_ptr->last_node = mailbox_list[mbxID].mailbox_ptr->last_node->next;
		mailbox_list[mbxID].mailbox_ptr->existing_messages+=1;
		mutex_unlock(&lock);
		return N;
	}
mutex_unlock(&lock);
return -1;		
}

asmlinkage long sys_receiveMsg421(unsigned long mbxID, char *msg, unsigned long N, unsigned char flag){
	unsigned long retval = 0, copy_failed = 0;
	mutex_lock(&lock);
	if(mailbox_list[mbxID].exists){
		if(!access_ok(VERIFY_WRITE, msg, N)){
				mutex_unlock(&lock);
				printk(KERN_ALERT "Invalid memory access");
				return -1;
		}
		if(mailbox_list[mbxID].mailbox_ptr->msg!=NULL){
			if(N>=mailbox_list[mbxID].mailbox_ptr->msg->size){
				if((copy_failed = copy_to_user((void*)msg, (void*)mailbox_list[mbxID].mailbox_ptr->msg->msg, mailbox_list[mbxID].mailbox_ptr->msg->size))){
					retval = mailbox_list[mbxID].mailbox_ptr->msg->size - copy_failed;
				 }
			}
			else{
				if((copy_failed = copy_to_user((void*)msg, (void*)mailbox_list[mbxID].mailbox_ptr->msg->msg, N-1))){
					retval = (N-1) - copy_failed;
					if(put_user(0, &msg[retval+1]) == -EFAULT){
						printk(KERN_ALERT "String not terminated properly while sending it to user");
					}
				 }
				retval = N;
				if(put_user(0,&msg[N]) == -EFAULT){
					printk(KERN_ALERT "String not terminated properly while sending it to user");
				}
			}
			if(flag){
				if(mailbox_list[mbxID].mailbox_ptr->msg->next==NULL){
					kfree(mailbox_list[mbxID].mailbox_ptr->msg);
					mailbox_list[mbxID].mailbox_ptr->msg = NULL;
					mailbox_list[mbxID].mailbox_ptr->last_node = NULL;
				}
				else{
					struct message *temp = mailbox_list[mbxID].mailbox_ptr->msg;
					mailbox_list[mbxID].mailbox_ptr->msg = mailbox_list[mbxID].mailbox_ptr->msg->next;
					kfree(temp);
				}
				mailbox_list[mbxID].mailbox_ptr->existing_messages-=1;
			}
			mutex_unlock(&lock);
			return retval;
		}
		mutex_unlock(&lock);
		return 0;
	}
mutex_unlock(&lock);
return -1;
}


asmlinkage long sys_countMsg421(unsigned long mbxID){
	return (long)(mailbox_list[mbxID].mailbox_ptr->existing_messages);
}
