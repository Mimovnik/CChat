#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_SIZE 2000
#define MAX_USERNAME_SIZE 256

int connected_sockets[MAX_CLIENTS];
char *user_names[MAX_CLIENTS];
pthread_mutex_t connected_sockets_mutex;

int safe_strcmp(const char *s1, const char *s2) {
  if (s1 == NULL) {
    return s2 == NULL ? 0 : 1;
  }
  if (s2 == NULL) {
    return 1;
  }
  return strcmp(s1, s2);
}

void modify_username_array(char *search_for, char *replace_with) {
  pthread_mutex_lock(&connected_sockets_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (safe_strcmp(user_names[i], search_for) == 0) {
      if (replace_with == NULL) {
        free(user_names[i]);
      }
      user_names[i] = replace_with;
      break;
    }
  }
  pthread_mutex_unlock(&connected_sockets_mutex);
}

void modify_socket_array(int search_for, int replace_with) {
  pthread_mutex_lock(&connected_sockets_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (connected_sockets[i] == search_for) {
      connected_sockets[i] = replace_with;
      break;
    }
  }
  pthread_mutex_unlock(&connected_sockets_mutex);
}

char *get_user_name_by_socket_id(int socket_id) {
  pthread_mutex_lock(&connected_sockets_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (connected_sockets[i] == socket_id) {
      pthread_mutex_unlock(&connected_sockets_mutex);
      return user_names[i];
    }
  }
  pthread_mutex_unlock(&connected_sockets_mutex);
  return NULL;
}

void broadcast(char *message, int sender) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (connected_sockets[i] != -1 && connected_sockets[i] != sender) {
      // Add to message the nickname
      char *nickname = get_user_name_by_socket_id(sender);
      char *message_with_nickname =
          malloc(strlen(nickname) + strlen(message) + 3);
      strcpy(message_with_nickname, nickname);
      strcat(message_with_nickname, ": ");
      strcat(message_with_nickname, message);
      send(connected_sockets[i], message_with_nickname,
           strlen(message_with_nickname), 0);
      free(message_with_nickname);
    }
  }
}

int is_username_available(char *username) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (safe_strcmp(user_names[i], username) == 0) {
      return 0;
    }
  }
  return 1;
}

void *connection_handler(void *socket_desc) {

  /* Get the socket descriptor */
  int sock = *(int *)socket_desc;
  int read_size;
  char client_message[MAX_MESSAGE_SIZE];
  char username[MAX_USERNAME_SIZE];

  // Verify if the username is already in use
  fprintf(stderr, "Waiting for username\n");
  while (1) {
    recv(sock, username, MAX_USERNAME_SIZE, 0);
    fprintf(stderr, "Received username %s\n", username);
    if (is_username_available(username)) {
      send(sock, "OK", 2, 0);
      fprintf(stderr, "Accepting\n");
      break;
    } else {
      send(sock, "NO", 2, 0);
      fprintf(stderr, "Username already in use\n");
    }
  }

  fprintf(stderr, "Adding user %s\n", username);
  modify_username_array(NULL, strdup(username));
  modify_socket_array(-1, sock);

  do {
    read_size = recv(sock, client_message, 2000, 0);
    fprintf(stderr, "Received %s\n", client_message);
    client_message[read_size] = '\0';

    broadcast(client_message, sock);

    /* Clear the message buffer */
    memset(client_message, 0, 2000);
  } while (read_size > 0);

  fprintf(stderr, "Client disconnected\n");

  modify_socket_array(sock, -1);
  modify_username_array(get_user_name_by_socket_id(sock), NULL);

  close(sock);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;

  pthread_t thread_id;
  pthread_mutex_init(&connected_sockets_mutex, NULL);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    connected_sockets[i] = -1;
  }

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(5222);

  int opt = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  int result = bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (result != 0) {
    perror("Error binding");
    exit(1);
  }

  listen(listenfd, 10);

  fprintf(stderr, "Server started\n");

  for (;;) {
    connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
    fprintf(stderr, "Connection accepted\n");
    pthread_create(&thread_id, NULL, connection_handler, (void *)&connfd);
  }
}
