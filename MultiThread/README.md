# Linux Multi Thread Socket
Multi Thread Socket MSG



### Server
* ASW EC2에서 server.c 구동
```bash
gcc server.c -lm -o server -lpthread
./server [Port]
```
* 기능 1 echo )
    *   client 가 소문자를 전송한다 ( Server 는 대문자로 돌려준다 )
* 기능 2 DB )
    *  client 가 변수에 값을 저장한다 ( Server 는 값을 보관한다 )
    * client 가 변수 의 값을 불러온다 ( Server 는 값을 돌려준다 )
    
### Client
* Virtual Machine ( Ubuntu Linux ) 접속
```bash
gcc client.c -o client -lpthread
./client [Name]
```


* connect [IP] [PORT]
    * 특정 IP와 PORT 로 접속한다.
    * 접속 시, Server 로부터 응답 메시지 출력
* save [KEY: VALUE]
    * Key를 이용해 Value 를 저장한다. ex) save BBQ 15
    * 이미 있는 Key 라면, 갱신한다.
    * 저장 시, Server로부터 응답 메시지 출력
* read [KEY]
    * Server 로부터 해당 Key의 Value 를 요청한다.
    * Server로부터 응답 메시지 출력
    * 없으면, error 메시지 출력
* close
    * 연결을 종료한다.
    * Client Shell 은 종료되지 않는다.
    * Server는 종료되지 않는다.
* exit
    * 접속 종료 신호를 보낸다.
    * Client Shell 안전하게 종료
    * Server 는 종료되지 않는다.