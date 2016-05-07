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
#define MSGSZ sizeof(struct message_que_1)-sizeof(long)

typedef struct PageTable{
	pid_t pid;
	int timestamp[MAX];
	int frame_no[MAX];
	int valid[MAX];
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

// Reference String -- Space Separated

struct message_que_1
{
    long mtype; // For MMU to Process (pid), For Process to MMU (MMU_Type)
    pid_t pid; 
    int response; // MMU to Process -> 0-Frame Number (Move to Next Frame Continue), -1 (Page Fault -- Pause() -- Wait for scheduler, -2 -- Invalid Page Reference , Terminate) ,
    				// Process to MMU ->Send -9 for Termination
    char mtext[MSGSIZE];
};
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


char * strg(int id)
{

	char * ss = (char*)malloc(100*sizeof(char));
	memset(ss,100,'\0');
	sprintf(ss,"%d",id);
	return ss;
}

void Termination(int signum,siginfo_t * info, void * old)
{
    printf("Received Scheduler's Termination Message\n");
}

void handle_term(int key)
{
	M = (Mem*)shmat(key,NULL,0);
	printf("Received KILL from master\n");
	FILE *result = fopen("result.txt","w");

	fprintf(result,"Page Faults: ");
	int i;
	for(i=0;i<M->pf_cnt;++i)
	{
		fprintf(result,"(%d, %d), ",M->PF[i].p,M->PF[i].x);
	}

	fprintf(result,"\nPage Fault Counts: ");
	for(i=0;i<M->pr_cnt;++i)
	{
		fprintf(result,"(%d, %d), ",M->PF_C[i].p,M->PF_C[i].c);
	}

	fprintf(result,"\nInvalid Page References: ");
	for(i=0;i<M->inv_cnt;++i)
	{
		fprintf(result,"(%d, %d), ",M->IV[i].p,M->IV[i].x);
	}

	fprintf(result,"\nInvalid Page Reference Counts: ");
	for(i=0;i<M->pr_cnt;++i)
	{
		fprintf(result,"(%d, %d), ",M->IV_C[i].p,M->IV_C[i].c);
	}

	fprintf(result,"\nGlobal Ordering: ");
	for(i=0;i<M->gl_cnt;++i)
	{
		fprintf(result,"(%d, %d, %d), ",M->GL[i].t,M->GL[i].p,M->GL[i].x);
	}

	fclose(result);
	shmdt((char *)M);
	//exit(1);
}




int main(int argc, char * argv[])
{
	int k,m,f;

	if(argc<5)
	{
		printf("USAGE: %s k m f Key(Any random number>2000)\n",argv[0]);
		exit(1);
	}
	k = atoi(argv[1]);
	m = atoi(argv[2]);
	f = atoi(argv[3]);

	int sm1 = atoi(argv[4]);
	int sm2 = sm1+1;
	int mq1 = sm2+1;
	int mq2 = mq1+1;
	int mq3 = mq2+1;

	int master_id = getpid();

	struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    act1.sa_sigaction = &Termination;
	if (sigaction(SIGUSR1, &act1, NULL) == -1)
    {
        perror("sigusr1: sigaction");
        return 0;
    }

	int SM1 = shmget((key_t)sm1,sizeof(PageTableList),IPC_CREAT|0666);
	if(SM1<0)printf("SM1 error\n");
	int SM2 = shmget((key_t)sm2,sizeof(freeFrames),IPC_CREAT|0666);
	if(SM2<0)printf("SM2 error\n");

	int SM3 = shmget((key_t)sm2+5,sizeof(Mem),IPC_CREAT|0666);

	int MQ1 = msgget(mq1,IPC_CREAT|0666);
	if(MQ1 < 0) printf("Message Get Error in MQ1\n");

	int MQ2 = msgget(mq2,IPC_CREAT|0666);
	if(MQ2 < 0) printf("Message Get Error in MQ2\n");

	int MQ3 = msgget(mq3,IPC_CREAT|0666);
	if(MQ3 < 0) printf("Message Get Error in MQ3\n");

	PTL = (PageTableList*)shmat(SM1,NULL,0);
	PTL->num_process = k;
	PTL->size_VS = m;


	FF = (freeFrames*)shmat(SM2,NULL,0);
	FF->size_M = f;

	int i,j,p_i,m_i, mm_i,v_i,jj;
	for(i = 0;i<FF->size_M;i++)
		FF->frames[i] =  0;

	int sched_pid = fork();
	if(sched_pid == 0)
	{
		execlp("xterm","xterm" ,"-hold","-e","./scheduler",strg(master_id),strg(k),strg(mq1),strg(mq2),(const char *)NULL);
		exit(1);
	}

	int MMU_id = fork();
	if(MMU_id == 0)
	{
		execlp("xterm","xterm" ,"-hold","-e","./MMU",strg(mq2),strg(mq3),strg(sm1),strg(sm2),(const char * )NULL);
		exit(1);
	}
	char str[100];
	memset(str,100,'\0');
	int pid[MAX];

	M = (Mem*)shmat(SM3,NULL,0);

	M->pr_cnt = k;
	
	for(i=0;i<k;i++)
	{

		//printf("Loop: %d\n",i);
		pid[i]  = fork();
		if(pid[i] ==0)
		{
			M->IV_C[i].p = getpid();
			M->IV_C[i].c = 0;
			M->PF_C[i].p = getpid();
			M->PF_C[i].c = 0;
			//printf("Process %d Created\n",getpid());
			PTL = (PageTableList*)shmat(SM1,NULL,0);
			srand(i);
			m_i = rand()%m;
			mm_i = 2*m_i + rand()%(8*m_i); 
			
			printf("Process %d mm_i %d\n",getpid(),mm_i);
			strcpy(str,"");

			for( j = 0; j< mm_i;j++)
			{
				//printf("Loop1: %d\n",i);
				v_i = rand()%(m_i + (int)1*m_i); 
				strcat(str,strg(v_i));
				strcat(str," ");
			}
			// strcat(str,strg(-1));
			// strcat(str," ");

			PTL->PT[i].max_num_frames = m_i;
			PTL->PT[i].pid = getpid();
			for(jj = 0;jj<m;jj++)
			{
				//printf("Loop 2: %d\n",i);
				
				PTL->PT[i].valid[jj] = 0;
				PTL->PT[i].frame_no[jj] = -1;
				PTL->PT[i].timestamp[jj] = 0;

			}
			
			shmdt((char*)PTL);
			printf("Process %d: Reference String %s\n",getpid(),str);
			execlp("./process","./process",str,strg(mq1),strg(mq3),(const char *)NULL);
			exit(1);

		}
		usleep(25000);
	}
	//usleep(25000);

	

	shmdt((char*)PTL);
	shmdt((char*)FF);
	shmdt((char*)M);

	pause();

	union sigval value;
	handle_term(SM3);
	//if(sigqueue(MMU_id,SIGUSR2,value) != 0) printf("KILL signal could not be sent to Process %d\n",MMU_id);
	int status = 0;
	kill(MMU_id,SIGKILL);
	//waitpid(MMU_id,&status,0);
	kill(sched_pid,SIGKILL);
	printf("Scheduler Terminated\nMMU Terminated\n");
	printf("Terminating Master\n");
	return 0;
}