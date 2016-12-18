//
//  smuTorrent.c
//  test
//
//  Created by 이태우, 최현수
//  Copyright © 2016년 이태우, 최현수. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>					// sleep()
#include <pthread.h>				// pthread
#include <arpa/inet.h>				// struct sockaddr_in
#include <sys/socket.h>				// socket()
#include <sys/types.h>              // open()을 쓰기 위함
#include <sys/stat.h>               // open()을 쓰기 위함
#include <fcntl.h>                  // open()을 쓰기 위함
#include <dirent.h>					// DIR* readdir()
#include <sys/stat.h>               // 디렉토리 내의 파일의 크기를 받기 위한 헤더

#define BUF_SIZE 100
#define IP_SIZE 20
#define TRUE 1
#define FALSE 0

typedef struct tTHREAD          // 다운로드 스레드를 만들 pthread_create함수에서 스레드로 값을 넘길때 두개 이상의 값을 넘기기 위한 구조체
{
    int filediscript;       // 파일 디스크립터를 담을 변수
    int clnt_socket;        // 연결된 upload 클라이언트들의 소켓디스크립터를 담을 변수
}THREAD;

void* thread_upload(void * arg);        // 파일을 업로드하기 위해 새로운 스레드를 만드는 스레드(메인 스레드에서 만들어짐)
void* thread_download(void * arg);      // 파일을 다른 클라이언트들에게 받기 위해 만들어지는 스레드
void* thread_sendingfile(void * arg);       // 파일을 업로드해주는 스레드
void file_download();       // thread_sending이 만들어지는 스레드
void* thread_checkdir(void * arg);      // 현재 디렉토리내에 있는 파일의 리스트와 파일의 크기를 서버로 보내주는 스레드
void error_msg(char * msg);

char server_ip[] = {"211.229.192.123"};  // server IP Address
char server_port[] = {"5100"};
char server_threadport[] = {"9190"};

char filename[BUF_SIZE];        // 다운받고자 하는 파일 이름이 담길 변수
int * check;        // 파일의 블록들이 받아졌는지 판단해주는 동적할당 배열을 가리키는 포인터
int block_count;    // 파일 블록의 갯수


pthread_mutex_t mutex;      // 여러개의 다운로드 스레드에서 파일블록들을 중복으로 다운받지 않게 막아주는 Mutex




int main(){
    void * thread_return;
    pthread_t upload, download, checkdir;
    printf("Welcome to SMU Torrent System!!\n");
    
    
    pthread_mutex_init(&mutex, NULL);       // Mutex 생성 선언
    
    pthread_create(&checkdir, NULL, thread_checkdir, NULL);     // 서버에게 현재 디렉토리 내의 파일 정보를 전달할 스래드 생성
    pthread_create(&upload, NULL, thread_upload, NULL);     // 다른 사용자에게 파일을 업로드 해줄 수 있는 스레드를 생성할 스레드 생성
    
    sleep(2);       // 화면에 문자 출력 순서를 맞춰주기 위한 sleep()
    
    
    while(1) {
        file_download();    // 메인에서 다운로드 함수 호출
    }
    
    pthread_join(upload, &thread_return);
    printf("close main_socket\n");
    
    pthread_mutex_destroy(&mutex);      // Mutext 소멸
    return 0;
}

void* thread_upload(void * arg){			// part of HS
    printf("running upload thread\n");
    
    pthread_t sendingfile;
    void * thread_return;
    int upload_socket, temp_socket;
    struct sockaddr_in upload_addr, temp_addr;
    socklen_t temp_adr_sz;
    
    /* 업로드를 위한 소켓 생성 */
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
    
    /* 업로드 소켓에 연결 요청이 들어오기를 accept로 기다림 */
    while(1){
        temp_socket = accept(upload_socket, (struct sockaddr*)&temp_addr, &temp_adr_sz);
        printf("temp_socket is %d\n", temp_socket);
        if(temp_socket == -1)
            error_msg("accept()");
        else
            pthread_create(&sendingfile, NULL, thread_sendingfile, (void*)&temp_socket);        // 하나의 클라이언트에 대해 업로드를 담당할 스레드 생성
    }
    pthread_join(sendingfile, &thread_return);
    printf("end of thread_upload\n");
    return NULL;
}

void* thread_sendingfile(void * arg){            //part of HS
    int clnt_sock = *((int*)arg);
    char dir[1000] = {"./"};
    FILE *fp;
    char msg[BUF_SIZE];
    char location[BUF_SIZE];
    int fd, total, sread, filesize;
    int i;
    int temp_num;
    printf("start of thread_sendingfile\n");
    read(clnt_sock, msg, BUF_SIZE);
    printf("msg:%s\n", msg);
    
    /* 업로드 요청받은 파일이 현재 디렉토리에 있을 경우 */
    if(access(strcat(dir, msg), 0) == 0){
        printf("there is file\n");
        write(clnt_sock, "1", BUF_SIZE);
        
        if((fd = open(msg, O_RDONLY,S_IRWXU)) == -1)
            error_msg("fopen()");
        else{
        
            while(1){
                read(clnt_sock, location, BUF_SIZE);        // 다운로드 클라이언트로부터 파일 블록의 바이너리 시작위치를 받음
                if(!strcmp(location, "EOF"))
                    break;
                printf("location is : %s\n", location);
                temp_num = atoi(location);
                lseek(fd, temp_num, SEEK_SET );     // 해당하는 파일 블록의 시작위치로 offset값 만큼 이동
                
                sread = read(fd, msg, BUF_SIZE );       // 해당하는 파일 블록을 읽어들임 (BUF_SIZE 만큼 = 100 byte)
                write(clnt_sock, msg, sread);       // 다운로드 클라이언트에게 파일블록을 보냄
                memset(location, 0, BUF_SIZE);
                memset(msg, 0, BUF_SIZE);
                
            }
            
            //printf("clnt_sock : %d\n",clnt_sock);
            puts("sending is over\n");
            close(fd);
        }
    }
    /* 업로드 요청받은 파일이 현재 디렉토리에 없을 경우 */
    else{
        printf("there isn't file\n");
        write(clnt_sock, "0", BUF_SIZE);
    }
    close(clnt_sock);
    printf("end of thread_sendingfile\n");
    printf("file Input download file\n");
    return NULL;
}

void file_download() {
    
    int howmany_clnt, howmuch_size, i,j;       // howmany_clnt : 다운받을 파일을 가지고 있는 클라이언트의 수, howmuch_size : 다운받을 파일의 크기(바이트)
    struct sockaddr_in clnt_adr;
    struct sockaddr_in server_addr;
    int serv_sock;      // 서버와 연결하는 소켓 디스크립터가 담길 변수
    int fd;
    char howmany[BUF_SIZE];         // 서버로부터 받을 클라이언트 수와 파일의 크기를 잠시 담아둘 배열
    char file[BUF_SIZE];        // 다운받은 바이너리를 담을 파일의 이름
    THREAD for_thread;      // 다운로드 스레드를 만들 pthread_create함수에서 스레드로 값을 넘길때 두개 이상의 값을 넘기기 위한 구조체 변수
    
    
    memset(filename, 0, BUF_SIZE);
    printf("Input download file : ");
    fgets(filename, BUF_SIZE, stdin);
    
    filename[strlen(filename)-1] = '\0';        // 입력되는 엔터 값을 빼줌
    
    /* 서버와 연결할 소켓 생성 */
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));
    
    /* 서버와 연결 */
    if(connect(serv_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error_msg("connection()");
    
    
    write(serv_sock, "ip_down", BUF_SIZE);      // 서버에 다운로드를 할 것임을 알려줌
    write(serv_sock, filename, BUF_SIZE);        // send filename to server (몇명이 가지고 있는지 누가 가지고 있는지 알기 위해)
    
    memset(howmany, 0, BUF_SIZE);
    read(serv_sock, howmany, BUF_SIZE);       // 현재 접속해있는 사람 중에 몇명이 파일을 가지고 있는지 서버로부터 받음
    howmany_clnt = atoi(howmany);
    
    memset(howmany, 0, BUF_SIZE);
    read(serv_sock, howmany, BUF_SIZE);     // 파일의 크기를 서버로부터 받음
    howmuch_size = atoi(howmany);
    
    block_count = howmuch_size / BUF_SIZE;
    check = (int*)malloc(sizeof(int)*block_count);  // 체크 배열을 파일 블록의 갯수만큼 동적할당(파일블록의 다운로드 상태를 유지하기 위함)
    
    memset(check, 0, sizeof(int)*block_count);
    
    printf("howmany_clnt : %d\n", howmany_clnt);
    printf("howmuch_data : %d\n", howmuch_size);
    
    sleep(2);
    
    
    char client[howmany_clnt][BUF_SIZE];        // 서버에서 받은 클라이언트의 갯수만큼 IP를 담을 2차원 배열 생성
    int clnt_socks[howmany_clnt];       // 다운로드를 위해 연결된 클라이언트 소켓 디스크립터를 담는 배열 생성
    pthread_t download[howmany_clnt];       // 생설될 스레드의 ID를 담아둘 배열
    
    
    for(i=0; i < howmany_clnt; i++) {       // 파일을 가지고 있는 클라이언트들의 IP를 받음
        read(serv_sock, client[i], BUF_SIZE);       // receive clients IP address
        printf("[%d]-%s\n", i,client[i]);
    }
    
    strcpy(file, "down_");
    strcat(file, filename);
    printf("down file name : %s\n", file);
    fd = open(file,  O_RDWR| O_CREAT| O_TRUNC,S_IRWXU);     // 읽기 쓰기 겸용으로 오픈, 필요하면 파일 생성(기존 데이터 전부 삭제), 사용자에게 모든 권한을 줌
    
    
    for_thread.filediscript = fd;       // 다운로드 스레드에 새로 만든 파일의 디스크립터를 전달하기 위해
    
    
    for(i=0; i < howmany_clnt; i++) {
        
        printf("Connection IP :%s \n", client[i]);
        /* 서버로부터 자신의 IP를 받았을 경우 거름 */
        if(!strcmp(client[i],"yourip")) {
            continue;
        }
        
        /* 파일을 가지고 있는 클라이언트와 연결 */
        clnt_socks[i] = socket(PF_INET, SOCK_STREAM, 0);
        
        memset(&clnt_adr, 0, sizeof(clnt_adr));
        clnt_adr.sin_family = AF_INET;
        clnt_adr.sin_addr.s_addr = inet_addr(client[i]);        // IP주소
        clnt_adr.sin_port = htons(atoi("9190"));      // Port 번호
        
        
        if(connect(clnt_socks[i], (struct sockaddr*)&clnt_adr, sizeof(clnt_adr))== -1) {        // 다운로드 받을 클라이언트와 연결
            printf("%s 는 연결 불가! \n", client[i]);
            continue;
        }
        
        for_thread.clnt_socket = clnt_socks[i];     // 각 다운로드 스레드에 각각의 연결된 클라이언트 소켓디스크립터를 전달하기 위함
        
        printf("%s 쓰레드 생성 ! \n", client[i]);
        pthread_create(&download[i], NULL, thread_download, (void*)&for_thread);     // 각각의 다운로드 thread 생성
    }
    
    /* 파일의 다운로드가 다 끝났을 경우 스레드를 반환*/
    for(i=0 ; i < howmany_clnt; i++) {
        pthread_join(download[i], NULL);
    }
    
    free(check);
    check = NULL;
    
    close(serv_sock);
}

void* thread_download(void * arg){			//part of taewoo
    THREAD * tValue = (THREAD*) arg;
    int clnt_sock = tValue->clnt_socket;
    int str_len;
    FILE * fp;
    int filesize, sread, total=0, temp, i;
    int fd = tValue->filediscript;
    
    char result[BUF_SIZE];      // 요청한 클라이언트에 정말 파일이 있는지 확인한 뒤 결과를 담음
    char data[BUF_SIZE];        // 다른 클라이언트로부터 다운받은 파일블록이 담기는 배열
    char size[BUF_SIZE];
    char start_binary[BUF_SIZE];        // 파일 블록의 시작위치를 업로드 클라이언트에게 보내주기 위한 배열
    char local_filename[BUF_SIZE];      // 전역변수로 된 파일의 이름이 중간에 바뀌는 것을 막기위한 지역변수처럼 사용되는 배열
    char file[BUF_SIZE];
    char state[BUF_SIZE];       // 파일블록을 남김없이 다운받게 하기위해 상태를 저장하는 배열
    
    printf("thread start! : %s\n", filename);
    
    strcpy(local_filename, filename);
    
    printf("file name is : %s\n", local_filename);
    
    write(clnt_sock, local_filename, BUF_SIZE);       // 진짜 이 파일을 가지고 있는지 물어봄
    str_len = read(clnt_sock, result, BUF_SIZE);        // 가지고 있다면 1, 없다면 0을 돌려받음
    
    
    if(str_len == -1) {
        return (void*)-1;
    }
    
    if(!strcmp(result, "1")) {      // 1을 돌려받을 경우 파일을 다운로드 함
        printf("block_conut : %d\n", block_count);
        
        strcpy(state, "break");
        while(1) {
            for(i=0; i<= block_count; i++) {
                pthread_mutex_lock(&mutex);     // 파일 블록의 상태를 참조하는 동안 임계영역으로 만듦
                
                /* 만일 파일 해당 블록이 받아지지 않았다면 업로드 클라이언트에 요청 */
                if(!check[i]) {
                    memset(start_binary, 0, BUF_SIZE);
                    sprintf(start_binary, "%d", i*100);
                
                    write(clnt_sock, start_binary, BUF_SIZE);       // 파일 블록의 바이너리 시작위치를 업로드 클라이언트에게 보냄
                    printf("%s - %d\n", start_binary, clnt_sock);
                    
                    sread = read(clnt_sock, data, BUF_SIZE);        // 파일블록을 다운받음
                    /* 클라이언트와의 연결이 끊겼을 때는 다운로드 스레드를 종료 */
                    if(sread == 0 || sread == -1) {
                        printf("!!!사용자와 연결이 끊겼습니다!!!\n");
                    
                        close(clnt_sock);
                        pthread_mutex_unlock(&mutex);
                        return NULL;
                    }
                    
                    lseek(fd, i*100, SEEK_SET );        // 다운받은 파일블록을 저장하기 위해 해당위치로 offset을 옮김
                    
                    write(fd, data, sread);     // 해당위치에 다운받은 파일블록을 저장
                    memset(data, 0, BUF_SIZE);
                    check[i] = 1;       // 해당 파일블록을 다운로드 완료상태로 전환
                }
                pthread_mutex_unlock(&mutex);       // 임계영역 해제
            }
            
            /* 파일블록의 다운로드가 완료되었는지 한번 더 확인 */
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
        
        write(clnt_sock, "EOF", BUF_SIZE);      // 업로드 클라이언트에게 파일의 다운로드가 끝났음을 알려줌
        
        close(fd);
    }
    else {      // 0을 돌려받을 경우 클라이언트와 연결을 끊음
        printf("got 0 so i will close clnt_sock\n");
    }
    printf("finish!\n");
    close(clnt_sock);
    return NULL;
}

void* thread_checkdir(void * arg){
    printf("start of checkdir\n");
    struct sockaddr_in server_addr;
    
    int main_socket;
    
    /*서버와 연결*/
    main_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));
    if(connect(main_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        error_msg("connection()");
    
    write(main_socket, "ip_up", BUF_SIZE);      // 서버에게 디렉토리 내의 파일정보들을 전해주는 스레드가 시작될 것임을 알려줌
    
    while(1){
        
        DIR *dir;
        struct dirent *ent;     // 디렉토리 내의 정보를 담는 구조체
        struct stat ent_size;       // 파일의 정보를 담는 구조체
        int num_file = 0;       // 디렉토리 내의 파일의 갯수
        char filelist[BUF_SIZE];        // 파일의 이름을 담는 배열
        char howmany_file[BUF_SIZE];        // 파일의 갯수를 서버로 보내기 위한 배열
        char filesize[BUF_SIZE];        // 파일의 크기를 서버로 보내기 위한 배열
        
        dir = opendir("./");        // 현재 디렉토리 경로를 담음
       
        if (dir != NULL) {
            /* 현재 디렉토리 내의 파일의 갯수를 세주는 부분 */
            while((ent=readdir(dir)) != NULL) {
                lstat(ent->d_name, &ent_size);
        
                if (S_ISREG(ent_size.st_mode)){     // 파일 정보를 통해 해당 파일이 파일인지 디렉토리인지 확인하는 조건
                    if(strcmp(ent->d_name, ".DS_Store")) {
                        num_file++;
                    }
                }
            }
            memset(howmany_file, 0, BUF_SIZE);
            sprintf(howmany_file, "%d", num_file);
            
            write(main_socket, howmany_file, BUF_SIZE);     // 현재 디렉토리 내의 파일의 갯수를 서버로 보내줌
            
            rewinddir(dir);     // 디렉토리 읽기 위치를 처음으로 이동
            
            /* 현재 디렉토리 내의 파일 이름과 파일 크기로 서버로 보내주는 부분 */
            while((ent=readdir(dir)) != NULL) {
                lstat(ent->d_name, &ent_size);      // 파일 위치(파일이름)을 주고 구조체의 포인터를 넘겨줌 => 해당 파일의 정보가 구조체에 담김
                
                if(S_ISREG(ent_size.st_mode)) {     //
                    if(strcmp(ent->d_name, ".DS_Store")) {
                        memset(filelist, 0, BUF_SIZE);
                        strcpy(filelist, ent->d_name);
                        write(main_socket, filelist, BUF_SIZE);     // 파일의 이름을 서버로 보내줌
                        
                        
                        memset(filesize, 0, BUF_SIZE);
                        sprintf(filesize, "%lld" , ent_size.st_size);
                        write(main_socket, filesize, BUF_SIZE);     // 파일의 크기를 서버로 보내줌
                        
                    }
                }
            }
        }
        else
            printf("can't find directory\n");
        
        
        closedir(dir);
        sleep(10);      // 10초에 한번씩 디렉토리 정보를 서버로 보냄
    }
    close(main_socket);
    return NULL;
}


void error_msg(char * msg){
    printf("%s is error!!\n", msg);
    //exit(1);
}
