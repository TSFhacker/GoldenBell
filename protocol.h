#ifndef __PROTOCOL__
#define __PROTOCOL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct player {
  char username[50];
  char password[50];
  int rank;
  int socket;
  int correct;
} playerinfo;

typedef struct room {
  playerinfo list[20];
  playerinfo waiting_list[20];
  int waiting_number;
  int player_number;
  int on_going_number;
  int rank;
  int id;
  int state; // 0 la dang choi, 1 la chua choi
} room;

typedef struct question {
  char content[100];
  char answer[3][100];
  char correct_answer[100];
} question;

extern int online_number;
extern int room_number;
extern room roomlist[50];
extern playerinfo player_list[100];
extern int number_of_info;
extern playerinfo online_player_list[20];
extern question questions[30];
extern int question_number;
extern int current_question;

typedef struct data {
  int i;
  char c;
  char *s;
} Data;

typedef enum {
  LOGIN,    //đăng nhập
  REGISTER, //đăng ký
  LOGOUT,   // đăng xuất
  ANSWER,   // trả lời
  SHOW_ROOM,
  JOIN_ROOM,   // tham gia phòng có sẵn
  CREATE_ROOM, // tạo phòng
  START_GAME,  // bắt đầu trò chơi
  EXIT_ROOM,   // thoát phòng
  EXIT_GAME    // thoát trò chơi
} CLIENT_CODE;

typedef enum {
  LOGIN_SUCCESSFULLY,       //đăng nhập thành công
  LOGIN_UNSUCCESSFULLY,     //đăng nhập không thành công
  ACCOUNT_IN_USE,           // tài khoản đang được sử dụng
  LOGOUT_SUCCESSFULLY,      //Đăng xuất thành công
  REGISTER_SUCCESSFULLY,    //Đăng ký thành công
  USERNAME_EXISTED,         // Tên tài khoản đã tồn tại
  ANSWER_IS_CORRECT,        // Câu trả lời đúng
  ANSWER_IS_INCORRECT,      // Câu trả lời sai
  JOIN_ROOM_SUCCESSFULLY,   // Tham gia phòng thành công
  CREATE_ROOM_SUCCESSFULLY, // Tạo phòng thành công
  GAME_READY,               // Game sẵn sàng bắt đầu
  GAME_NOT_READY,           // Game chưa sẵn sàng bắt đầu
  GAME_END,                 // Game kết thúc
  EXIT_ROOM_SUCESSFULLY,    // Thoát phòng thành công
  EXIT_GAME_SUCCESSFULLY    // Thoát game thành công
} SERVER_CODE;

typedef struct {
  CLIENT_CODE code;
  char message[50];
  Data data[10];
} Request;

typedef struct {
  SERVER_CODE code;
  char message[50];
  Data data[10];
} Response;

// core function
int receiveData(int socket, char *buffer);
int sendData(int socket, char *msg);
void broadcast_msg(char *msg, int socket);
int checkLogin(char *username);

// ham them xoa phong
int findUser(char *username);
int findUserBySocket(int socket);
void addPoint(int socket);
int findRoomByHost(char *username);
int findRoomBySocket(int socket);
void createRoom(playerinfo player, int roomnumber);
void addPlayerToRoom(playerinfo player, int roomnumber);
void removePlayerFromRoom(char *username, int roomid);
void deleteRoom(char *username);
void addPlayerToWaitingList(playerinfo player, int roomid);
void removePlayerFromWaitingList(char *username, int roomid);

#endif