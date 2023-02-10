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
char prvs_mess[50];

// truyen thong tin cho tat ca
void broadcast_msg_to_all(char *msg) {
  for (int i = 0; i < online_number; i++) {
    sendData(online_player_list[i].socket, msg);
  }
}

// truyen thong tin nguoi choi
void broadcast_playerinfo(playerinfo player, int socket) {
  for (int i = 0; i < online_number; i++) {
    if (online_player_list[i].socket == socket)
      continue;
    write(online_player_list[i].socket, &player, sizeof(player));
  }
}

// truyen thong tin nguoi choi cho tat ca client
void broadcast_playerinfo_to_all(playerinfo player) {
  for (int i = 0; i < online_number; i++) {
    write(online_player_list[i].socket, &player, sizeof(player));
  }
}

// tao 1 luong moi voi moi client ket noi
void createThread(int conn_sock) {
  pthread_detach(pthread_self());
  int bytes_sent, bytes_received;
  char buff[100];
  while (1) {
    if (receiveData(conn_sock, buff) == 0)
      pthread_exit(NULL);
    printf("From client: %s\n", buff);
    if (strcmp(buff, "LOG_IN") == 0) {
      char username[50];
      char password[50];
      char result[50];
      bytes_received = recv(conn_sock, username, 50, 0);
      username[bytes_received] = '\0';
      bytes_sent = send(conn_sock, "received", 8, 0);
      bytes_received = recv(conn_sock, password, 50, 0);
      password[bytes_received] = '\0';
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

      char buff[20];
      if (count > 0) {
        recv(conn_sock, buff, 10, 0);
        int converted_number = htonl(room_number);
        bytes_sent =
            write(conn_sock, &converted_number, sizeof(converted_number));
        for (int i = 0; i < room_number; i++) {
          write(conn_sock, &roomlist[i], sizeof(room));
        }
        converted_number = htonl(online_player_list[findUser(username)].rank);
        write(conn_sock, &converted_number, sizeof(converted_number));
        break;
      }
    } else if (strcmp(buff, "SIGN_UP") == 0) {
      char username[50];
      char password[50];
      char result[50];
      if (receiveData(conn_sock, buff) == 0)
        break;
      strcpy(username, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      strcpy(password, buff);
      int count = 0;
      for (int i = 0; i < number_of_info; i++) {
        if (strcmp(username, player_list[i].username) == 0) {
          strcpy(result, "This account name has already existed");
          count++;
          if (sendData(conn_sock, result) == 0)
            break;
          break;
        }
      }
      if (count == 0) {
        strcpy(result, "Sign up successfully");
        strcpy(player_list[number_of_info].username, username);
        strcpy(player_list[number_of_info].password, password);
        player_list[number_of_info].rank = 0;
        player_list[number_of_info].correct = 0;
        number_of_info++;
        MYSQL *con = mysql_init(NULL);
        if (con == NULL) {
          fprintf(stderr, "%s\n", mysql_error(con));
          exit(1);
        }

        if (mysql_real_connect(con, "localhost", "root", "root", "golden-bell",
                               0, NULL, 0) == NULL) {
          fprintf(stderr, "%s\n", mysql_error(con));
          mysql_close(con);
          exit(1);
        }
        char query[500];
        sprintf(query,
                "INSERT INTO players (account, password, player_rank) VALUES "
                "('%s', "
                "'%s', 0)",
                username, password);
        if (mysql_query(con, query)) {
          fprintf(stderr, "%s\n", mysql_error(con));
          mysql_close(con);
          exit(1);
        }
        mysql_close(con);
        if (sendData(conn_sock, result) == 0)
          break;
      }
    }
  }
  // Nhan thong tin tu client de xu ly
  while (1) {
    if (receiveData(conn_sock, buff) == 0) {
      if (strcmp(prvs_mess, "READY_TO_ANSWER") == 0) {
        printf("Handling disconnection\n");
        roomlist[findRoomBySocket(conn_sock)].on_going_number--;
        MYSQL *con = mysql_init(NULL);
        if (con == NULL) {
          fprintf(stderr, "%s\n", mysql_error(con));
          exit(1);
        }

        if (mysql_real_connect(con, "localhost", "root", "root", "golden-bell",
                               0, NULL, 0) == NULL) {
          fprintf(stderr, "%s\n", mysql_error(con));
          mysql_close(con);
          exit(1);
        }
        char query[500];
        sprintf(query,
                "UPDATE players SET player_rank = %d WHERE (account = '%s')",
                online_player_list[findUserBySocket(conn_sock)].rank - 10,
                online_player_list[findUserBySocket(conn_sock)].username);
        if (mysql_query(con, query)) {
          fprintf(stderr, "%s\n", mysql_error(con));
          mysql_close(con);
          exit(1);
        }
        mysql_close(con);
        for (int i = 0; i < number_of_info; i++) {
          if (strcmp(online_player_list[findUserBySocket(conn_sock)].username,
                     player_list[i].username) == 0)
            player_list[i].rank -= 10;
        }
        broadcast_msg_to_all("LEAVE_GAME_SUCCESSFULLY");
        broadcast_msg_to_all(
            online_player_list[findUserBySocket(conn_sock)].username);
        if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
          broadcast_msg_to_all(
              roomlist[findRoomBySocket(conn_sock)].list[0].username);
          int converted_number =
              htonl(roomlist[findRoomBySocket(conn_sock)].on_going_number);
          for (int i = 0; i < online_number; i++)
            write(online_player_list[i].socket, &converted_number,
                  sizeof(converted_number));
          deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
        } else {
          broadcast_msg_to_all(
              roomlist[findRoomBySocket(conn_sock)].list[0].username);
          int converted_number =
              htonl(roomlist[findRoomBySocket(conn_sock)].on_going_number);
          for (int i = 0; i < online_number; i++)
            write(online_player_list[i].socket, &converted_number,
                  sizeof(converted_number));
          removePlayerFromRoom(
              online_player_list[findUserBySocket(conn_sock)].username,
              findRoomBySocket(conn_sock));
        }
      } else {
        deleteRoom(online_player_list[findRoomBySocket(conn_sock)].username);
        for (int i = 0; i < room_number; i++) {
          if (conn_sock == roomlist[i].list[0].socket)
            continue;
          removePlayerFromRoom(
              online_player_list[findUserBySocket(conn_sock)].username, i);
          removePlayerFromWaitingList(
              online_player_list[findUserBySocket(conn_sock)].username, i);
        }
      }
      broadcast_msg_to_all("DISCONNECTED");
      broadcast_msg_to_all(
          online_player_list[findUserBySocket(conn_sock)].username);
      for (int i = 0; i < online_number; i++) {
        if (conn_sock == online_player_list[i].socket) {
          if (i != online_number - 1) {
            for (int j = i; j < online_number - 1; j++) {
              strcpy(online_player_list[j].username,
                     online_player_list[j + 1].username);
              strcpy(online_player_list[j].password,
                     online_player_list[j + 1].password);
              online_player_list[j].rank = online_player_list[j + 1].rank;
              online_player_list[j].socket = online_player_list[j + 1].socket;
              online_player_list[j].correct = online_player_list[j + 1].correct;
            }
          }
          online_number--;
          break;
        }
      }
      break;
    }
    strcpy(prvs_mess, buff);
    printf("From client: %s\n", buff);
    if (strcmp(buff, "CREATE_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char username[20];
      strcpy(username, buff);
      createRoom(online_player_list[findUser(username)], room_number);
      if (sendData(conn_sock, "CREATE_ROOM_SUCCESSFULLY") == 0)
        break;
      broadcast_msg("CREATE_ROOM_SUCCESSFULLY", conn_sock);
      write(conn_sock, &online_player_list[findUser(username)],
            sizeof(playerinfo));
      broadcast_playerinfo(online_player_list[findUser(username)], conn_sock);
    } else if (strcmp(buff, "LOG_OUT") == 0) {
      if (sendData(conn_sock, "LOG_OUT_SUCCESSFULLY") == 0)
        break;
      for (int i = 0; i < online_number; i++) {
        if (conn_sock == online_player_list[i].socket) {
          if (i != online_number - 1) {
            for (int j = i; j < online_number - 1; j++) {
              strcpy(online_player_list[j].username,
                     online_player_list[j + 1].username);
              strcpy(online_player_list[j].password,
                     online_player_list[j + 1].password);
              online_player_list[j].rank = online_player_list[j + 1].rank;
              online_player_list[j].socket = online_player_list[j + 1].socket;
              online_player_list[j].correct = online_player_list[j + 1].correct;
            }
          }
          online_number--;
          break;
        }
      }
      break;
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
        if (sendData(conn_sock, "CAN_NOT_JOIN") == 0)
          break;
        if (sendData(conn_sock, "Room is full") == 0)
          break;
        continue;
      } else if (online_player_list[findUser(buff)].rank >
                 roomlist[findRoomByHost(host)].rank) {
        if (sendData(conn_sock, "CAN_NOT_JOIN") == 0)
          break;
        if (sendData(conn_sock, "Room rank is below yours") == 0)
          break;
        continue;
      } else if (online_player_list[findUser(buff)].rank <=
                 roomlist[findRoomByHost(host)].rank - 500) {
        if (sendData(conn_sock, "CAN_NOT_JOIN") == 0)
          break;
        if (sendData(conn_sock, "This room is too advanced") == 0)
          break;
        continue;
      } else {
        if (sendData(conn_sock, "CAN_JOIN_ROOM") == 0)
          break;
        addPlayerToWaitingList(online_player_list[findUser(buff)],
                               findRoomByHost(host));
        if (sendData(online_player_list[findUser(host)].socket, "JOIN_ROOM") ==
            0)
          break;
        write(online_player_list[findUser(host)].socket,
              &online_player_list[findUser(buff)], sizeof(playerinfo));
      }

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
    } else if (strcmp(buff, "LEAVE_ROOM") == 0) {
      if (receiveData(conn_sock, buff) == 0)
        break;
      char username[20];
      strcpy(username, buff);
      if (receiveData(conn_sock, buff) == 0)
        break;
      removePlayerFromRoom(username, findRoomByHost(buff));
      if (roomlist[findRoomByHost(buff)].state == 0)
        roomlist[findRoomByHost(buff)].on_going_number--;
      broadcast_msg_to_all("LEAVE_ROOM_SUCCESSFULLY");
      broadcast_msg_to_all(username);
      broadcast_msg_to_all(buff);
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
      if (receiveData(conn_sock, buff) == 0)
        break;
      roomlist[findRoomByHost(buff)].on_going_number =
          roomlist[findRoomByHost(buff)].player_number;
      for (int i = 0; i < roomlist[findRoomByHost(buff)].player_number; i++) {
        broadcast_msg_to_all("START_GAME_SUCCESSFULLY");
        broadcast_msg_to_all(buff);
        if (sendData(roomlist[findRoomByHost(buff)].list[i].socket,
                     "PLAYING") == 0)
          break;
        if (roomlist[findRoomByHost(buff)].rank <= 500)
          write(roomlist[findRoomByHost(buff)].list[i].socket, &questions[0],
                sizeof(question));
        else if (roomlist[findRoomByHost(buff)].rank > 500 &&
                 roomlist[findRoomByHost(buff)].rank <= 2000)
          write(roomlist[findRoomByHost(buff)].list[i].socket,
                &intermediate_questions[0], sizeof(question));
        else
          write(roomlist[findRoomByHost(buff)].list[i].socket,
                &advanced_questions[0], sizeof(question));
        roomlist[findRoomBySocket(conn_sock)].list[i].correct = 0;
      }
    } else if (strcmp(buff, "READY_TO_ANSWER") == 0) {
      int n = receiveData(conn_sock, buff);
      char ans[10];
      strcpy(ans, buff);
      int ques_no;
      bytes_received = read(conn_sock, &ques_no, sizeof(ques_no));
      if (bytes_received <= 0) {
        continue;
      }
      ques_no = ntohl(ques_no);
      if (strcmp(ans, "leaving") != 0) {
        if (receiveData(conn_sock, buff) == 0)
          continue;
        if (strcmp(buff, "leaving") == 0) {
          bytes_received = read(conn_sock, &ques_no, sizeof(ques_no));
          if (bytes_received <= 0) {
            continue;
          }
          ques_no = ntohl(ques_no);
        }
      }

      if (n == 0) {
        continue;
      } else {
        char correct_answer[100];
        char answer1[100];
        char answer2[100];
        char answer3[100];
        if (roomlist[findRoomBySocket(conn_sock)].rank <= 500) {
          strcpy(correct_answer, questions[ques_no].correct_answer);
          strcpy(answer1, questions[ques_no].answer[0]);
          strcpy(answer2, questions[ques_no].answer[1]);
          strcpy(answer3, questions[ques_no].answer[2]);
        } else if (roomlist[findRoomBySocket(conn_sock)].rank > 500 &&
                   roomlist[findRoomBySocket(conn_sock)].rank <= 2000) {
          strcpy(correct_answer,
                 intermediate_questions[ques_no].correct_answer);
          strcpy(answer1, intermediate_questions[ques_no].answer[0]);
          strcpy(answer2, intermediate_questions[ques_no].answer[1]);
          strcpy(answer3, intermediate_questions[ques_no].answer[2]);
        } else {
          strcpy(correct_answer, advanced_questions[ques_no].correct_answer);
          strcpy(answer1, advanced_questions[ques_no].answer[0]);
          strcpy(answer2, advanced_questions[ques_no].answer[1]);
          strcpy(answer3, advanced_questions[ques_no].answer[2]);
        }
        if (((strcmp(ans, "PickA") == 0 &&
              strcmp(answer1, correct_answer) == 0) ||
             (strcmp(ans, "PickB") == 0 &&
              strcmp(answer2, correct_answer) == 0) ||
             (strcmp(ans, "PickC") == 0 &&
              strcmp(answer3, correct_answer) == 0)) &&
            strcmp(buff, "leaving") != 0) {
          if (sendData(conn_sock, "CORRECT_ANSWER") == 0)
            break;
          ques_no++;
          if (ques_no != 20) {
            if (roomlist[findRoomBySocket(conn_sock)].rank <= 500)
              write(conn_sock, &questions[ques_no], sizeof(question));
            else if (roomlist[findRoomBySocket(conn_sock)].rank > 500 &&
                     roomlist[findRoomBySocket(conn_sock)].rank <= 2000)
              write(conn_sock, &intermediate_questions[ques_no],
                    sizeof(question));
            else
              write(conn_sock, &advanced_questions[ques_no], sizeof(question));
          }
          addPoint(conn_sock);
        } else if (strcmp(ans, "leaving") == 0 ||
                   strcmp(buff, "leaving") == 0) {
          roomlist[findRoomBySocket(conn_sock)].on_going_number--;
          MYSQL *con = mysql_init(NULL);
          if (con == NULL) {
            fprintf(stderr, "%s\n", mysql_error(con));
            exit(1);
          }

          if (mysql_real_connect(con, "localhost", "root", "root",
                                 "golden-bell", 0, NULL, 0) == NULL) {
            fprintf(stderr, "%s\n", mysql_error(con));
            mysql_close(con);
            exit(1);
          }
          char query[500];
          sprintf(query,
                  "UPDATE players SET player_rank = %d WHERE (account = '%s')",
                  online_player_list[findUserBySocket(conn_sock)].rank - 10,
                  online_player_list[findUserBySocket(conn_sock)].username);
          if (mysql_query(con, query)) {
            fprintf(stderr, "%s\n", mysql_error(con));
            mysql_close(con);
            exit(1);
          }
          mysql_close(con);
          online_player_list[findUserBySocket(conn_sock)].rank -= 10;
          for (int i = 0; i < number_of_info; i++) {
            if (strcmp(online_player_list[findUserBySocket(conn_sock)].username,
                       player_list[i].username) == 0)
              player_list[i].rank -= 10;
          }
          broadcast_msg_to_all("LEAVE_GAME_SUCCESSFULLY");
          broadcast_msg_to_all(
              online_player_list[findUserBySocket(conn_sock)].username);
          if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
            broadcast_msg_to_all(
                roomlist[findRoomBySocket(conn_sock)].list[0].username);
            int converted_number =
                htonl(roomlist[findRoomBySocket(conn_sock)].on_going_number);
            for (int i = 0; i < online_number; i++)
              write(online_player_list[i].socket, &converted_number,
                    sizeof(converted_number));
            deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
          } else {
            broadcast_msg_to_all(
                roomlist[findRoomBySocket(conn_sock)].list[0].username);
            int converted_number =
                htonl(roomlist[findRoomBySocket(conn_sock)].on_going_number);
            for (int i = 0; i < online_number; i++)
              write(online_player_list[i].socket, &converted_number,
                    sizeof(converted_number));
            removePlayerFromRoom(
                online_player_list[findUserBySocket(conn_sock)].username,
                findRoomBySocket(conn_sock));
          }

        } else {
          if (sendData(conn_sock, "WRONG_ANSWER") == 0)
            break;
          roomlist[findRoomBySocket(conn_sock)].on_going_number--;
          if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
            int point_count = calculatePoint(findRoomBySocket(conn_sock));
            for (int i = 0; i < point_count; i++)
              if (point_count == 1) {
                for (int i = 0;
                     i < roomlist[findRoomBySocket(conn_sock)].player_number;
                     i++)
                  roomlist[findRoomBySocket(conn_sock)].list[i].point = 0;
              } else if (point_count == 2) {
                for (int i = 0;
                     i < roomlist[findRoomBySocket(conn_sock)].player_number;
                     i++) {
                  if (roomlist[findRoomBySocket(conn_sock)].list[i].correct ==
                      roomlist[findRoomBySocket(conn_sock)].pointlist[0])
                    roomlist[findRoomBySocket(conn_sock)].list[i].point = -5;
                  else
                    roomlist[findRoomBySocket(conn_sock)].list[i].point = 5;
                }
              } else if (point_count >= 3 && point_count <= 9) {
                for (int i = 0;
                     i < roomlist[findRoomBySocket(conn_sock)].player_number;
                     i++) {
                  for (int j = 0; j < point_count; j++) {
                    if (roomlist[findRoomBySocket(conn_sock)].list[i].correct ==
                        roomlist[findRoomBySocket(conn_sock)].pointlist[j]) {
                      if (j <= point_count / 3 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point =
                            -5;
                      else if (j > point_count / 3 - 1 &&
                               j <= point_count * 2 / 3 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point = 0;
                      else
                        roomlist[findRoomBySocket(conn_sock)].list[i].point = 5;
                      break;
                    }
                  }
                }
              } else if (point_count >= 10 && point_count <= 20) {
                for (int i = 0;
                     i < roomlist[findRoomBySocket(conn_sock)].player_number;
                     i++) {
                  for (int j = 0; j < point_count; j++) {
                    if (roomlist[findRoomBySocket(conn_sock)].list[i].correct ==
                        roomlist[findRoomBySocket(conn_sock)].pointlist[j]) {
                      if (j <= point_count / 5 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point =
                            -10;
                      else if (j > point_count / 5 - 1 &&
                               j <= point_count * 2 / 5 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point =
                            -5;
                      else if (j > point_count * 2 / 5 - 1 &&
                               j <= point_count * 3 / 5 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point = 0;
                      else if (j > point_count * 3 / 5 - 1 &&
                               j <= point_count * 4 / 5 - 1)
                        roomlist[findRoomBySocket(conn_sock)].list[i].point = 5;
                      else
                        roomlist[findRoomBySocket(conn_sock)].list[i].point =
                            10;
                      break;
                    }
                  }
                }
              }
            MYSQL *con = mysql_init(NULL);
            if (con == NULL) {
              fprintf(stderr, "%s\n", mysql_error(con));
              exit(1);
            }

            if (mysql_real_connect(con, "localhost", "root", "root",
                                   "golden-bell", 0, NULL, 0) == NULL) {
              fprintf(stderr, "%s\n", mysql_error(con));
              mysql_close(con);
              exit(1);
            }
            for (int i = 0;
                 i < roomlist[findRoomBySocket(conn_sock)].player_number; i++) {
              char query[500];
              sprintf(query,
                      "UPDATE players SET player_rank = %d WHERE (account = "
                      "'%s')",
                      roomlist[findRoomBySocket(conn_sock)].list[i].rank +
                          roomlist[findRoomBySocket(conn_sock)].list[i].point,
                      roomlist[findRoomBySocket(conn_sock)].list[i].username);
              online_player_list[findUser(roomlist[findRoomBySocket(conn_sock)]
                                              .list[i]
                                              .username)]
                  .rank += roomlist[findRoomBySocket(conn_sock)].list[i].point;
              for (int i = 0; i < number_of_info; i++) {
                if (roomlist[findRoomBySocket(conn_sock)].list[i].username,
                    player_list[i].username == 0)
                  player_list[i].rank +=
                      roomlist[findRoomBySocket(conn_sock)].list[i].point;
              }
              if (mysql_query(con, query)) {
                fprintf(stderr, "%s\n", mysql_error(con));
                mysql_close(con);
                exit(1);
              }
              if (sendData(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                           "END_GAME") == 0)
                break;
              write(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                    &roomlist[findRoomBySocket(conn_sock)], sizeof(room));
            }
            broadcast_msg_to_all("DELETE_ROOM_SUCCESSFULLY");
            broadcast_msg_to_all(
                roomlist[findRoomBySocket(conn_sock)].list[0].username);
            deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
            mysql_close(con);
          }
        }
      }
    } else if (strcmp(buff, "FINISH") == 0) {
      printf("Someone has finished the game\n");
      roomlist[findRoomBySocket(conn_sock)].on_going_number--;
      printf("On going: %d\n",
             roomlist[findRoomBySocket(conn_sock)].on_going_number);
      if (roomlist[findRoomBySocket(conn_sock)].on_going_number == 0) {
        for (int i = 0; i < roomlist[findRoomBySocket(conn_sock)].player_number;
             i++) {
          if (sendData(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                       "END_GAME") == 0)
            break;
          write(roomlist[findRoomBySocket(conn_sock)].list[i].socket,
                &roomlist[findRoomBySocket(conn_sock)], sizeof(room));
          printf("DOne sending\n");
        }
        broadcast_msg_to_all("DELETE_ROOM_SUCCESSFULLY");
        broadcast_msg_to_all(
            roomlist[findRoomBySocket(conn_sock)].list[0].username);
        deleteRoom(roomlist[findRoomBySocket(conn_sock)].list[0].username);
      }
    }
  }
  printf("End thread\n");
  pthread_exit(NULL);
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
                  "questions.answer_id = answer2.answer_id\n"
                  "WHERE questions.difficulty = 1")) {
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

  question_number = 0;
  if (mysql_query(con,
                  "SELECT questions.content, answer1.content, answer2.content\n"
                  "FROM questions\n"
                  "INNER JOIN answers answer1 on "
                  "questions.question_id = answer1.question_id\n"
                  "INNER JOIN answers answer2 on "
                  "questions.answer_id = answer2.answer_id\n"
                  "WHERE questions.difficulty = 2")) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  result = mysql_store_result(con);
  num_fields = mysql_num_fields(result);

  while ((row = mysql_fetch_row(result))) {
    if (question_number % 3 == 0) {
      strcpy(intermediate_questions[question_number / 3].content, row[0]);
      strcpy(intermediate_questions[question_number / 3].correct_answer,
             row[2]);
    }
    strcpy(
        intermediate_questions[question_number / 3].answer[question_number % 3],
        row[1]);
    question_number++;
  }

  question_number = 0;
  if (mysql_query(con,
                  "SELECT questions.content, answer1.content, answer2.content\n"
                  "FROM questions\n"
                  "INNER JOIN answers answer1 on "
                  "questions.question_id = answer1.question_id\n"
                  "INNER JOIN answers answer2 on "
                  "questions.answer_id = answer2.answer_id\n"
                  "WHERE questions.difficulty = 3")) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
  }

  result = mysql_store_result(con);
  num_fields = mysql_num_fields(result);

  while ((row = mysql_fetch_row(result))) {
    if (question_number % 3 == 0) {
      strcpy(advanced_questions[question_number / 3].content, row[0]);
      strcpy(advanced_questions[question_number / 3].correct_answer, row[2]);
    }
    strcpy(advanced_questions[question_number / 3].answer[question_number % 3],
           row[1]);
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
  server.sin_port = htons(5550);
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
