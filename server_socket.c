#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUF_SIZE 65536 // 64KB
#define ERROR_404 "<h1>404 Not Found</h1>\n" // 404 에러 발생시 띄워주기 위해 생성
#define ERROR_500 "<h1>500 Internal Server Error</h1>\n" // 500 에러 발생시 띄워주기 위해 생성
#define RESPONSE_HEADER_FORMAT "HTTP/1.1 %d %s\nContent-Length: %lld\nContent-Type: %s\n\n" // response 메세지 header의 기본 format 설정

int server_socket; // server socket 받는 변수
int client_socket; // client socket 받는 변수
int log_fd; // HTTP request 메서지를 dump하기 위한 file descriptor

void make_response(char *header, int status, long long content_len, char *file_type); // HTTP response 메세지 header를 만들어준다.
void find_type(char *file_type, char *); // HTTP response 메세지에 사용될 Content-Type을 구분해주는 함수
void http_handler(int client_socket); // HTTP request 메시지를 읽고, HTTP response 메시지로 처리해주는 함수

int main(int argc, char **argv){ // argv:실행시 입력한 것 저장, argc:입력 개수
  pid_t pid;

  if(argc!=2) perror("[ERR] no port");// (실행 명령어 + port) 2개가 들어와야 한다.
  printf("[INFO] The server will listen to port: %d.\n", atoi(argv[1]));

  log_fd = open("log.txt", O_CREAT | O_TRUNC | O_RDWR, 0644);
  // 파일 없으면 생성해야 하며, 실행할 때 계속 새롭게 dump해야 하므로 내용이 있어도 지우고 쓸 수 있도록 생성한다.

  // server, client 주소 받기 위해 생성
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_size; // client_addr

  // TCP socket 생성
  server_socket = socket(PF_INET, SOCK_STREAM, 0);
  if(server_socket == -1){
    perror("[ERR] socket error");
    close(log_fd); // file descriptor 닫아준다.
    exit(1);
  }

  // address 초기화 & setting ip daress, port
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET; // type : ipv4
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // setting server ip address
  server_addr.sin_port = htons(atoi(argv[1])); // setting server port


  // Server_Address binding to socket
  if ( bind(server_socket,(struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("[ERR] bind error");
    close(server_socket); // server socket 닫아준다.
    close(log_fd); // file descriptor 닫아준다.
    exit(1);
  }

  // listen 연결 대기열 생성
  if (listen(server_socket, 5) == -1){
    perror("listen");
    close(server_socket); // server socket 닫아준다.
    close(log_fd); // file descriptor 닫아준다.
    exit(1);
  }

  // to handle zombie process
  signal(SIGCHLD, SIG_IGN); // Child process 종료시 시그널을 무시하여 종료시킨다.

  // socket 연결 실행
  while(1){
    printf("[INFO] waiting...\n\n");
    client_addr_size = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
    if(client_socket == -1){
      perror("[ERR] accept fail");
      close(server_socket); // server socket 닫아준다.
      close(log_fd); // file descriptor 닫아준다.
      exit(1);
    }
    // 대기 시간 없이 동시에 client 접속 제공 위해 fork()통해 Multiprocess 진행
    pid = fork(); // client 접속할 때마다 fork를 통해 child process를 생성
    if(pid<0){
      perror("fork error");
      exit(1);
    } else if(pid == 0){
      close(server_socket); // child process server socket 닫아준다.
      http_handler(client_socket); // client가 요청한 메세지 읽고 처리해준다.
      close(client_socket); // child process client socket 닫아준다.
      exit(0);
    } else{
      close(client_socket); // parent process client socket 닫아준다.
    }
  }
  close(server_socket); // 사용이 끝난 server socket 닫아준다.
  close(log_fd); // file descriptor 닫아준다.
  return 0;
}

void make_response(char *response_header, int status, long long content_len, char *file_type){
  char status_msg[40];
  // status에 따라 status 메세지를 설정해준다.
  switch(status) {
        case 200:
            strcpy(status_msg, "OK");
            break;
        case 404:
            strcpy(status_msg, "Not Found");
            break;
        case 500:
        default:
            strcpy(status_msg, "Internal Server Error");
    }
    // 정보 추합하여 response 메세지 header를 만들어준다.
    sprintf(response_header, RESPONSE_HEADER_FORMAT, status, status_msg, content_len, file_type);
}

void find_type(char *file_type, char *uri){
  char *type_name = strrchr(uri, '.'); // uri에서 확장자 명을 받는다.

  // 확장자 명을 이용하여 content_type 구분해준다.
  if(!strcmp(type_name, ".html")) {
      strcpy(file_type, "text/html");
  } else if(!strcmp(type_name, ".gif")) {
      strcpy(file_type, "image/gif");
  } else if(!strcmp(type_name, ".jpeg")) {
      strcpy(file_type, "image/jpeg");
  } else if(!strcmp(type_name, ".mp3")) {
      strcpy(file_type, "audio/mpeg");
  } else if(!strcmp(type_name, ".pdf")) {
      strcpy(file_type, "application/pdf");
  }
}

void http_handler(int client_socket){
  char response_header[BUF_SIZE]; // HTTP response 메세지 header 저장
  char buf[BUF_SIZE]; // HTTP request 메세지 및 파일 불러올때 사용

  memset(response_header, 0, sizeof(response_header));
  memset(buf, 0, sizeof(buf));

  // 500 Error
  if(read(client_socket, buf, BUF_SIZE) < 0){ // read에 실패한 경우 500 error
    make_response(response_header, 500, sizeof(ERROR_500), "text/html");
    printf("%s\n", response_header);
    // client 소켓에 HTTP response 메세지 헤더와 내용을 넣어준다.
    write(client_socket, response_header, strlen(response_header));
    write(client_socket, ERROR_500, sizeof(ERROR_500));
    perror("[ERR] read error");
    return;
  }
  printf("%s\n\n", buf); // HTTP request 메세지 출력
  write(log_fd, buf, BUF_SIZE);

  char *method = strtok(buf, " "); // ex) GET
  char *uri = strtok(NULL, " "); // ex) /index.html
  printf("[INFO] Handling Request: method=%s, URI=%s\n", method, uri);

  if(method == NULL || uri == NULL){ // method 및 uri 받기에 실패한 경우 500 error
    make_response(response_header, 500, sizeof(ERROR_500), "text/html");
    printf("%s\n", response_header);
    // client 소켓에 HTTP response 메세지 헤더와 내용을 넣어준다.
    write(client_socket, response_header, strlen(response_header));
    write(client_socket, ERROR_500, sizeof(ERROR_500));
    perror("[ERR] method or uri error");
    return;
  }

  if(!strcmp(uri, "/")) strcpy(uri, "/index.html"); // "/" 입력시 index.html로 연결되도록 예외처리
  char *local_uri;
  local_uri = uri+1; // 앞에 있는 "/"를 제거해준다. (상대 경로)
  struct stat st; // file 상태 받아오기 위한 구조체

  if(stat(local_uri,&st) == -1){ // file 없는 경우 404 error
    make_response(response_header, 404, sizeof(ERROR_404), "text/html");
    printf("%s\n", response_header);
    // client 소켓에 HTTP response 메세지 헤더와 내용을 넣어준다.
    write(client_socket, response_header, strlen(response_header));
    write(client_socket, ERROR_404, sizeof(ERROR_404));
    perror("[ERR] No file");
    return;
  }

  int local_fd;
  local_fd = open(local_uri, O_RDONLY);
  if(local_fd == -1){ // 파일 열기에 실패한 경우 500 error
    make_response(response_header, 500, sizeof(ERROR_500), "text/html");
    printf("%s\n", response_header);
    // client 소켓에 HTTP response 메세지 헤더와 내용을 넣어준다.
    write(client_socket, response_header, strlen(response_header));
    write(client_socket, ERROR_500, sizeof(ERROR_500));
    perror("[ERR] failed to open file");
    return;
  }

  char file_type[40]; // file의 type 받기 위해 생성
  long long content_len = st.st_size; // file의 크기를 받음
  find_type(file_type, uri);
  make_response(response_header, 200, content_len, file_type); // 404및 500 에러 처리 완료했기 때문에 200
  printf("%s\n", response_header);
  // client 소켓에 HTTP response 메세지 헤더를 넣어준다.
  write(client_socket, response_header, strlen(response_header));
  // file을 계속 읽어 client 소켓에 보내준다.
  int reading;
  while((reading = read(local_fd, buf, BUF_SIZE)) > 0){
    write(client_socket, buf, reading);
  }
}
