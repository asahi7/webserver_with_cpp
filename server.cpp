#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string>

#define PORT 5000

void respond(int sock);

std::string ROOT;

int main(int argc, char *argv[]) {
    int sockfd, portno = PORT;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    ROOT = getenv("PWD");

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Make port reusable */
    int tr = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Bind socket */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket */
    if (listen(sockfd, 1000) < 0) {
        perror("ERROR on listening");
        exit(-1);
    }

    printf("Server is running on port %d\n", portno);

    clilen = sizeof(serv_addr);

    while (1) {
        /* Acccepting connections */
        int newsockfd = accept(sockfd, (struct sockaddr *) &serv_addr, (socklen_t *) &clilen);
        if (newsockfd < 0) {
            printf("ERROR on accepting");
            exit(-1);
        }

        /*int pid = fork();
        if (pid == 0) {
            respond(newsockfd);
            close(newsockfd);
            exit(0);
        } else if (pid > 0) {
            //close(newsockfd);
        } else {
            printf("Error occured while forking");
            exit(-1);
        }*/

        // TODO : implement processing. there are three ways - single threaded, multi threaded, thread pooled
        respond(newsockfd);
    }

    return 0;
}

void send_file(const char *response, int sock, int len) {
    send(sock, response, len, 0);
}

void send_headers(const char *response, int sock) {
    std::cout << response << std::endl;
    send(sock, response, strlen(response), 0);
    send(sock, "\n", 1, 0);
}

void send_bad_request(int sock) {
    std::string response = "HTTP/1.1 400 Bad Request\n";
    send_headers(response.c_str(), sock);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

long get_file_size(std::string filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

bool has_suffix(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void respond(int sock) {
    int n;
    char buffer[9999];
    char abs_path[256];
    char send_buff[8192];
    std::ifstream in;
    std::string response;
    const char *delim = "\r\n\r\n";

    bzero(buffer, 9999);
    n = recv(sock, buffer, 9999, 0);
    if (n < 0) {
        printf("recv() error\n");
        return;
    } else if (n == 0) {
        printf("Client disconnected unexpectedly\n");
        return;
    } else {
        char *token = strtok(buffer, delim);

        std::cout << token << std::endl;

        if (strncmp("GET", token, 3) != 0) {
            std::cout << "Only GET method works" << std::endl;
            send_bad_request(sock);
            return;
        }

        char token_cpy[256];
        strcpy(token_cpy, token);
        std::string path = strtok(token_cpy, " \t");
        path = strtok(NULL, " \t");

        if (strncmp("/", path.c_str(), path.length()) == 0) {
            path = "/index.html";
        }

        if (has_suffix(path, ".jpeg") || has_suffix(path, ".jpg")) {
            response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: image/jpeg\r\n"
                       "Connection: keep-alive\r\n";
            in.open(ROOT + path.c_str(), std::ios::binary);
            in.seekg(0, std::ios::beg);
            std::cout << "IMG SIZE: " << std::to_string(get_file_size(ROOT + path.c_str())) << std::endl;
            response += "Content-Length: " + std::to_string(get_file_size(ROOT + path.c_str())) + "\r\n";
            send_headers(response.c_str(), sock);
            while (!in.eof() && in.is_open()) {
                memset(send_buff, 0, sizeof(send_buff));
                in.read(send_buff, sizeof(send_buff));
                send_file(send_buff, sock, sizeof(send_buff));
            }
        } else {
            response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html; charset=UTF-8\r\n"
                       "Content-Encoding: UTF-8\r\n";
            in.open(ROOT + path.c_str());
            std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            response += "Content-Length: " + std::to_string(get_file_size(ROOT + path.c_str())) + "\r\n";
            send_headers(response.c_str(), sock);
            std::cout << ROOT + path.c_str() << std::endl << response << std::endl;
            send_file(contents.c_str(), sock, strlen(contents.c_str()));
        }

        std::cout << std::endl;
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

}
