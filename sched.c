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
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};

struct message_que_2
{
    long mtype;
    char mtext[MSGSIZE];
};

#define MSGSZ1 sizeof(struct message_que_1)-sizeof(long)
#define MSGSZ2 sizeof(struct message_que_2)-sizeof(long)


int main(int argc, char *argv[])
{
	int MQ1key,MQ2key,k,cnt = 0, masterPID;
	union sigval value;

	if(argc < 5)
	{
		printf("USAGE: %s MasterPID k MQ1KEY MQ2KEY\n",argv[0]);
		exit(1);
	}

	masterPID = atoi(argv[1]);
	k = atoi(argv[2]);
	MQ1key = atoi(argv[3]);
	MQ2key = atoi(argv[4]);

	// Get MQ1 -- Ready Queue
	int MQ1 = msgget(MQ1key,IPC_CREAT|0666);
	if(MQ1 < 0) printf("Message Get Error in MQ1\n");

	// Get MQ2 -- For Communication between Scheduler and MMu
	int MQ2 = msgget(MQ2key,IPC_CREAT|0666);
	if(MQ2 < 0) printf("Message Get Error in MQ2\n");

	struct message_que_1 MQ1Send,MQ1Rcv;
	struct message_que_2 MQ2Send,MQ2Rcv;


	while(1)
	{
		// Scan the Ready Queue and select a process for scheduling
		if(msgrcv(MQ1,&MQ1Rcv,MSGSZ1,0,0) < 0) printf("msgrcv Error\n");
        else printf("Process Selected For Scheduling: %d\n",MQ1Rcv.pid);

        // Send NOTIFY Signal to the Process
        if(sigqueue(MQ1Rcv.pid,SIGUSR1,value) != 0) perror("NOTIFY signal could not be sent\n");

        // Wait for Notification from MMU
        if(msgrcv(MQ2,&MQ2Rcv,MSGSZ2,0,0) < 0) printf("msgrcv Error\n");

        if(MQ2Rcv.mtype == 1)
        {
			printf("PAGEFAULT HANDLED FOR PROCESS: %d Enqueing it to the Ready Queue\n",MQ1Rcv.pid);
			MQ1Send.mtype = getpid();
			MQ1Send.pid = MQ1Rcv.pid;
			if(msgsnd(MQ1,&MQ1Send,MSGSZ1,0) < 0) printf("msgsnd Error\n");     	

        }

        else if(MQ2Rcv.mtype == 2)
        {
        	printf("PROCESS TERMINATED: %d\n",MQ1Rcv.pid);
        	//if(sigqueue(MQ1Rcv.pid,SIGUSR2,value) != 0) printf("KILL signal could not be sent to Process %d\n",MQ1Rcv.pid);
        	//printf("PROCESS TERMINATED sent to: %d\n",MQ1Rcv.pid);
        	cnt++;
        	if(cnt == k)
        	{
        		// Notify Master
        		printf("ALL PROCESS TERMINATED....Notifying Master\n");
        		if(sigqueue(masterPID,SIGUSR1,value) != 0) printf("NOTIFY signal could not be sent\n");
        	}
        }
	}

	return 0;
}