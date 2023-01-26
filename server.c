#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <mysql.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int listen_sock, conn_sock;
struct sockaddr_in server;
struct sockaddr_in client;
int sin_size;
int bytes_sent, bytes_received;
fd_set rset;

// truyen thong tin cho tat ca
void broadcast_msg_to_all(char *msg) {
  for (int i = 0; i < online_number; i++) {
    sendData(online_player_list[i].socket, msg);
  }
}

// truyen thong tin nguoi choi
void broadcast_playerinfo(playerinfo player, int socket) {
  // printf("%s\n", msg);
  for (int i = 0; i < online_number; i++) {
    if (online_player_list[i].socket == socket)
      continue;
    write(online_player_list[i].socket, &player, sizeof(player));
  }
}

// truyen thong tin nguoi choi cho tat ca client
void broadcast_playerinfo_to_all(playerinfo player) {
  // printf("%s\n", msg);
  for (int i = 0; i < online_number; i++) {
    write(online_player_list[i].socket, &player, sizeof(player));
  }
}

// tao 1 luong moi voi moi client ket noi
void createThread(int conn_sock) {
  int bytes_sent, bytes_received;
  printf("%d\n", conn_sock);
  // authentication
  while (1) {
    char username[50];
    char password[50];
    // char last_sign_in_try[50];
    // char signed_in[50];
    // int password_error = 0;
    char result[50];
    bytes_received = recv(conn_sock, username, 50, 0);
    username[bytes_received] = '\0';
    // printf("username: %s\n", username);
    bytes_sent = send(conn_sock, "received", 8, 0);
    // if (bytes_sent <= 0) {
    //   printf("\nConnection closed!\n");
    // }
    bytes_received = recv(conn_sock, password, 50, 0);
    password[bytes_received] = '\0';
    // printf("password: %s\n", password);
    int count = 0;
    if (checkLogin(username) == 0) {
      for (int i = 0; i < number_of_info; i++) {
        if (strcmp(username, player_list[i].username) == 0 &&
            strcmp(password, player_list[i].password) == 0) {
          strcpy(result, "Hello\0");
          count++;
          strcpy(online_player_list[online_number].username, username);
          strcpy(online_player_list[online_number].password, password);
          online_player_list[online_number].rank = player_list[i].rank;
          online_player_list[online_number].socket = conn_sock;
          online_player_list[online_number].correct = 0;
          online_number++;
        }
      }
      if (count == 0)
        strcpy(result, "Wrong username or password\0");
    } else {
      strcpy(result, "This account has already been logged in");
    }
    if (sendData(conn_sock, result) == 0)
      break;
    // if (bytes_sent <= 0) {
    //   printf("\nConnection closed!\n");
    // }

    char buff[20];
    if (count > 0) {
      recv(conn_sock, buff, 10, 0);
      int converted_number = htonl(room_number);
      bytes_sent =
          write(conn_sock, &converted_number, sizeof(converted_number));
      for (int i = 0; i < room_number; i++) {
        write(conn_sock, &roomlist[i], sizeof(room));
      }
      break;
    }
  }
  // printf("Done with authentication\n");
  // Nhan thong tin tu client de xu ly
  while (1) {
    char buff[20];
    if (receiveData(conn_sock, buff) == 0)
      break;
    printf("from client: %s\n", buff);
    if (strcmp(buff, "CREATE_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char username[20];
      strcpy(username, buff);
      printf("%s\n", username);
      printf("%s\n", player_list[findUser(username)].username);
      createRoom(online_player_list[findUser(username)], room_number);
      printf("%d\n", room_number);
      if (sendData(conn_sock, "CREATE_ROOM_SUCCESSFULLY") == 0)
        break;
      broadcast_msg("CREATE_ROOM_SUCCESSFULLY", conn_sock);
      write(conn_sock, &online_player_list[findUser(username)],
            sizeof(playerinfo));
      broadcast_playerinfo(online_player_list[findUser(username)], conn_sock);
    } else if (strcmp(buff, "DELETE_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      deleteRoom(buff);
      broadcast_msg("DELETE_ROOM_SUCCESSFULLY", conn_sock);
      if (sendData(conn_sock, "DELETE_ROOM_SUCCESSFULLY") == 0)
        break;
      broadcast_msg(buff, conn_sock);
      if (sendData(conn_sock, buff) == 0)
        break;
    } else if (strcmp(buff, "JOIN_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char host[20];
      strcpy(host, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      if (roomlist[findRoomByHost(host)].player_number >= 20) {
        printf("Room is full!\n");
        continue;
      }
      printf("%s\n", buff);
      addPlayerToWaitingList(online_player_list[findUser(buff)],
                             findRoomByHost(host));
      if (sendData(online_player_list[findUser(host)].socket, "JOIN_ROOM") == 0)
        break;
      write(online_player_list[findUser(host)].socket,
            &online_player_list[findUser(buff)], sizeof(playerinfo));
    } else if (strcmp(buff, "CANCEL_JOIN_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char username[20];
      strcpy(username, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      removePlayerFromWaitingList(username, findRoomByHost(buff));
      if (sendData(online_player_list[findUser(buff)].socket,
                   "CANCEL_JOIN_ROOM") == 0)
        break;
      if (sendData(online_player_list[findUser(buff)].socket, username) == 0)
        break;
    } else if (strcmp(buff, "ACCEPT_JOIN_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char joining_player[20];
      strcpy(joining_player, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      addPlayerToRoom(online_player_list[findUser(joining_player)],
                      findRoomByHost(buff));
      removePlayerFromWaitingList(joining_player, findRoomByHost(buff));
      broadcast_msg_to_all("ACCEPT_JOIN_ROOM_SUCCESSFULLY");
      broadcast_playerinfo_to_all(online_player_list[findUser(joining_player)]);
      broadcast_playerinfo_to_all(online_player_list[findUser(buff)]);
    } else if (strcmp(buff, "REFUSE_JOIN_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char joining_player[20];
      strcpy(joining_player, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      removePlayerFromWaitingList(joining_player, findRoomByHost(buff));
      broadcast_msg_to_all("REFUSE_JOIN_ROOM_SUCCESSFULLY");
      broadcast_msg_to_all(joining_player);
    } else if (strcmp(buff, "START_GAME") == 0) {
      printf("%d\n", room_number);
      if (receiveData(conn_sock, buff) == 0)
        break;
      roomlist[findRoomByHost(buff)].on_going_number =
          roomlist[findRoomByHost(buff)].player_number;
      // roomlist[findRoomByHost(buff)].state = 0;
      for (int i = 0; i < roomlist[findRoomByHost(buff)].player_number; i++) {
        if (sendData(roomlist[findRoomByHost(buff)].list[i].socket,
                     "START_GAME_SUCCESSFULLY") == 0)
          break;
        write(roomlist[findRoomByHost(buff)].list[i].socket, &questions[0],
              sizeof(question));
      }
      printf("waiting\n");
      // bytes_received = recv(conn_sock, buff, 6, 0);
      // buff[bytes_received] = '\0';
      // printf("%s\n", buff);
    } else if (strcmp(buff, "READY_TO_ANSWER") == 0) {
      printf("%d\n", room_number);
      // struct timeval tv;
      // tv.tv_sec = 10;
      // tv.tv_usec = 0;
      // setsockopt(conn_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
      //            sizeof tv);
      // clock_t t;
      int n = receiveData(conn_sock, buff);
      printf("%s\n", buff);
      char ans[10];
      strcpy(ans, buff);
      // char *eptr;
      // double result;
      // if (strcmp(ans, "timeout") != 0) {
      //   receiveData(conn_sock, buff);
      //   result = strtod(buff, &eptr);
      // }

      // printf("%s\n", buff);
      // printf("%f\n", result);
      int ques_no;
      bytes_received = read(conn_sock, &ques_no, sizeof(ques_no));
      if (bytes_received <= 0) {
        printf("\nConnection closed!\n");
      }
      ques_no = ntohl(ques_no);
      recv(conn_sock, buff, 8, 0);
      if (n == 0) {
        if (sendData(conn_sock, "WRONG_ANSWER") == 0)
          break;
      } else {
        // if (strcmp(ans, "timeout") != 0)
        //   sleep(result);
        if ((strcmp(ans, "PickA") == 0 &&
             strcmp(questions[ques_no].answer[0],
                    questions[ques_no].correct_answer) == 0) ||
            (strcmp(ans, "PickB") == 0 &&
             strcmp(questions[ques_no].answer[1],
                    questions[ques_no].correct_answer) == 0) ||
            (strcmp(ans, "PickC") == 0 &&
             strcmp(questions[ques_no].answer[2],
                    questions[ques_no].correct_answer) == 0)) {
          if (sendData(conn_sock, "CORRECT_ANSWER") == 0)
            break;
          ques_no++;
          write(conn_sock, &questions[ques_no], sizeof(question));
          addPoint(conn_sock);
        } else {
          if (sendData(conn_sock, "WRONG_ANSWER") == 0)
            break;
          roomlist[findRoomBySocket(conn_sock)].on_going_number--;
          // printf("%d %s\n",
          //        roomlist[findRoomBySocket(conn_sock)].on_going_number,
          //        roomlist[findRoomBySocket(conn_sock)].list[0].username);
          if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
            for (int i = 0;
                 i < roomlist[findRoomBySocket(conn_sock)].player_number; i++) {
              if (sendData(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                           "END_GAME") == 0)
                break;
              write(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                    &roomlist[findRoomBySocket(conn_sock)], sizeof(room));
              roomlist[findRoomBySocket(conn_sock)].list[i].correct = 0;
            }
            deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
          }
        }
      }
    } else if (strcmp(buff, "FINISH") == 0) {
      roomlist[findRoomBySocket(conn_sock)].on_going_number--;
      if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
        for (int i = 0; i < roomlist[findRoomBySocket(conn_sock)].player_number;
             i++) {
          if (sendData(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                       "END_GAME") == 0)
            break;
          write(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                &roomlist[findRoomBySocket(conn_sock)], sizeof(room));
          roomlist[findRoomBySocket(conn_sock)].list[i].correct = 0;
        }
        deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
      }
    }
  }
}
void main() {
  MYSQL *con = mysql_init(NULL);
  if (con == NULL) {
    fprintf(stderr, "%s\n", mysql_error(con));
    exit(1);
  }

  if (mysql_real_connect(con, "localhost", "root", "root", "golden-bell", 0,
                         NULL, 0) == NULL) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  if (mysql_query(con, "SELECT * FROM players")) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  MYSQL_RES *result = mysql_store_result(con);
  int num_fields = mysql_num_fields(result);

  MYSQL_ROW row;
  // luu thong tin tu database
  while ((row = mysql_fetch_row(result))) {
    unsigned long *lengths;
    lengths = mysql_fetch_lengths(result);
    strcpy(player_list[number_of_info].username, row[1]);
    strcpy(player_list[number_of_info].password, row[2]);
    player_list[number_of_info].rank = atoi(row[3]);
    number_of_info++;
  }

  if (mysql_query(con,
                  "SELECT questions.content, answer1.content, answer2.content\n"
                  "FROM questions\n"
                  "INNER JOIN answers answer1 on "
                  "questions.question_id = answer1.question_id\n"
                  "INNER JOIN answers answer2 on "
                  "questions.answer_id = answer2.answer_id")) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  result = mysql_store_result(con);
  num_fields = mysql_num_fields(result);

  while ((row = mysql_fetch_row(result))) {
    if (question_number % 3 == 0) {
      strcpy(questions[question_number / 3].content, row[0]);
      strcpy(questions[question_number / 3].correct_answer, row[2]);
    }
    strcpy(questions[question_number / 3].answer[question_number % 3], row[1]);
    question_number++;
  }

  mysql_close(con);

  if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) ==
      -1) { /* calls socket() */
    printf("socket() error\n");
    return;
  }

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(5551);
  server.sin_addr.s_addr =
      htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

  if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
    perror("\nError: ");
    return;
  }

  if (listen(listen_sock, 20) == -1) {
    perror("\nError: ");
    return;
  }

  /* Establish a signal handler to catch SIGCHLD */
  signal(SIGCHLD, sig_chld);

  while (1) {
    sin_size = sizeof(struct sockaddr_in);
    if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client,
                            &sin_size)) == -1) {
      if (errno == EINTR)
        continue;
      else {
        perror("\nError: ");
        return;
      }
    }

    pthread_t id;
    pthread_create(&id, NULL, (void *)createThread,
                   (void *)(intptr_t)conn_sock);
  }
  /* The parent closes the connected socket since the child handles the new
   * client */
  close(conn_sock);
  close(listen_sock);
  return;
}
