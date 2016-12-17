//
//  smuTorrent.c
//  test
//
//  Created by 이태우, 최현수
//  Copyright © 2016년 이태우, 최현수. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>					//exit()
#include <string.h>					//memset()
#include <unistd.h>					//sleep()
#include <pthread.h>				//pthread
#include <arpa/inet.h>				//struct sockaddr_in
#include <sys/socket.h>				//socket()
#include <sys/types.h>              // open()을 쓰기 위함
#include <sys/stat.h>               // open()을 쓰기 위함
#include <fcntl.h>                  // open()을 쓰기 위함
#include <dirent.h>					//DIR* readdir()
#include <sys/stat.h>               // 디렉토리 내의 파일의 크기를 받기 위한 헤더

#define BUF_SIZE 100
#define IP_SIZE 20      // add!!!!
#define TRUE 1
#define FALSE 0

typedef struct tTHREAD
{
    int filediscript;
    int clnt_socket;
}THREAD;

void* thread_upload(void * arg);
void* thread_download(void * arg);
void* thread_sendingfile(void * arg);
void file_download();
void* thread_checkdir(void * arg);
void error_msg(char * msg);

char server_ip[] = {"211.229.192.123"};  // server IP Address
char server_port[] = {"5100"};
char server_threadport[] = {"9190"};

char filename[BUF_SIZE];        // 다운받고자 하는 파일 이름이 담길 변수
int *check;
int block_count;


//int serv_sock;


pthread_mutex_t mutex;




int main(){
    void * thread_return;
    pthread_t upload, download, checkdir;
    printf("Welcome to SMU Torrent System!!\n");
    
    
    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&checkdir, NULL, thread_checkdir, NULL);
    pthread_create(&upload, NULL, thread_upload, NULL);
    //pthread_create(&download, NULL, thread_download, (void*)&main_socket);
    
    sleep(2);
    
    
    
    /*
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));
    printf("server sock : %d\n", serv_sock);
    
    if(connect(serv_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error_msg("connection()");
    */
    while(1) {
        file_download();    // 메인에서 다운로드 함수 호출
    }
    
    //pthread_join(checkdir, &thread_return);
    pthread_join(upload, &thread_return);
    //pthread_join(download, &thread_return);
    printf("close main_socket\n");
    
    pthread_mutex_destroy(&mutex);
    return 0;
}

void* thread_upload(void * arg){			//part of HS
    printf("running upload thread\n");
    
    pthread_t sendingfile;
    void * thread_return;
    //	char msg[1024] = "Project_client.c";
    //	char send_msg[1024], received_msg[1024];
    int upload_socket, temp_socket;
    struct sockaddr_in upload_addr, temp_addr;
    socklen_t temp_adr_sz;
    
    upload_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&upload_addr, 0, sizeof(upload_addr));
    upload_addr.sin_family = AF_INET;
    upload_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    upload_addr.sin_port = htons(atoi(server_threadport));
    
    if(bind(upload_socket, (struct sockaddr*)&upload_addr, sizeof(upload_addr)) == -1)
        error_msg("bind()");
    if(listen(upload_socket, 5) == -1)
        error_msg("listen()");
    
    temp_adr_sz = sizeof(temp_addr);
    
    while(1){
        temp_socket = accept(upload_socket, (struct sockaddr*)&temp_addr, &temp_adr_sz);
        printf("temp_socket is %d\n", temp_socket);
        if(temp_socket == -1)
            error_msg("accept()");
        else
            pthread_create(&sendingfile, NULL, thread_sendingfile, (void*)&temp_socket);
    }
    pthread_join(sendingfile, &thread_return);
    printf("end of thread_upload\n");
    return NULL;
}

void* thread_sendingfile(void * arg){            //part of HS
    int clnt_sock = *((int*)arg);
    char dir[1000] = {"./"};
    FILE *fp;
    //    char file[1024];
    char msg[BUF_SIZE];
    char location[BUF_SIZE];
    int fd, total, sread, filesize;
    int i;
    int temp_num;
    printf("start of thread_sendingfile\n");
    read(clnt_sock, msg, BUF_SIZE);
    printf("msg:%s\n", msg);
    
    if(access(strcat(dir, msg), 0) == 0){
        printf("there is file\n");
        write(clnt_sock, "1", BUF_SIZE);
        
        //        if((fp = fopen(msg, "rb")) == NULL)
        //            error_msg("fopen()");
        if((fd = open(msg, O_RDONLY,S_IRWXU)) == -1)
            error_msg("fopen()");
        else{
            //filesize = lseek( fd, 0, SEEK_END);
            //write(clnt_sock, &filesize, BUF_SIZE);                       //file size전송
            //lseek(fd, 0, SEEK_SET );
            
            //printf("file size : %d\n", filesize);
            while(1){
                read(clnt_sock, location, BUF_SIZE);
                if(!strcmp(location, "EOF"))
                    break;
                printf("location is : %s\n", location);
                temp_num = atoi(location);
                lseek(fd, temp_num, SEEK_SET );
                
                sread = read(fd, msg, BUF_SIZE );
                write(clnt_sock, msg, sread);
                memset(location, 0, BUF_SIZE);
                memset(msg, 0, BUF_SIZE);
                
            }
            //            while( total != filesize )                   //file 전송 부
            //            {
            //               /*
            //                 sread = read(fd, msg, 100 );
            //                 printf( "file is sending now.. " );
            //                 total += sread;
            //                 msg[sread] = 0;
            //                 send(temp_socket, msg, sread, 0 );
            //                 printf( "processing :%4.2f%% \n ", (float)total*100 / (float)filesize );
            //                 */
            //                sread = read(fd, msg, BUF_SIZE );
            //                printf( "file is sending now.. , %d", sread );
            //                total += sread;
            //                msg[sread] = 0;
            //                write(clnt_sock, msg, sread);
            //                memset(msg, 0, BUF_SIZE);
            //                printf( "processing : %4.2f%%, total : %d \n", (float)total*100 / (float)filesize, total );
            
            //           }
            //write(temp_socket, "EOF", BUF_SIZE);
            
            printf("clnt_sock : %d\n",clnt_sock);
            puts("sending is over\n");
            close(fd);
        }
    }
    else{
        printf("there isn't file\n");
        write(clnt_sock, "0", BUF_SIZE);
    }
    close(clnt_sock);
    printf("end of thread_sendingfile\n");
    return NULL;
}

void file_download() {
    
    int howmany_clnt, howmuch_size, i,j;       // howmany_clnt : 다운받을 파일을 가지고 있는 클라이언트의 수
    struct sockaddr_in clnt_adr;
    struct sockaddr_in server_addr;
    int serv_sock;
    int fd;
    char howmany[BUF_SIZE];
    char file[BUF_SIZE];
    THREAD for_thread;
    
    
    memset(filename, 0, BUF_SIZE);
    printf("Input download file : ");
    fgets(filename, BUF_SIZE, stdin);
    
    filename[strlen(filename)-1] = '\0';
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));
    
    if(connect(serv_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error_msg("connection()");
    
    printf("server sock : %d\n", serv_sock);
    
    write(serv_sock, "ip_down", BUF_SIZE);
    write(serv_sock, filename, BUF_SIZE);        // send filename to server (몇명이 가지고 있는지 누가 가지고 있는지 알기 위해)
    
    memset(howmany, 0, BUF_SIZE);
    read(serv_sock, howmany, BUF_SIZE);       // 몇명이 파일을 가지고 있는지 서버로부터 받음
    howmany_clnt = atoi(howmany);
    
    memset(howmany, 0, BUF_SIZE);
    read(serv_sock, howmany, BUF_SIZE);     // 파일의 크기를 서버로부터 받음
    howmuch_size = atoi(howmany);
    
    block_count = howmuch_size / BUF_SIZE;
    check = (int*)malloc(sizeof(int)*block_count);  // 체크 배열을 동적할당'
    
    
    
    printf("after make malloc - check pointer : %p\n", check);
    memset(check, 0, sizeof(int)*block_count);
    
    /*printf("after malloc-blcok_count : %d\n", block_count);
    for(i=0; i<=block_count; i++) {
        printf("%d ", check[i]);
    }
    printf("\n");
    */
    printf("howmany_clnt : %d\n", howmany_clnt);
    printf("howmuch_data : %d\n", howmuch_size);
    
    sleep(2);
    
    //howmany_clnt = 1; // <Hard-coding> 파일을 가지고 있는 클라이언트의 수를 그냥 하드코딩 함(일단)
    
    char client[howmany_clnt][BUF_SIZE];
    int clnt_socks[howmany_clnt];
    pthread_t download[howmany_clnt];
    
    
    for(i=0; i < howmany_clnt; i++) {       // 파일을 가지고 있는 클라이언트들의 IP를 받음
        read(serv_sock, client[i], BUF_SIZE);       // receive clients IP address
        printf("[%d]-%s\n", i,client[i]);
    }
    
    //strcpy(client[0], "203.237.179.42");     // <Hard-coding> 파일을 가지고 있는 클라이언트들의 IP를 나 자신으로 하드코딩 함(일단)
    //strcpy(client[1], "127.0.0.1");     // <Hard-coding>
    
    strcpy(file, "down_");
    strcat(file, filename);
    printf("down file name : %s\n", file);
    fd = open(file,  O_RDWR| O_CREAT| O_TRUNC,S_IRWXU);
    
    
    for_thread.filediscript = fd;
    
    
    for(i=0; i < howmany_clnt; i++) {
        
        printf("Connection IP :%s \n", client[i]);
        if(!strcmp(client[i],"yourip")) {
            continue;
        }
        
        clnt_socks[i] = socket(PF_INET, SOCK_STREAM, 0);    // 파일을 가지고 있는 클라이언트와 연결
        
        memset(&clnt_adr, 0, sizeof(clnt_adr));
        clnt_adr.sin_family = AF_INET;
        clnt_adr.sin_addr.s_addr = inet_addr(client[i]);        // IP주소
        clnt_adr.sin_port = htons(atoi("9190"));      // Port 번호
        
        
        if(connect(clnt_socks[i], (struct sockaddr*)&clnt_adr, sizeof(clnt_adr))== -1) {        // 다운로드 받을 클라이언트와 연결
            //error_msg("connect() error");
            printf("%s 는 연결 불가! \n", client[i]);
            continue;
        }
        
        for_thread.clnt_socket = clnt_socks[i];
        
        //pthread_create(&download[i], NULL, thread_download, (void*)&clnt_socks[i]);     // 각각의 thread 생성
        //pthread_create(&download[i], NULL, thread_download, for_thread);     // 각각의 thread 생성
        printf("%s 쓰레드 생성 ! \n", client[i]);
        pthread_create(&download[i], NULL, thread_download, (void*)&for_thread);     // 각각의 thread 생성
    }
    
    for(i=0 ; i < howmany_clnt; i++) {
        pthread_join(download[i], NULL);
    }
    
    //pthread_mutex_lock(&mutex);
    printf("before free - check pointer : %p\n", check);
    free(check);
    check = NULL;
    //pthread_mutex_unlock(&mutex);
    printf("malloc is free!\n");
    
    sleep(5);
    close(serv_sock);
    /*
    printf("after free-blcok_count : %d\n", block_count);
    for(i=0; i<=block_count; i++) {
        printf("%d ", check[i]);
    }
    printf("\n");
    */
}

void* thread_download(void * arg){			//part of taewoo
    THREAD * tValue = (THREAD*) arg;
    int clnt_sock = tValue->clnt_socket;
    //int clnt_sock = arg[1];
    int str_len;
    FILE * fp;
    int filesize, sread, total=0, temp, i;
    int fd = tValue->filediscript;
    //int fd = *((int *)arg[1]);
    char result[BUF_SIZE];
    char data[BUF_SIZE];
    char size[BUF_SIZE];
    char start_binary[BUF_SIZE];
    char local_filename[BUF_SIZE];
    char file[BUF_SIZE];
    char state[BUF_SIZE];
    
    //temp = clnt_sock;
    
    printf("thread start! : %s\n", filename);
    
    strcpy(local_filename, filename);
    
    //local_filename[strlen(local_filename) - 1] = 0;     // 엔터키 지우기
    
    printf("file name is : %s\n", local_filename);
    
    write(clnt_sock, local_filename, BUF_SIZE);       // 진짜 이 파일을 가지고 있는지 물어봄
    str_len = read(clnt_sock, result, BUF_SIZE);        // 가지고 있다면 1, 없다면 0을 돌려받음
    
    printf("clnt_sock1 : %d\n", clnt_sock);
    
    if(str_len == -1) {
        return (void*)-1;
    }
    printf("clnt_sock2 : %d\n", clnt_sock);
    
    //str_len = read(clnt_sock, &filesize, BUF_SIZE);
    //str_len = read(clnt_sock, size, BUF_SIZE);
    //filesize = atoi(size);
    
    //printf("clnt_sock3 : %d, %d\n", clnt_sock, temp);
    //clnt_sock = temp;
    //printf("file size : %d\n", filesize);
    
    
    if(!strcmp(result, "1")) {      // 1을 돌려받을 경우 파일을 다운로드 함
        
        
        
        
        printf("block_conut : %d\n", block_count);
        
        //memset(check, 0, sizeof(int)*block_count);
        
        /*printf("before while-blcok_count : %d\n", block_count);
        for(i=0; i<=block_count; i++) {
            printf("%d ", check[i]);
        }
        printf("\n");
        */
        strcpy(state, "break");
        while(1) {
            for(i=0; i<= block_count; i++) {
                pthread_mutex_lock(&mutex);
                if(!check[i]) {
                    memset(start_binary, 0, BUF_SIZE);
                    sprintf(start_binary, "%d", i*100);
                
                    write(clnt_sock, start_binary, BUF_SIZE);
                    printf("%s - %d\n", start_binary, clnt_sock);
                    
                    sread = read(clnt_sock, data, BUF_SIZE);
                    if(sread == 0 || sread == -1) {
                        printf("!!!!!!\n");
                        //strcpy(state, "force");
                        close(clnt_sock);
                        pthread_mutex_unlock(&mutex);
                        return NULL;
                    }
                    
                    lseek(fd, i*100, SEEK_SET );
                    
                    write(fd, data, sread);
                    memset(data, 0, BUF_SIZE);
                    check[i] = 1;
                }
                pthread_mutex_unlock(&mutex);
            }
            
            /*if(!strcmp(state, "force")) {
                break;
            }*/
            
            for(i=0; i<= block_count; i++) {
                pthread_mutex_lock(&mutex);
                if(!check[i]) {
                    strcpy(state, "one_more");
                }
                pthread_mutex_unlock(&mutex);
            }
            
            if(!strcmp(state, "one_more")) {
                continue;
            }
            else {
                break;
            }
        }
        
        /*printf("after while-blcok_count : %d\n", block_count);
        for(i=0; i<=block_count; i++) {
            printf("%d ", check[i]);
        }
        printf("\n");
        */
        write(clnt_sock, "EOF", BUF_SIZE);
        /*
        while( total != filesize )
        {
            sread = read(clnt_sock, data, BUF_SIZE);
            printf( "file is receiving now.. ");
            
            total += sread;
            data[sread] = 0;
            write(fd, data, sread);
            memset(data, 0, BUF_SIZE);
            printf( "processing : %4.2f%%, total : %d \n", (float)total*100 / (float)filesize, total );
            
            
        }
        */
        close(fd);
    }
    else {      // 0을 돌려받을 경우 클라이언트와 연결을 끊음
        printf("got 0 so i will close clnt_sock\n");		//add by hs
    }
    printf("finish!\n");
    close(clnt_sock);
    return NULL;
}

void* thread_checkdir(void * arg){
    printf("start of checkdir\n");
    struct sockaddr_in server_addr;
    
    int main_socket;
    
    main_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));
    if(connect(main_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error_msg("connection()");
    
    write(main_socket, "ip_up", BUF_SIZE);
    
    while(1){
        
        //printf("start send_filename function\n");
        DIR *dir;
        struct dirent *ent;
        struct stat ent_size;
        int num_file = 0;
        char filelist[BUF_SIZE];
        char howmany_file[BUF_SIZE];
        char filesize[BUF_SIZE];
        //dir = opendir ("/Users/huynsoochoi/Desktop/1");
        //dir = opendir("/Users/Taewoo/Desktop/test/test");
        dir = opendir("./");
        //	char str1[1024];
        
        if (dir != NULL) {
            while((ent=readdir(dir)) != NULL) {
                lstat(ent->d_name, &ent_size);
        
                
                if (S_ISREG(ent_size.st_mode)){
                    if(strcmp(ent->d_name, ".DS_Store")) {
                        num_file++;
                    }
                }
            }
            memset(howmany_file, 0, BUF_SIZE);
            sprintf(howmany_file, "%d", num_file);
            printf("file의 갯수 : %s\n", howmany_file);
            write(main_socket, howmany_file, BUF_SIZE);
            
            rewinddir(dir);     // 디렉토리 읽기 위치를 처음으로 이동
            
            while((ent=readdir(dir)) != NULL) {
                lstat(ent->d_name, &ent_size);      // 파일 위치(파일이름)을 주고 구조체의 포인터를 넘겨줌 => 해당 파일의 정보가 구조체에 담김
                
                if(S_ISREG(ent_size.st_mode)) {     //
                    if(strcmp(ent->d_name, ".DS_Store")) {
                        memset(filelist, 0, BUF_SIZE);
                        strcpy(filelist, ent->d_name);
                        //printf("filelist : %s ,", filelist);
                        write(main_socket, filelist, BUF_SIZE);
                        
                        
                        memset(filesize, 0, BUF_SIZE);
                        sprintf(filesize, "%lld" , ent_size.st_size);
                        //printf("filesize : %s\n", filesize);
                        write(main_socket, filesize, BUF_SIZE);
                        
                    }
                }
            }
        }
        else
            printf("can't find directory\n");
        
        //printf("filelist :%d\n%s\n", num_file, filelist);
        closedir(dir);
        //printf("end send_filename function\n");
        sleep(10);
    }
    close(main_socket);
    return NULL;
}

//void send_filelist(){
//	printf("start send_filename function\n");
//	char filelist[10240] = {"/"};
//
//	DIR *dir;
//	struct dirent *ent;
//	dir = opendir ("/Users/huynsoochoi/Desktop/1");
//
//	if (dir != NULL) {
//		while((ent=readdir(dir)) != NULL) {
//			strcat(filelist, ent->d_name);
//			strcat(filelist, "/");
//			printf("name is : %s\n", ent->d_name);
//			//write(clnt_sock, ent->d_name, strlen(ent->d_name));
//			//read(clnt_sock, message, BUF_SIZE);
//		}
//		//write(clnt_sock, "EOF", strlen("EOF"));
//	}
//	else
//		printf("can't find directory\n");
//	//write(clnt_sock, filelist, sizeof(filelist));
//	printf("filelist %s\n", filelist);
//	closedir(dir);
//	printf("end send_filename function\n");
//}

void error_msg(char * msg){
    printf("%s is error!!\n", msg);
    //exit(1);
}
