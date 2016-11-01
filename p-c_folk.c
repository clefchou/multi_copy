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

/*定义传输数据结构data struct
 *ioffset->文件偏移量
 *ilen->文件长度
 *buf->存储文件数据
 */
struct data{
	int ioffset;
	int ilen;
	char buf[BLOCKSIZE];
};
/*写入进程执行函数*/
void fork_w(){
int fork_num;
int n = 0;
struct data *tempdataptr;
close(pfd[1]);//关闭写入进程的管道写端口，如果没关read中将会被阻塞
for(;;){
	
	
	tempdataptr = malloc(sizeof(struct data));
	 /*从管道中读取整个struct出来*/
	if((n = read(pfd[0],tempdataptr,sizeof(struct data))) < 0){
		printf("read pipe error\n");
	}
	
	else if (n == 0){
	/*如果读取的字段长度为0，则说明文件已全部读完，管道被关闭，进程结束*/
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
/*读取进程执行函数*/
void fork_r(int i){
int fork_num = i;
int n = 0;
struct data *tempdataptr;
close(pfd[0]);//关闭读取进程的管道读端口
for(;;){
	
	tempdataptr = malloc(sizeof(struct data));
	/*从文件中读取整个struct并将其写入管道
	 *进程的编号fork_num，读取数据时根据编号分段读取
	 *例如有5个进程，则0号进程读0,5,10段，1号读1,6,11段，以此类推
	 */
	if((n = pread(infd,tempdataptr->buf,BLOCKSIZE,BLOCKSIZE*fork_num)) < 0)
		printf("read infd error\n");
	else if (n == 0){
	/*如果读取的字段长度为0，则说明已经读到了文件末尾，进程结束*/
		printf("%d\n",BLOCKSIZE*(fork_num));
		free(tempdataptr);
		printf("read %d complete\n",i);
		break;
	}
	else{
	/*将读取的文件信息存入struct结构中，并将进程编号相应增加，以在下次循环中 读取下一段内容
	 */
		tempdataptr->ioffset = BLOCKSIZE*fork_num;
		tempdataptr->ilen = n;
		write(pfd[1],tempdataptr,sizeof(struct data));
	//	printf("%d read succeed\n",tempdataptr->ioffset);
		fork_num += NFORK/2;
	}
	
}
}
/*主函数*/
int main(int argc,char *argv[]){ 

clock_t start_time,finish_time;
double total_time;
start_time = clock();

int i,err;
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
/*创建管道*/
if(pipe(pfd)<0){
	printf("create pipe error!\n");
	return -1;
}

/*创建进程
 *此处只有父进程能够创建子进程，父进程创建完后进入等待，子进程进入读写函数 
 */
for(i = 0; i < NFORK; i++){
	status[i] = fork();
	fork_num = i;	//记录进程数，由于创建进程后数据分离，每个进程的fork_num数都不同
	if(status[i] == 0 || status[i] == -1) break;	//确保只有父进程创建子进程
	else
		fork_num = -1;	
}
/*读进程函数*/
if(fork_num < NFORK/2 && fork_num >= 0)
	fork_r(fork_num);
/*写进程函数*/
if(fork_num >= NFORK/2 && fork_num >= 0)
	fork_w();
/*父进程等待子进程执行*/
if(fork_num < 0){ 
	for(i = 0; i < NFORK/2; i++){
		waitpid(status[i],&fork_state[i],0);
	}//判断读取进程完成
	/*关闭最后一个管道写端口，此时read函数不再阻塞*/
	close(pfd[1]);
	printf("pipe1 closed\n");
		
	for(;i < NFORK; i++){
		waitpid(status[i],&fork_state[i],0);
	}//判断写入进程完成
}
close(infd);
close(outfd);
close(pfd[0]);

finish_time = clock();
total_time = ((double)(finish_time - start_time))/CLOCKS_PER_SEC;
printf("%f seconds\n",total_time);
return 0;

}
