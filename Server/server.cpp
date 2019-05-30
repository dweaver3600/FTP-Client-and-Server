#include <iostream>
#include <string>
#include <WS2tcpip.h>	//Windows Socket is the API framework that Windows uses to access Network Sockets
//Includes many header files that will be used later

#include <filesystem>
#include <fstream>
#include <stdio.h> // for remove function

#include <fstream> // for reading/writing files
#include <process.h>

#pragma comment(lib, "ws2_32.lib")



using namespace std;

void main() {
	// initialize Winsock
		// make sure that we have a connection to winsock
	WSADATA WSData;

	WORD ver = MAKEWORD(2, 2); //version 2.2



	int wsOk = WSAStartup(ver, &WSData);

	if (wsOk != 0) {
		cerr << "Cant Initiate winsock! Quitting" << endl;
		return;
	}

	// Create a socket
		// we will bind to this
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0); //Address Family INET, socket number, no flags

	if (listening == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting" << endl;
		return; // quit early
	}

	// Bind the socket to an ip address and port to a socket
	sockaddr_in hint;

	// networking is Big Endian
	// PCs are little Endian
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // could also use inet_pton
		// could also bind to local loopback address

	// bind htons (54000) to socket
	bind(listening, (sockaddr*)&hint, sizeof(hint)); //cast hint as sockaddr pointer

	// Tell Winsock the socket is for listening
	listen(listening, SOMAXCONN); // Max connection. is a huge number

	cout << "Server awaiting connection...";

	// Wait for a connection
	sockaddr_in client; // create client address

	int clientSize = sizeof(client); // get size of client

	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

	//--------------------
	// DO the following if we have a valid socket
	//-------------------

	char host[NI_MAXHOST];		// clients remote name
	char service[NI_MAXSERV];	//service (port) that the client is connected on

	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	//---------------------------
	//MAC/LINUX
	//memset(host, 0, NI_MAXHOST); this is for making a portal
	//---------------------------

	// try to get name information
	// try to get hostname. if not display IP address

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		cout << "\n\n" << host << " connected on port " << service << endl;
	}
	else { // dns service lookup
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		cout << "\n\n" << host << " connected on port " << ntohs(client.sin_port) << endl;
	}

	// Close listening socket
	closesocket(listening);

	// While loop : accept and echo message back to client
	char buf[4096]; // should be reading until no bytes left to read

	while (true) {
		ZeroMemory(buf, 4096);

		// Wait for client to send data
		int bytesReceived = recv(clientSocket, buf, 4096, 0);

		if (bytesReceived == SOCKET_ERROR) {
			cerr << "Error in recv(). quitting" << endl;
			break;
		}

		if (bytesReceived == 0) {
			cout << "Client disconnected" << endl;
			break;
		}

		if (strcmp(buf, "quit") == 0) {
			cout << "Client terminated connection" << endl;
			closesocket(clientSocket);
			break;
		}

		if (strcmp(buf, "ls") == 0) {
			string path = __FILE__;
			string file_path;

			// path is originally the path to THIS file, so remove the file name to have the folder location
			while (path.back() != '\\') {
				path.pop_back();
			}
			path.pop_back();

			string response = "Server Echo>\tFiles Found: \n";
			send(clientSocket, response.c_str(), response.length(), 0);

			cout << "Sending file names..." << endl;

			for (const auto & entry : std::experimental::filesystem::directory_iterator(path)) {
				file_path = entry.path().string();

				auto p_it = file_path.begin();
				auto it = file_path.begin();

				while (it != file_path.end()) {
					if (*it == '\\') {
						while (p_it != it) {
							p_it++;
						}
					}
					it++;
				}

				string file_name;

				p_it++; // this excludes the '/'
				while (p_it != file_path.end()) {
					file_name += *p_it;
					p_it++;
				}


				cout << "\t\t" << file_name << endl;
				file_name = "\t\t" + file_name;
				send(clientSocket, file_name.c_str(), file_name.size(), 0);
			}

			send(clientSocket, "0", 1, 0); // I use this to send a token to tell the client the server is no longer sending files
			cout << "Finished sending files..." << endl;
			continue;
		}

		string userInput = string(buf, 0, bytesReceived);
		string param_1, param_2;

		if (userInput.length() > 3) {	
			param_1 = userInput.substr(0, 3); // command
			param_2 = userInput.substr(param_1.length() + 1); // file
		}

		else continue;

		cout << param_1 << " + " << param_2 << "\n\n";

		// client sends file to be written on server
		if (strcmp(param_1.c_str(), "put") == 0) {
			cout << "putting" << endl;
			
			size_t datasize;
			FILE* fd;
			fopen_s(&fd, param_2.c_str(), "wb");

			//int datagrams_raw = recv(clientSocket, buf, 4096, 0);

			recv(clientSocket, buf, 4096, 0);
			string datagrams_s = string(buf, 0, bytesReceived);

			int datagrams = stoi(datagrams_s);

			while (datagrams > 0)
			{
				
				datasize = recv(clientSocket, buf, sizeof(buf), 0);

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

		} // END PUT FILE

		if (strcmp(param_1.c_str(), "get") == 0) {
			cout << "getting" << endl;

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
			send(clientSocket, to_string(datagrams).c_str(), to_string(datagrams).length(), 0);

			rewind(fd);

			while (!feof(fd)) {
				if ((bytes_read = fread(&buf, 1, 4096, fd)) > 0) {
					send(clientSocket, buf, bytes_read, 0);
				}
				else
					break;
				ZeroMemory(buf, 4096);
			}
			fclose(fd);

			cout << "File sent!" << endl;

			continue;

			continue;
		}

		string echo = "Server Echo> \"" + string(buf, 0, bytesReceived) + "\"";
		system("pause");
		//send(clientSocket, echo.c_str(), echo.length(), 0);
		ZeroMemory(buf, 4096);
	}


	//	Close the cocket
	closesocket(clientSocket);

	// Cleanup Winsock... this is basically close
	WSACleanup();

	system("pause");
}