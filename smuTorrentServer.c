#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include </usr/include/mysql/mysql.h>
#define SOCKET int
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS "***********"
#define DB_NAME "net"
#define CHOP(x) x[strlen(x) - 1] = ' '
#define BUF_SIZE 100
#define MAX_CLNT 256

int       query_stat;
int user_join(void * clnt_sock,char data1[],char data2[],char data3[]);
void * handle_clnt(int arg[]);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int db2();
void DB_main();
int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
char user_id[10];
char user_pw[20];
char filename[20];
pthread_mutex_t mutx;
#define MAXLINE 127
void finish_with_error(MYSQL *conn)
{
    fprintf(stderr, "%s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

int main(int argc, char *argv[])
{char cli_ip[20];
    int tmp;
    int serv_sock, clnt_sock;
    int sock_info[2];
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    
    pthread_mutex_init(&mutx, NULL);
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");
    inet_ntop(AF_INET, &clnt_adr.sin_addr.s_addr, cli_ip, sizeof(cli_ip));
    
    while(1)
    {
        
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);//클라이언트와의 통신을 위한 새로운 소켓 생성
        sock_info[0]=clnt_sock;
        sock_info[1] =inet_ntoa(clnt_adr.sin_addr);
        pthread_mutex_lock(&mutx);
        clnt_socks[tmp = clnt_cnt++]=clnt_sock;
        pthread_mutex_unlock(&mutx);
        
        pthread_create(&t_id, NULL, handle_clnt, sock_info);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void * handle_clnt(int  arg[])
{char buf[MAXLINE+1];
    
   
    
    int clnt_sock=arg[0];
    int i;
    char msg[BUF_SIZE];
    char tmp[BUF_SIZE];
    char tmp1[BUF_SIZE];
    char tmp2[BUF_SIZE];
    char tmp3[BUF_SIZE];
    char tmp4[BUF_SIZE];
    char ip_tmp[BUF_SIZE];
    int num;
    int ak_i;
    int check;
    int ip_num;
    int file_num;
    char del_data[BUF_SIZE]="$del_data", nodata[BUF_SIZE]="nodata",update[BUF_SIZE]="$update";
    char data1[BUF_SIZE], data2[BUF_SIZE], data3[BUF_SIZE], data4[BUF_SIZE];
    
    
    sprintf(ip_tmp,"%s",arg[1]);

    memset(&tmp,0,100);
    read(clnt_sock, tmp, 100);
  
    if(!strcmp(tmp,"ip_up"))
    {while(1){
        memset(tmp3, 0, 100);
        check = read(clnt_sock, tmp3, 100);
              if(check == 0)
        {
            user_join(clnt_sock,ip_tmp,del_data,nodata);
            for(i=0; i<clnt_cnt; i++)   // remove disconnected client
            {
                if(clnt_sock==clnt_socks[i])
                {
                    while(i++<clnt_cnt-1)
                        clnt_socks[i]=clnt_socks[i+1];
                    break;
                }
                
            }
            close(clnt_sock);
            return NULL;
            
        }
        user_join(clnt_sock,ip_tmp,del_data,nodata);
        file_num = atoi(tmp3);
        printf("파일개수:%d\n",file_num);
        for(ak_i=0;ak_i<file_num; ak_i++)
        {
            
            memset(&tmp3, 0, 100);
            check = read(clnt_sock, tmp3, 100);
            printf("check : %d\n",check);
            printf("tmp3%s\n",tmp3);
            //sleep(1);
            memset(&tmp4, 0, 100);
            read(clnt_sock, tmp4, 100);
            printf("파일사이즈%s \n",tmp4);
            printf("끝\n");
            printf("num = %d\n",ak_i);
            user_join(clnt_sock,ip_tmp,tmp3,tmp4);//ip_addr 파일 이름파일 사이즈
        }
    }
    }
    else if(!strcmp(tmp,"ip_down"))
    {while(1){
        check=  read(clnt_sock, tmp, sizeof(tmp));
        if(check == 0)
        {
            for(i=0; i<clnt_cnt; i++)   // remove disconnected client
            {
                if(clnt_sock==clnt_socks[i])
                {
                    while(i++<clnt_cnt-1)
                        clnt_socks[i]=clnt_socks[i+1];
                    break;
                }
                
            }
            close(clnt_sock);
            return NULL;
            
        }
        printf("9999:%s\n",tmp);
        user_join(clnt_sock,tmp,nodata,arg[1]);
        
    }  }
    printf("hello finish\n");
    
    
    
    for(i=0; i<clnt_cnt; i++)   // remove disconnected client
    {
        if(clnt_sock==clnt_socks[i])
        {
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}
int user_join(void * clnt_sock,char data1[],char data2[],char data3[])
{
    MYSQL       *connection=NULL, conn;
    MYSQL_RES   *sql_result;
    MYSQL_ROW   sql_row;
    char sql1[2048];
    char sql2[2048];
    char buf1[2048];
    int sql_i=0,sql_j;
    mysql_init(&conn);
    char last[BUF_SIZE];
    connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, (char*)NULL, 0);
    memset(&sql1,0,sizeof(sql1));
    memset(&sql2,0,sizeof(sql2));
    memset(&last,0,sizeof(last));
    if(connection==NULL)
    {
        fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn));
        return EXIT_FAILURE;
    }
    
    else
        printf("Mysql connected\n");
  
    if(!strcmp(data2,"nodata"))
    {
        sprintf(sql1,"select (select count(*) from new_p2p WHERE file_name = '%s') as no,ip_addr , file_size,file_name from new_p2p where file_name = '%s' ",data1,data1);
        sprintf(sql2,"SELECT ip_addr,file_size from `smu_p2p` WHERE `file_name` LIKE '%s'",data1);
        printf("query = %s\n",sql1);
        query_stat = mysql_query(connection,sql1);
        if(query_stat != 0)
        {
            fprintf(stderr, "Mysql query error : %s\n", mysql_error(&conn));
            return EXIT_FAILURE;
        }
     
        sql_result = mysql_store_result(connection);
     
        if(sql_result == NULL)
        {
            write(clnt_sock,"0",100);
            return EXIT_FAILURE;
        }
   
        sql_i = 0;
        while ( (sql_row = mysql_fetch_row(sql_result)) != NULL )
        {
            if(sql_i ==0)
            {	write(clnt_sock,sql_row[0],100);
                printf("send howmany ip :%",sql_row[0]);
                write(clnt_sock,sql_row[2],100);
                printf("send filesize",sql_row[2]);
                sql_i= 1;
            }
            if(!strcmp(data3,sql_row[1]))
            {
                write(clnt_sock,"yourip",100);
                continue;
            }
            
            write(clnt_sock,sql_row[1],100);
            printf("ip send%s\n",sql_row[1]);
        }
        
        
        mysql_close(connection);
        
    }
    else if(!strcmp(data2,"$del_data"))
    {
        printf("(%s)'s data is del");
        
        sprintf(sql2,"DELETE FROM `new_p2p` WHERE ip_addr LIKE '%s'",data1);
        printf("del_query %s\n",sql2);
        query_stat = mysql_query(connection,sql2);
        
        
        if(query_stat != 0)
        {
            fprintf(stderr, "Mysql query error : %s\n", mysql_error(&conn));
            return EXIT_FAILURE;
        }
   
        
    }
    else if(!strcmp(data2,"$update"))
    {
        sprintf(sql2,"UPDATE new_p2p SET online = 0 WHERE ip_addr ='%s'",data1);
        printf("update query %s\n",sql2);
        query_stat = mysql_query(connection,sql2);
        
        if(query_stat != 0)
        {
            fprintf(stderr, "Mysql query error : %s\n", mysql_error(&conn));
            return EXIT_FAILURE;
        }
        
    }
    else
    {
        
        sprintf(sql1,"INSERT IGNORE INTO `new_p2p`( `ip_addr`, `file_name`, `file_size`) VALUES ('%s','%s','%s')",data1,data2,data3);
 
        query_stat = mysql_query(connection,sql1);
        
        
        if(query_stat != 0)
        {
            fprintf(stderr, "Mysql query error : %s\n", mysql_error(&conn));
            return EXIT_FAILURE;
        }
    }
    mysql_close(connection);
}








void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}


