#include <stdio.h>
#include "csapp.h"
#include <pthread.h>
#include <time.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* Function Prototypes */
void doit(int fd);
void parseuri(char *uri, char *hostname, char *port, char *path);
void read_request_headers(rio_t *rio, char *extra_hdrs);
void *thread_routine(void *vargp);

int main(int argc, char **argv)
{
    int listenfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* Lab Requirement: Ignore SIGPIPE so the proxy doesn't crash */
    Signal(SIGPIPE, SIG_IGN);

    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        int *connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        pthread_t tid;
        Pthread_create(&tid, NULL, thread_routine, connfdp);

        // for logging purpose
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }
    return 0;
}

void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char extra_hdrs[MAXLINE] = "";
    char final_request[MAXLINE];
    int serverfd;
    rio_t rio_client, rio_server;
    ssize_t n;

    /* 1. Read Request Line */
    Rio_readinitb(&rio_client, fd);
    if (!Rio_readlineb(&rio_client, buf, MAXLINE))
        return;

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        printf("Proxy only handles GET requests.\n");
        return;
    }

    /* 2. Parse URI */
    parseuri(uri, hostname, port, path);

    /* 3. Read and collect headers from the client */
    read_request_headers(&rio_client, extra_hdrs);

    /* 4. Build the final request string for the server */
    sprintf(final_request, "GET %s HTTP/1.0\r\n", path); // Always HTTP/1.0
    sprintf(final_request + strlen(final_request), "Host: %s\r\n", hostname);
    strcat(final_request, user_agent_hdr);
    strcat(final_request, "Connection: close\r\n");
    strcat(final_request, "Proxy-Connection: close\r\n");
    strcat(final_request, extra_hdrs); // Add any cookies/extra info from client
    strcat(final_request, "\r\n");     // Final blank line

    /* 5. Connect to Server and Send Request */
    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0)
        return;

    Rio_writen(serverfd, final_request, strlen(final_request));

    /* 6. Forward Response (Binary Safe) */
    Rio_readinitb(&rio_server, serverfd);
    while ((n = Rio_readnb(&rio_server, buf, MAXLINE)) > 0)
    {
        Rio_writen(fd, buf, n);
    }

    Close(serverfd);
}

void read_request_headers(rio_t *rio, char *extra_hdrs)
{
    char buf[MAXLINE];

    while (Rio_readlineb(rio, buf, MAXLINE) > 0)
    {
        /* Stop at the blank line */
        if (!strcmp(buf, "\r\n") || !strcmp(buf, "\n"))
            break;

        /* Forward only headers that we aren't already hardcoding in doit */
        if (strstr(buf, "Host:") || strstr(buf, "User-Agent:") ||
            strstr(buf, "Connection:") || strstr(buf, "Proxy-Connection:"))
        {
            continue;
        }
        strcat(extra_hdrs, buf);
    }
}

void parseuri(char *uri, char *hostname, char *port, char *path)
{
    char *hostptr = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;
    char *pathptr = strstr(hostptr, "/");

    if (pathptr)
    {
        strcpy(path, pathptr);
        *pathptr = '\0';
    }
    else
    {
        strcpy(path, "/");
    }

    char *portptr = strstr(hostptr, ":");
    if (portptr)
    {
        strcpy(port, portptr + 1);
        *portptr = '\0';
    }
    else
    {
        strcpy(port, "80");
    }
    strcpy(hostname, hostptr);
}

void *thread_routine(void *vargp)
{
    int connfd = *((int *)vargp);
    Free(vargp);
    Pthread_detach(Pthread_self());

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    printf("[%.3f] Thread %ld: START\n", 0.0, pthread_self());

    doit(connfd);
    Close(connfd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("[%.3f] Thread %ld: END (%.3f sec)\n", elapsed, pthread_self(), elapsed);

    return NULL;
}