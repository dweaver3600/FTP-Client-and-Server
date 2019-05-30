#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <fstream>

#include <conio.h>

#include <process.h>


#pragma comment(lib, "ws2_32.lib")



using namespace std;

void main() {
	string ipAddress = "127.0.0.1";	//IP Address of the server 127.0.0.1 is the THIS device
	int port = 54000;				// Listening port number on the server

	// Initialize WinSock
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);

	if (wsResult != 0) {
		cerr << "Cant' start Winsock, Err #" << wsResult << endl;
		system("pause");
		return;
	}

	// create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); // address family - internet, socket, protocol number
		//socket valid?
	if (sock == INVALID_SOCKET) {
		cerr << "Can't create socket, Err #" << WSAGetLastError << endl;
		WSACleanup();
		system("pause");
		return;
	}

	// fill in a hint structure
		// this tells WINSock which server and which port we want to connect to
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port); // host to network socket
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// connect to a server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		system("pause");
		return;
	}

	// do while loop to send and receive data
	cout << "Connection Established at " << ipAddress << "\n\n";
	char buf[4096];
	string userInput;

	do {
		// prompt the user for some text
		cout << "ftp>";
		//getline(cin, userInput); // will grab the whole line of text from input
		getline(cin, userInput);

		if (userInput.size() > 0) { // User needs to send something
			// send the text
			int sendResult = send(sock,
				userInput.c_str(),
				userInput.size() + 1, // doing + 1 because string has '\0' at the end
				0);

			if (userInput == "quit") {
				closesocket(sock);
				cout << "Terminating Connection to Server\n";

				WSACleanup();
				system("pause");
				return;
			}

			if (userInput == "ls") {
				int bytesReceived;
				do {
					bytesReceived = recv(sock, buf, 4096, 0);

					if (string(buf, 0, bytesReceived) == "0") {
						break;
					}

					cout << string(buf, 0, bytesReceived) << endl;
				} while (true);
				continue;
			}

			else {

				// tokenize
				string param_1, param_2;

				if (userInput.length() > 3) {
					param_1 = userInput.substr(0, 3);
					param_2 = userInput.substr(param_1.length() + 1);
				}

				else continue;

				cout << param_1 << " + " << param_2 << endl;
				if (param_1 != "get" && param_1 != "put") {
					cout << "unrecognized input" << endl;
					continue;
				}

				if (param_1 == "put") {
					//int size = ftell(fd); // the size of that file

					//send(sock, &size, sizeof(size), 0);
					FILE *fd;
					fopen_s(&fd, param_2.c_str(), "rb");

					if (fd == NULL)
					{
						puts("Couldn't open file");
						continue;
					}

					fopen_s(&fd, param_2.c_str(), "rb");
					size_t rret, wret;
					int bytes_read;

					int datagrams = 0;

					while (!feof(fd)) {
						if ((bytes_read = fread(&buf, 1, 4096, fd)) > 0) {
							datagrams++;
						}
							
						else
							break;
					}

					send(sock, to_string(datagrams).c_str(), to_string(datagrams).length(), 0);

					rewind(fd);

					while (!feof(fd)) {
						if ((bytes_read = fread(&buf, 1, 4096, fd)) > 0) {
							send(sock, buf, bytes_read, 0);
						}
						else
							break;
						ZeroMemory(buf, 4096);
					}
					fclose(fd);

					cout << "File sent!" << endl;

					continue;
				}

				if (param_1 == "get") {
					cout << "getting" << endl;
					
					size_t datasize;
					FILE* fd;
					fopen_s(&fd, param_2.c_str(), "wb");

					//int datagrams_raw = recv(clientSocket, buf, 4096, 0);

					recv(sock, buf, 4096, 0);
					string datagrams_s = string(buf, 0, sock);

					int datagrams = stoi(datagrams_s);

					while (datagrams > 0)
					{

						datasize = recv(sock, buf, sizeof(buf), 0);

						cout << "DATA SIZE: " << sizeof(buf) << endl;

						datagrams--;

						cout << "DATAGRAMS LEFT: " << datagrams << endl;

						fwrite(&buf, 1, sizeof(buf), fd);

						//ZeroMemory(&datasize, 4096);
						ZeroMemory(buf, 4096);

						if (datagrams == 0) {
							fclose(fd);
							cout << "File Recieved" << endl;
							break;
						}
					}

					continue;
				}

				if (sendResult != SOCKET_ERROR) {
					// wait for response
					ZeroMemory(buf, 4096);
					int bytesReceived = recv(sock, buf, 4096, 0);
					if (bytesReceived > 0) {
						// Echo the input
						cout << string(buf, 0, bytesReceived) << endl;
					}
				}

			}
		}

		cout << "\n";

	} while (userInput.size() > 0);

	// gracefully close down everything
	closesocket(sock);
	WSACleanup();
}