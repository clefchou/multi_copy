#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>

#define NFORK 10
#define BLOCKSIZE 2048
#define headlong 4
#define MYMODE S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH

int outfd,infd;
int pfd[2];


struct data{
	int ioffset;
	int ilen;
	char buf[BLOCKSIZE];
};

void fork_w(){
int fork_num;
int n = 0;
struct data *tempdataptr;
close(pfd[1]);
for(;;){
	
	
	tempdataptr = malloc(sizeof(struct data));
	
	if((n = read(pfd[0],tempdataptr,sizeof(struct data))) < 0){
		printf("read pipe error\n");
	}
	
	else if (n == 0){	
		printf("write complete\n");
		break;
	}
	else{
		pwrite(outfd,tempdataptr->buf,tempdataptr->ilen,tempdataptr->ioffset);
		free(tempdataptr);
		fork_num = tempdataptr->ioffset;
		
		//printf("%d write succeed\n",tempdataptr->ioffset);
	}
	
}
}

void fork_r(int i){
int fork_num = i;
int n = 0;
struct data *tempdataptr;
close(pfd[0]);
for(;;){
	
	tempdataptr = malloc(sizeof(struct data));
	if((n = pread(infd,tempdataptr->buf,BLOCKSIZE,BLOCKSIZE*fork_num)) < 0)
		printf("read infd error\n");
	else if (n == 0){
		printf("%d\n",BLOCKSIZE*(fork_num));
		free(tempdataptr);
		printf("read %d complete\n",i);
		break;
	}
	else{
		tempdataptr->ioffset = BLOCKSIZE*fork_num;
		tempdataptr->ilen = n;
		write(pfd[1],tempdataptr,sizeof(struct data));
	//	printf("%d read succeed\n",tempdataptr->ioffset);
		fork_num += NFORK/2;
	}
	
}
}

int main(int argc,char *argv[]){

clock_t start_time,finish_time;
double total_time;
start_time = clock();

int i,err;
//pid_t wpid[NFORK];
//pid_t rpid[NFORK];
//int fork_state_w[NFORK],fork_state_r[NFORK];
int fork_num,fork_state[NFORK],status[NFORK];

if(argc != 3){
	printf("usage:./task4.out src dst \n");
	return -1;
}
outfd = open(argv[2],O_WRONLY |O_CREAT |O_TRUNC,MYMODE);
if(outfd < 0){
	printf("create outfile.txt failed!\n");
	return -1;
}
infd = open(argv[1],O_RDONLY);
if(infd < 0){
	printf("open infile.txt error!\n");
	return -1;
}
if(pipe(pfd)<0){
	printf("create pipe error!\n");
	return -1;
}


for(i = 0; i < NFORK; i++){
	status[i] = fork();
	fork_num = i;	//记录进程数
	if(status[i] == 0 || status[i] == -1) break;	//确保只有父进程创建子进程
	else
		fork_num = -1;	
}
if(fork_num < NFORK/2 && fork_num >= 0)
	fork_r(fork_num);
if(fork_num >= NFORK/2 && fork_num >= 0)
	fork_w();
if(fork_num < 0){ //父进程等待
	for(i = 0; i < NFORK/2; i++){
		waitpid(status[i],&fork_state[i],0);
	}//判断写入进程完成
	close(pfd[1]);
	printf("pipe1 closed\n");
		
	for(;i < NFORK; i++){
		//close(pfd[1]);
		waitpid(status[i],&fork_state[i],0);
	}
}
close(infd);
close(outfd);
close(pfd[0]);

finish_time = clock();
total_time = ((double)(finish_time - start_time))/CLOCKS_PER_SEC;
printf("%f seconds\n",total_time);
return 0;

}
