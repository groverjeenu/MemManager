// Assignment 8

// Group Details
// Group No: 22
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)


#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>

#define MAX 10000
#define MAX_FRAME 1000000
#define MMU_TYPE 2000
#define MSGSIZE 10

struct message_que_1
{
    long mtype; // For MMU to Process (pid), For Process to MMU (MMU_Type)
    pid_t pid; 
    int response; // MMU to Process -> 0-Frame Number (Move to Next Frame Continue), -1 (Page Fault -- Pause() -- Wait for scheduler, -2 -- Invalid Page Reference , Terminate) ,
    				// Process to MMU ->Send -9 for Termination
    char mtext[MSGSIZE];
};

struct message_que_11
{
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};

#define MSGSZ sizeof(struct message_que_1)-sizeof(long)
#define MSGSZ1 sizeof(struct message_que_11)-sizeof(long)

void Termination(int signum,siginfo_t * info, void * old)
{
    printf("Process %d: Notified by scheduler\n",getpid());
    return;
}

void handle_term(int signum,siginfo_t * info, void * old)
{
	printf("Process %d: Invalid Page Referenced...Exiting\n",getpid());
	exit(1);
}



int main(int argc, char *argv[])
{
	char * str = (char*)malloc(1000*sizeof(char));
	strcpy(str,argv[1]);

	int MQ1 = msgget(atoi(argv[2]),IPC_CREAT|0666);
	if(MQ1 < 0) printf("Message Get Error in MQ1\n");

	

	int MQ3 = msgget(atoi(argv[3]),IPC_CREAT|0666);
	if(MQ3 < 0) printf("Message Get Error in MQ3\n");

	int i = 0,l=0,m=0,j,k,val[MAX];
	printf("Scanning Started\n");

	int len = strlen(str);
	char temp[MAX];

	struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &Termination;
    //usleep(500000);
	if (sigaction(SIGUSR1, &act1, NULL) == -1)
    {
        perror("sigusr1: sigaction");
        return 0;
    }

    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    act2.sa_sigaction = &handle_term;
    //usleep(500000);
	if (sigaction(SIGUSR2, &act2, NULL) == -1)
    {
        perror("sigusr2: sigaction");
        return 0;
    }

	while(i<len)
	{
		if(str[i] == ' '){
			temp[m] = '\0';
			val[l] = atoi(temp);
			l++;
			temp[0] = '\0';
			m=0;
			i++;

		}
		temp[m++] = str[i];
		i++;
	}

	/*sscanf(str,"%d",&val[i]);

	while(val[i] != -1)
	{
		printf("Scanned %d\n",val[i]);
		i++;
		sscanf(str,"%d",&val[i]);
	}*/

	//printf("Scanning Done\n");

	struct message_que_11 MQ1Send;
	int mypid = getpid();
	MQ1Send.mtype = mypid;
	MQ1Send.pid = mypid;
	if(msgsnd(MQ1,&MQ1Send,MSGSZ1,0) < 0) printf("msgsnd Error\n");

	//printf("Inserted in Ready Queur \n");

	int num = i;

	int num_done = 0;
	struct message_que_1 msgSend,msgRcv;

	pause();
	

	while(num_done < num)
	{
		strcpy(msgSend.mtext,"");
		strcpy(msgSend.mtext,"Message");
		msgSend.mtype = MMU_TYPE;
		msgSend.pid = mypid;
		msgSend.response = val[num_done];
		if(msgsnd(MQ3,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");
		printf("Process %d: Page Sent: %d\n",getpid(),val[num_done]);

		if(msgrcv(MQ3,&msgRcv,MSGSZ,mypid,0) <0 ) printf("Proces %d: msgrcv Error\n",getpid());
		//else printf("Message Receved 1\n");

		if(msgRcv.response >= 0)
		{
			printf("Process %d: Valid Page\n",getpid());
			num_done++;
		}
		
		else if(msgRcv.response == -2)
		{
			printf("Process %d: Invalid Page Reference...Terminating...\n",getpid());
			exit(1);
		}
		else if(msgRcv.response == -1)
		{
			printf("Process %d: Page Fault\n",getpid());
			pause();
		}


	}	

	strcpy(msgSend.mtext,"");
	strcpy(msgSend.mtext,"Message");
	msgSend.mtype = MMU_TYPE;
	msgSend.pid = mypid;
	msgSend.response = -9;
	if(msgsnd(MQ3,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");	
	printf("Process %d: All pages refernced. Program exiting normally\n",getpid());





	return 0;
}