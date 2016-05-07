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
//#include <sys/types.h>

#define MAX 10000
#define MAX_FRAME 1000000

#define MMU_Type 2000
#define MSGSIZE 10
#define MAX_INT 1000000000

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

struct message_que_3
{
    long mtype; // For MMU to Process (pid), For Process to MMU (MMU_Type)
    pid_t pid; 
    int response; // MMU to Process -> 0-Frame Number (Move to Next Frame Continue), -1 (Page Fault -- Pause() -- Wait for scheduler, -2 -- Invalid Page Reference , Terminate) ,
    				// Process to MMU -> Send -9 for Termination, Else Page_Number
    char mtext[MSGSIZE];
};

#define MSGSZ2 sizeof(struct message_que_2)-sizeof(long)
#define MSGSZ3 sizeof(struct message_que_3)-sizeof(long)

typedef struct PageTable{
	pid_t pid;
	int timestamp[MAX];
	int frame_no[MAX];
	int valid[MAX];	// Vaid - 1, Invalid - 0
	int max_num_frames;
}PageTable;

typedef struct PageTableList{
 	int num_process;
 	int size_VS;
 	PageTable PT[MAX];
}PageTableList;

PageTableList * PTL;

typedef struct freeFrames
{
	int size_M;
	int frames[MAX_FRAME];
}freeFrames;

freeFrames *FF;

typedef struct PageFault_Cnt{
	int p;
	int c;
}PageFault_Cnt;

typedef struct Invalid_Cnt{
	int p;
	int c;
}Invalid_Cnt;

typedef struct PageFault{
	int p;
	int x;
}PageFault;

typedef struct Invalid{
	int p;
	int x;
}Invalid;

typedef struct Global{
	int t;
	int p;
	int x;
}Global;

typedef struct memPrint{

int pf_cnt,inv_cnt, gl_cnt,pr_cnt;

PageFault PF[MAX];
Invalid IV[MAX];
PageFault_Cnt PF_C[MAX];
Invalid_Cnt IV_C[MAX];
Global GL[MAX];


}Mem;
Mem * M;


int global_time = 0;


int main(int argc, char *argv[])
{
	int MQ2key,MQ3key,SM1key,SM2key,i,k,j,l;

	struct message_que_2 MQ2Send,MQ2Rcv;
	struct message_que_3 MQ3Send,MQ3Rcv;

	if(argc < 5)
	{
		printf("USAGE: %s MQ2KEY MQ3KEY SM1key SM2key\n",argv[0]);
		exit(1);
	}

	MQ2key = atoi(argv[1]);
	MQ3key = atoi(argv[2]);
	SM1key = atoi(argv[3]);
	SM2key = atoi(argv[4]);

	// FreeFrames = (int *)malloc(MAX_FRAME*sizeof(int));

	// Get MQ2 -- For Communication between Scheduler and MMU
	int MQ2 = msgget(MQ2key,IPC_CREAT|0666);
	if(MQ2 < 0) printf("Message Get Error in MQ2\n");

	// Get MQ3 -- For Communication between processes and MMU
	int MQ3 = msgget(MQ3key,IPC_CREAT|0666);
	if(MQ3 < 0) printf("Message Get Error in MQ3\n");

	int SM1 = shmget((key_t)SM1key,sizeof(PageTableList),IPC_CREAT|0666);
	int SM2 = shmget((key_t)SM2key,sizeof(freeFrames),IPC_CREAT|0666);
	int SM3 = shmget((key_t)SM2key+5,sizeof(Mem),IPC_CREAT|0666);
	//int SM4 = shmget((key_t)SM2key+6,sizeof(freeFrames),IPC_CREAT|0666);
	//int SM5 = shmget((key_t)SM2key+7,sizeof(freeFrames),IPC_CREAT|0666);

	M = (Mem*)shmat(SM3,NULL,0);

	M->gl_cnt = 0;
	M->pf_cnt = 0;
	M->inv_cnt = 0;



	while(1)
	{
		global_time++;

		if(msgrcv(MQ3,&MQ3Rcv,MSGSZ3,MMU_Type,0) < 0) printf("msgrcv Error\n");
        else printf("New Request: (%d,%d)\n",MQ3Rcv.pid,MQ3Rcv.response);

        

        int curr_pid = MQ3Rcv.pid;
        int page_no = MQ3Rcv.response;
        PTL = (PageTableList *)shmat(SM1,NULL,0);
        int num_p = PTL->num_process;
        int VS = PTL->size_VS;

        //printf("Num OF Process: %d\n",num_p);

        for(i=0;i<num_p;++i)
        {
        	//printf("Table %d PID: %d",i,PTL->PT[i].pid);
        	if(PTL->PT[i].pid == curr_pid)
        	{
        		k = i;
        		break;
        	}
        }

        if(i==num_p)
        {
        	printf("Invalid PID\n");
        	shmdt((char *)PTL);
        	continue;
        }

        if(MQ3Rcv.response == -9)
        {
        	// Inform Scheduler
        	MQ2Send.mtype = 2;
			if(msgsnd(MQ2,&MQ2Send,MSGSZ2,0) < 0) printf("msgsnd Error\n");

			// Update Free Frame List

			FF = (freeFrames *)shmat(SM2,NULL,0);
			for(i=0;i<VS;++i)
			{
				if(PTL->PT[k].valid[i] == 1){
					FF->frames[PTL->PT[k].frame_no[i]] = 0;
				}
			}


			shmdt((char *)FF);
			shmdt((char *)PTL);
			continue;
        }

        M->GL[M->gl_cnt].t = global_time;
        M->GL[M->gl_cnt].p = MQ3Rcv.pid;
        M->GL[M->gl_cnt++].x = MQ3Rcv.response;

        int frame_num = PTL->PT[k].frame_no[page_no];
        int is_valid = PTL->PT[k].valid[page_no];

        if(page_no >= PTL->PT[k].max_num_frames)
        {
        	// Illegal Frame Reference
        	int ii;
        	for(ii=0;ii<(M->pr_cnt);++ii)
        	{
        		if(M->IV_C[ii].p == curr_pid)
        		{
        			(M->IV_C[ii].c)++;
        			break;
        		}
        	}

	        M->IV[M->inv_cnt].p = MQ3Rcv.pid;
	        M->IV[M->inv_cnt++].x = MQ3Rcv.response;

        	printf("Process TRYING TO ACCESS INVALID PAGE REFERENCE: %d\n",curr_pid);
        	MQ3Send.mtype = curr_pid;
        	MQ3Send.response = -2;
			if(msgsnd(MQ3,&MQ3Send,MSGSZ3,0) < 0) printf("msgsnd Error\n");

			usleep(100);

			// Inform Scheduler
        	MQ2Send.mtype = 2;
			if(msgsnd(MQ2,&MQ2Send,MSGSZ2,0) < 0) printf("msgsnd Error\n");

			FF = (freeFrames *)shmat(SM2,NULL,0);
			for(i=0;i<VS;++i)
			{
				if(PTL->PT[k].valid[i] == 1){
					FF->frames[PTL->PT[k].frame_no[i]] = 0;
				}
			}


			shmdt((char *)FF);
			shmdt((char *)PTL);
			continue;
        }

        if(is_valid == 0)
        {
        	// Page Fault
        	M->PF[M->pf_cnt].p = MQ3Rcv.pid;
	        M->PF[M->pf_cnt++].x = MQ3Rcv.response;

	        int ii;
        	for(ii=0;ii<(M->pr_cnt);++ii)
        	{
        		if(M->PF_C[ii].p == curr_pid)
        		{
        			(M->PF_C[ii].c)++;
        			break;
        		}
        	}

        	printf("PAGE FAULT: %d\n",curr_pid);

        	FF = (freeFrames *)shmat(SM2,NULL,0);

        	

			// Check if Free Frames are Available

			for(j=0;j<(FF->size_M);++j)
			{
				if(FF->frames[j] == 0)
				{
					// Frame is free
					FF->frames[j] = 1;
					PTL->PT[k].valid[page_no] = 1;
					PTL->PT[k].timestamp[page_no] = global_time;
				}
			}

			int to_replace = -1;

			if(j == (FF->size_M))
			{
				// No Free Frames -- LRU
				int min_time = MAX_INT;
				for(l = 0;l<VS; ++l)
				{
					if(PTL->PT[k].timestamp[l] < min_time)
					{
						min_time = PTL->PT[k].timestamp[l];
						to_replace = l;
					}
				}
				PTL->PT[k].valid[to_replace] = 0;			
				PTL->PT[k].valid[page_no] = 1;
				PTL->PT[k].frame_no[page_no] = PTL->PT[k].frame_no[l];
				PTL->PT[k].timestamp[page_no] = global_time;
			}

        	usleep(500000);

        	MQ3Send.mtype = curr_pid;
        	MQ3Send.response = -1;
			if(msgsnd(MQ3,&MQ3Send,MSGSZ3,0) < 0) printf("msgsnd Error\n");

			usleep(100);

			MQ2Send.mtype = 1;
			if(msgsnd(MQ2,&MQ2Send,MSGSZ2,0) < 0) printf("msgsnd Error\n");

        	shmdt((char *)PTL);
        	shmdt((char *)FF);
        	continue;
        }

        PTL->PT[k].timestamp[page_no] = global_time;
        MQ3Send.mtype = curr_pid;
    	MQ3Send.response = frame_num;
		if(msgsnd(MQ3,&MQ3Send,MSGSZ3,0) < 0) printf("msgsnd Error\n");
		shmdt((char *)PTL);
	}
	shmdt((char *)M);

	return 0;
}