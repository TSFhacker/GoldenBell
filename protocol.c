#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int online_number = 0;
int room_number = 0;
int number_of_info = 0;
room roomlist[50];
playerinfo player_list[100];
playerinfo online_player_list[20];
question questions[30];
question intermediate_questions[30];
question advanced_questions[30];
int question_number = 0;
int current_question = 0;

int receiveData(int socket, char *buffer) {
  int n;
  int bytes_received;
  bytes_received = read(socket, &n, sizeof(n));
  if (bytes_received <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  n = ntohl(n);
  bytes_received = recv(socket, buffer, n, 0);
  buffer[bytes_received] = '\0';
  if (bytes_received <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  return 1;
}

int sendData(int socket, char *msg) {
  int bytes_sent;
  int n = htonl(strlen(msg));
  bytes_sent = write(socket, &n, sizeof(n));
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  bytes_sent = send(socket, msg, strlen(msg), 0);
  if (bytes_sent <= 0) {
    printf("\nConnection closed!\n");
    return 0;
  }
  return 1;
}

void broadcast_msg(char *msg, int socket) {
  for (int i = 0; i < online_number; i++) {
    if (online_player_list[i].socket == socket)
      continue;
    sendData(online_player_list[i].socket, msg);
  }
}

int checkLogin(char *username) {
  for (int i = 0; i < online_number; i++) {
    if (strcmp(online_player_list[i].username, username) == 0) {
      return 1;
    }
  }
  return 0;
}

int findUser(char *username) {
  for (int i = 0; i < online_number; i++) {
    if (strcmp(online_player_list[i].username, username) == 0) {
      return i;
    }
  }
  return -1; // khong tim duoc user
}

int findUserBySocket(int socket) {
  for (int i = 0; i < online_number; i++) {
    if (online_player_list[i].socket == socket) {
      return i;
    }
  }
  return -1; // khong tim duoc user
}

void addPoint(int socket) {
  for (int i = 0; i < room_number; i++) {
    for (int j = 0; j < roomlist[i].player_number; j++) {
      if (roomlist[i].list[j].socket == socket) {
        roomlist[i].list[j].correct++;
      }
    }
  }
}

int findRoomByHost(char *username) {
  for (int i = 0; i < room_number; i++) {
    if (strcmp(roomlist[i].list[0].username, username) == 0) {
      return i;
    }
  }
  return -1; // khong tim duoc phong
}

int findRoomBySocket(int socket) {
  for (int i = 0; i < room_number; i++) {
    for (int j = 0; j < roomlist[i].player_number; j++) {
      if (roomlist[i].list[j].socket == socket) {
        return i;
      }
    }
  }
  return -1; // khong tim duoc phong
}

void createRoom(playerinfo player, int roomnumber) {
  strcpy(roomlist[room_number].list[0].username, player.username);
  strcpy(roomlist[room_number].list[0].password, player.password);
  roomlist[room_number].list[0].rank = player.rank;
  roomlist[room_number].list[0].socket = player.socket;
  roomlist[room_number].rank = player.rank;
  roomlist[room_number].state = 1;
  roomlist[room_number].waiting_number = 0;
  roomlist[room_number].player_number = 1;
  roomlist[room_number].different_points_number = 0;
  roomlist[room_number].id = roomnumber;
  room_number++;
}

void addPlayerToRoom(playerinfo player, int roomid) {
  strcpy(roomlist[roomid].list[roomlist[roomid].player_number].username,
         player.username);
  strcpy(roomlist[roomid].list[roomlist[roomid].player_number].password,
         player.password);
  roomlist[roomid].list[roomlist[roomid].player_number].rank = player.rank;
  roomlist[roomid].list[roomlist[roomid].player_number].socket = player.socket;
  roomlist[roomid].player_number++;
}

void removePlayerFromRoom(char *username, int roomid) {
  for (int i = 0; i < roomlist[roomid].player_number; i++) {
    if (strcmp(username, roomlist[roomid].list[i].username) == 0) {
      if (i != roomlist[roomid].player_number - 1) {
        for (int j = i; j < roomlist[roomid].player_number - 1; j++) {
          strcpy(roomlist[roomid].list[j].username,
                 roomlist[roomid].list[j + 1].username);
          strcpy(roomlist[roomid].list[j].password,
                 roomlist[roomid].list[j + 1].password);
          roomlist[roomid].list[j].rank = roomlist[roomid].list[j + 1].rank;
          roomlist[roomid].list[j].socket = roomlist[roomid].list[j + 1].socket;
          roomlist[roomid].list[j].correct =
              roomlist[roomid].list[j + 1].correct;
          roomlist[roomid].list[j].point = roomlist[roomid].list[j + 1].point;
        }
      }
      roomlist[roomid].player_number--;
      break;
    }
  }
}

void deleteRoom(char *username) {
  for (int i = 0; i < room_number; i++) {
    printf("%s %s\n", username, roomlist[i].list[0].username);
    if (strcmp(username, roomlist[i].list[0].username) == 0) {
      if (i != room_number - 1) {
        for (int j = i; j < room_number - 1; j++) {
          for (int k = 0; k < roomlist[j + 1].player_number; k++) {
            strcpy(roomlist[j].list[k].username,
                   roomlist[j + 1].list[k].username);
            strcpy(roomlist[j].list[k].password,
                   roomlist[j + 1].list[k].password);
            roomlist[j].list[k].rank = roomlist[j + 1].list[k].rank;
            roomlist[j].list[k].socket = roomlist[j + 1].list[k].socket;
          }
          for (int k = 0; k < roomlist[j + 1].waiting_number; k++) {
            strcpy(roomlist[j].waiting_list[k].username,
                   roomlist[j + 1].waiting_list[k].username);
            strcpy(roomlist[j].waiting_list[k].password,
                   roomlist[j + 1].waiting_list[k].password);
            roomlist[j].waiting_list[k].rank =
                roomlist[j + 1].waiting_list[k].rank;
            roomlist[j].waiting_list[k].socket =
                roomlist[j + 1].waiting_list[k].socket;
          }
          for (int k = 0; k < roomlist[j + 1].different_points_number; k++)
            roomlist[j].pointlist[k] = roomlist[j + 1].pointlist[k];
          roomlist[j].rank = roomlist[j + 1].rank;
          roomlist[j].state = roomlist[j + 1].state;
          roomlist[j].player_number = roomlist[j + 1].player_number;
          roomlist[j].waiting_number = roomlist[j + 1].waiting_number;
          roomlist[j].on_going_number = roomlist[j + 1].on_going_number;
          roomlist[j].different_points_number =
              roomlist[j + 1].different_points_number;
        }
      }
      room_number--;
      break;
    }
  }
}

void addPlayerToWaitingList(playerinfo player, int roomid) {
  strcpy(
      roomlist[roomid].waiting_list[roomlist[roomid].waiting_number].username,
      player.username);
  strcpy(
      roomlist[roomid].waiting_list[roomlist[roomid].waiting_number].password,
      player.password);
  roomlist[roomid].waiting_list[roomlist[roomid].waiting_number].rank =
      player.rank;
  roomlist[roomid].waiting_list[roomlist[roomid].waiting_number].socket =
      player.socket;
  roomlist[roomid].waiting_number++;
}

void removePlayerFromWaitingList(char *username, int roomid) {
  for (int i = 0; i < roomlist[roomid].waiting_number; i++) {
    if (strcmp(username, roomlist[roomid].waiting_list[i].username) == 0) {
      if (i != roomlist[roomid].waiting_number - 1) {
        for (int j = i; j < roomlist[roomid].waiting_number - 1; j++) {
          strcpy(roomlist[roomid].waiting_list[j].username,
                 roomlist[roomid].waiting_list[j + 1].username);
          strcpy(roomlist[roomid].waiting_list[j].password,
                 roomlist[roomid].waiting_list[j + 1].password);
          roomlist[roomid].waiting_list[j].rank =
              roomlist[roomid].waiting_list[j + 1].rank;
          roomlist[roomid].waiting_list[j].socket =
              roomlist[roomid].waiting_list[j + 1].socket;
        }
      }
      roomlist[roomid].waiting_number--;
      break;
    }
  }
}

int calculatePoint(int roomid) {
  int count = 1;
  int count2 = 0;
  for (int i = 0; i < roomlist[roomid].player_number; i++) {
    if (i == 0)
      roomlist[roomid].pointlist[i] = roomlist[roomid].list[i].correct;
    else {
      for (int j = 0; j < count; j++) {
        if (roomlist[roomid].pointlist[j] == roomlist[roomid].list[i].correct) {
          count2++;
          break;
        }
      }
      if (count2 == 0) {
        roomlist[roomid].pointlist[count] = roomlist[roomid].list[i].correct;
        count++;
      }
    }
  }
  int temp;
  int i, j;
  for (i = 0; i < count - 1; i++) {
    for (j = 0; j < count - 1 - i; j++) {
      if (roomlist[roomid].pointlist[j] > roomlist[roomid].pointlist[j + 1]) {
        temp = roomlist[roomid].pointlist[j];
        roomlist[roomid].pointlist[j] = roomlist[roomid].pointlist[j + 1];
        roomlist[roomid].pointlist[j + 1] = temp;
      }
    }
  }
  roomlist[roomid].different_points_number = count;
  return count;
}
