#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include "StudyBuddy.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

void runServer();
void runClient();

int main() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    int userChoice;
    cout << "Enter 1 to join a group, or 0 to host a group: ";
    cin >> userChoice;
    cin.ignore();

    if (userChoice == 0) {
        runServer();
    }
    else if (userChoice == 1) {
        runClient();
    }

    WSACleanup();
    return 0;
}

void runServer() {
    string name, location, courses;
    vector<string> members;

    cout << "Enter your study group name: ";
    getline(cin, name);
    cout << "Enter the study group location: ";
    getline(cin, location);
    cout << "Enter the courses (comma separated): ";
    getline(cin, courses);

    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    cout << "Server is running and waiting for queries..." << endl;

    char recvBuf[DEFAULT_BUFLEN];
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    while (true) {
        int recvLen = recvfrom(serverSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&clientAddr, &clientAddrSize);
        if (recvLen > 0) {
            recvBuf[recvLen] = '\0';
            string request(recvBuf);
            cout << "Received: " << request << endl;

            string response;
            if (request == Study_QUERY) {
                response = string(Study_NAME) + name;
            }
            else if (request == Study_WHERE) {
                response = string(Study_LOC) + location;
            }
            else if (request == Study_WHAT) {
                response = string(Study_COURSES) + courses;
            }
            else if (request == Study_MEMBERS) {
                string memberList = "";
                for (const string& member : members) memberList += member + "\n";
                response = string(Study_MEMLIST) + memberList;
            }
            else if (request.rfind(Study_JOIN, 0) == 0) {
                string newMember = request.substr(strlen(Study_JOIN));
                members.push_back(newMember);
                response = Study_CONFIRM;
            }
            sendto(serverSocket, response.c_str(), response.length() + 1, 0, (sockaddr*)&clientAddr, clientAddrSize);
            cout << "Sent: " << response << endl;
        }
    }
    closesocket(serverSocket);
}

void runClient() {
    string username;
    cout << "Enter your name: ";
    getline(cin, username);

    ServerStruct servers[MAX_SERVERS];
    SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int numServers = getServers(clientSocket, servers);

    for (int i = 0; i < numServers; i++) {
        cout << "Group #" << i + 1 << " " << servers[i].name << endl;
    }

    int choice;
    cout << "Enter the number of the group to interact with: ";
    cin >> choice;
    choice--;
    cin.ignore();

    sockaddr_in serverAddr = servers[choice].addr;
    char sendBuf[DEFAULT_BUFLEN];
    char recvBuf[DEFAULT_BUFLEN];
    int addrSize = sizeof(serverAddr);

    while (true) {
        cout << "Options:\n"
            << "0: Get location\n"
            << "1: Get course list\n"
            << "2: Get member list\n"
            << "3: Join group\n"
            << "4: Exit\n"
            << "Enter choice: ";
        int option;
        cin >> option;
        cin.ignore();

        string message;
        if (option == 0) message = Study_WHERE;
        else if (option == 1) message = Study_WHAT;
        else if (option == 2) message = Study_MEMBERS;
        else if (option == 3) message = string(Study_JOIN) + username;
        else break;

        sendto(clientSocket, message.c_str(), message.length() + 1, 0, (sockaddr*)&serverAddr, addrSize);

        if (wait(clientSocket, 2, 0) > 0) {
            int recvLen = recvfrom(clientSocket, recvBuf, DEFAULT_BUFLEN, 0, (sockaddr*)&serverAddr, &addrSize);
            if (recvLen > 0) {
                recvBuf[recvLen] = '\0';
                cout << "Received: " << recvBuf << endl;
                if (option == 3) break;
            }
        }
    }
    closesocket(clientSocket);
}
