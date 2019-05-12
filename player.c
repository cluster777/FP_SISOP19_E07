#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define SHMKEY 11037
#include <ao/ao.h>
#include <mpg123.h>

#define BITS 8
pthread_mutex_t play,fin;
int finish=0,Ssignal=0;
// A complete working C program to demonstrate all insertion methods 
#include <stdio.h> 
#include <stdlib.h> 

// A linked list node 
struct Node { 
	char data[100]; 
	struct Node* next; 
	struct Node* prev; 
}; 

/* Given a reference (pointer to pointer) to the head 
of a DLL and an int, appends a new node at the end */
void append(struct Node** head_ref, char*new_data) 
{ 
	/* 1. allocate node */
	struct Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 

	struct Node* last = *head_ref; /* used in step 5*/

	/* 2. put in the data */
	//new_node->data = new_data; 
	strcpy(new_node->data,new_data);
	/* 3. This new node is going to be the last node, so 
		make next of it as NULL*/
	new_node->next = NULL; 

	/* 4. If the Linked List is empty, then make the new 
		node as head */
	if (*head_ref == NULL) { 
		new_node->prev = NULL; 
		*head_ref = new_node; 
		return; 
	} 

	/* 5. Else traverse till the last node */
	while (last->next != NULL) 
		last = last->next; 

	/* 6. Change the next of last node */
	last->next = new_node; 

	/* 7. Make last node as previous of new node */
	new_node->prev = last; 

	return; 
} 


void* player(void* input)
{
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;
	printf("playing %s\n",(char*)input);
	char path[200]="/root/mounted/";
	strcat(path,(char*)input);
    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	//printf("buka file\n");
    /* open the file and get the decoding format */
    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    /* decode and play */
    while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK &&Ssignal==0){
			usleep(68000);//printf("work\n");
			//ini gak jelas banget framerate nya overated jadi begini
			ao_play(dev, buffer, done);
			pthread_mutex_lock(&play);
			pthread_mutex_unlock(&play);
		}

    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
	

}
int main(){
	pthread_t tid[3];
	//printf("ini buat thread\n");
	struct Node* list=NULL;
	char input[100],ch;
	system("ls /root/mounted>/root/listlagu");
	FILE * infile;
	infile=fopen("/root/listlagu","r");
	//mendapatkan list lagu dalam list
	while(fscanf(infile,"%[^\t\n]%*c",&input,&ch)!=EOF){
		//kalo dibuat disini entah kok jadi korupted 
		//sprintf(input,"/root/mounted/%s",input);
		printf("%s\n",input);
		append(&list,input);
	}
	struct Node* front=list;
	//buat cycle
	while(list->next!=NULL)list=list->next;
	list->next=front;
	front->prev=list;
	list=list->next;
	
	pthread_create(&(tid[0]),NULL,&player,list->data);
	while(1){
		pthread_mutex_lock(&fin);
		if(finish==1){
			pthread_create(&(tid[0]),NULL,&player,list->data);
			finish=0;
		}
		pthread_mutex_unlock(&fin);
		int in;
		scanf("%d",&in);
		if(in==1)//play
			pthread_mutex_unlock(&play);
		if(in==2)//pause
			pthread_mutex_lock(&play);
		if(in==3){//next
			//pthread_cancel(tid[0]);tidak boleh langsung cancel;
			Ssignal=1;//woy berhenti
			pthread_mutex_unlock(&play);
			pthread_join(tid[0],NULL);
			Ssignal=0;//nah ok
			if(list->next!=NULL)list=list->next;
			//pthread_mutex_unlock(&play);
			pthread_create(&(tid[0]),NULL,&player,list->data);
		}
		if(in==4){//prev
			//pthread_cancel(tid[0]);
			Ssignal=1;//woy berhenti
			pthread_mutex_unlock(&play);
			pthread_join(tid[0],NULL);
			Ssignal=0;//nah ok
			if(list->prev!=NULL)list=list->prev;
			//pthread_mutex_unlock(&play);
			pthread_create(&(tid[0]),NULL,&player,list->data);
		}
		if(in==5){//find and play
			printf("mau namanya apa--pakai ext>>");
			scanf("%s",&input);
			if(strcmp(list->data,input)==0){
				printf("lagi main nih\n");
				continue;
			}
			printf("mencari...\n");
			front=list->next;
			while(front!=list){
				if(strcmp(front->data,input)==0){
					printf("ganti lagu\n");
					Ssignal=1;//woy berhenti
					pthread_mutex_unlock(&play);
					pthread_join(tid[0],NULL);
					Ssignal=0;//nah ok
					pthread_create(&(tid[0]),NULL,&player,front->data);
					list=front;
					break;
				}
				front=front->next;
			}
			if(strcmp(front->data,input)!=0)printf("tidak ketemu :(\n");
		}
		if(in==6){//listing ulang
			front=list;
			printf("ini nama yang di list:\n%s\n",front->data);
			front=front->next;
			while(front!=list){
				printf("%s\n",front->data);
				front=front->next;
			}
			
		}
		if(in==7){ //i quit
			Ssignal=1;//woy berhenti
			pthread_mutex_unlock(&play);
			break;
		}
	}
	pthread_join(tid[0],NULL);
	return 0;
}
