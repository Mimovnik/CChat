import socket
import threading

SERVER_PORT = 5222
USERNAME_LEN = 256
MESSAGE_LEN = 2000


def receive_messages(sockfd):
    while True:
        try:
            # Receive message from server
            message = sockfd.recv(MESSAGE_LEN).decode()
            if not message:
                continue
            print("\n", message, "\nEnter message: ", end="")
        except socket.error as e:
            print("Error receiving message from server:", e)
            break


def main():
    server_ip = input(
        "Enter server IP (leave empty to use default 127.0.0.1): ")
    if not server_ip:
        server_ip = "127.0.0.1"

    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv_addr = (server_ip, SERVER_PORT)

    try:
        # Connect to server
        sockfd.connect(serv_addr)

        receive_thread = threading.Thread(
            target=receive_messages, args=(sockfd,))
        receive_thread.start()

        while True:
            # Send name to server
            name = input("Enter your name: ")
            sockfd.sendall(name.encode())

            # Verify unique name
            verification = sockfd.recv(2).decode()
            if verification == "OK":
                break
            print("Name already taken.")

        while True:
            # Send message to server
            message = input("Enter message: ")
            sockfd.sendall(message.encode())

    except socket.error as e:
        print(f"ERROR: {e}")

    finally:
        # Close the connection
        sockfd.close()


if __name__ == "__main__":
    main()
