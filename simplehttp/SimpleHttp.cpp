/*! \file SimpleHttp.cpp
    \brief  C++ class wrapper for simple http server C library
  
  * make it very easy and simple to embed http servers in C++ applications
   
  * platforms: linux, win32 (via mingw cross-compilation) 
   
  * based on Abhijeet Rastogi's very simple server:
     http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html ,
   with inspiration from http://mihl.sourceforge.net/  et al.
 */
#include <iostream>
#include <sstream>
#include <string>
#ifdef USE_STD_THREAD
#include <thread>
#include <mutex>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "SimpleHttp.hpp"

#ifndef MS_WINDOWS
// linux, etc
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/tcp.h>
# include <netdb.h>
#else
// Windows
# include <io.h>
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define READ_BUF_SIZE 1024

#ifndef SOMAXCONN
#  define SOMAXCONN 1000000
#endif

//!> construt simple http server object 
SimpleHttp::SimpleHttp()
{
    init();
    port = 80;
}

//!> construt simple http server object, specifing server port
SimpleHttp::SimpleHttp( int http_port )
{
    init();
    port = http_port;
}

//!> object initialisations
void SimpleHttp::init()
{
    log = 0;
    status = INIT;
    listen_socket = INVALID_SOCKET;
    tcp_nodelay = false;
    maxRecvBufferSize = 1024 * 128 ; // max recv message size [128k]
    max_getaddr_tries = 7;
    getaddr_retry_wait_secs = 15;
}

SimpleHttp::~SimpleHttp()
{
    closeServer();
}


void SimpleHttp:: set_nonblock(SOCKET_TYPE socket) {
#ifdef MS_WINDOWS
    u_long iMode=1;
    ioctlsocket(socket,FIONBIO,&iMode); 
#else
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    if (! (flags != -1) ) 
        printf("set_nonblock: fcntl F_GETFL fails\n");
    else
        fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
}


#ifdef MS_WINDOWS
//!> usleep implementation from FreeSCI 
void SimpleHttp:: usleep (long usec)
{
    LARGE_INTEGER lFrequency;
    LARGE_INTEGER lEndTime;
    LARGE_INTEGER lCurTime;
    QueryPerformanceFrequency (&lFrequency);
    if (lFrequency.QuadPart) {
        QueryPerformanceCounter (&lEndTime);
        lEndTime.QuadPart += (LONGLONG) usec *
            lFrequency.QuadPart / 1000000;
        do {
            QueryPerformanceCounter (&lCurTime);
            Sleep(0);
        } while (lCurTime.QuadPart < lEndTime.QuadPart);
    }
}
#endif


/* \brief start the http server */
bool SimpleHttp::start()
{
    if ( (status != INIT) && (status != CLOSED) ) {
        if (log) printf("SimpleHttp::start - attempted start() "
                    " in non INIT (or CLOSED) state - failed\n");
        return false;
    }
    if (log) printf("SimpleHttp::start ... \n");

    struct addrinfo *_p;
    struct addrinfo hints, *result;
    int iResult;

#ifdef MS_WINDOWS
    // Initialize Winsock
    WSADATA wsaData;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        status = SERVER_ERROR;
        return false;
    }
#endif


/////////////////////////////////////////

    int get_addr_try = -1;
    for ( get_addr_try = 0;
          get_addr_try < max_getaddr_tries;
          get_addr_try++ ) 
    {
        // getaddrinfo for host
        memset (&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
#ifdef MS_WINDOWS
        hints.ai_protocol = IPPROTO_TCP;
#endif
        std::stringstream port_str;
        port_str << port;
        if (log>1)
            printf("SimpleHttp::start - getaddrinfor for port %s\n",
                    port_str.str().c_str() );
        if ( getaddrinfo( NULL, port_str.str().c_str(), &hints, &result) != 0)
        {
#ifdef MS_WINDOWS
            printf("getaddrinfo() error\n");
#else
            perror ("getaddrinfo() error");
#endif
            status = SERVER_ERROR;
            return false;
        }

        // socket and bind
        for (_p = result; _p!=NULL; _p=_p->ai_next) // ? for win
        {

            if (log>1) printf("SimpleHttp::start - socket ...\n");
#ifdef MS_WINDOWS
            listen_socket = socket (_p->ai_family, _p->ai_socktype,
                    _p->ai_protocol); // ?
            if (listen_socket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", (long)WSAGetLastError());
                freeaddrinfo( _p ); // or result ?
                WSACleanup();
                continue;
            }
#else
            listen_socket = socket(_p->ai_family, _p->ai_socktype, 0);
            if (listen_socket == SOCKET_ERROR) {
                if (log>1) printf("SimpleHttp::start - socket() == SOCKET_ERROR, try next\n");
                continue;
            }
#endif
            if (log>1) printf("SimpleHttp::start - set non-block ...\n");
            set_nonblock(listen_socket);

            if (log>1) printf("SimpleHttp::start - bind ...\n");
            iResult =  bind(listen_socket, _p->ai_addr, _p->ai_addrlen);
            if ( iResult == 0 ) {
                if (log>1) printf("SimpleHttp::start - bind(...) == 0, OK, stop\n");
                break;
            } else {
                if (log>1) {
                    printf("SimpleHttp::start - bind(...) == %d != 0, (bind fails)\n",iResult);
#ifdef MS_WINDOWS
                    if (iResult == SOCKET_ERROR)
                        printf("bind failed with error %d\n", WSAGetLastError());
#endif
                }
            }
        }
        if ( _p == NULL )
        {
#define EMSG    "socket() or bind() problem, not started"
#ifndef MS_WINDOWS
            perror(EMSG);
#else
            printf(EMSG"\n");
#endif
            status = SERVER_ERROR;

            if ( get_addr_try >= max_getaddr_tries )
                return false;

            printf("# retry (%d of %d), after %d secs ...\n",
                    get_addr_try+1 , max_getaddr_tries,
                    getaddr_retry_wait_secs );

#ifndef MS_WINDOWS
            sleep( getaddr_retry_wait_secs );
#else
            Sleep( getaddr_retry_wait_secs * 1000 );
#endif
        } else {
            // got it - ok
            if ( get_addr_try>0 ) {
                printf("# ok, on retry (%d of %d)\n", 
                        get_addr_try+1 , max_getaddr_tries);
            }
            break;
        }

    } // get_addr_try loop

    /////////////////////////////////////////


    freeaddrinfo(result);

    int option = 1;
#ifndef MS_WINDOWS
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN); // ?
    if ( setsockopt(listen_socket, SOL_SOCKET,
                (SO_REUSEPORT | SO_REUSEADDR),
                (char*)&option,sizeof(option)) < 0)
        printf("warning: setsockopt failed ((SO_REUSEPORT|SO_REUSEADDR)\n");
#endif
    if ( tcp_nodelay ) {
        if ( setsockopt(listen_socket,
                    IPPROTO_TCP,     /* set option at TCP level */
                    TCP_NODELAY,     /* name of option */
                    (char*)&option,sizeof(option)) < 0 )
            printf("warning: setsockopt failed (TCP_NODELAY)\n");
    }

    // listen for incoming connections
#ifdef MS_WINDOWS
    if ( listen (listen_socket, SOMAXCONN) == SOCKET_ERROR )
#else
    if ( listen (listen_socket, SOMAXCONN) != 0 )
#endif
    {
#ifdef MS_WINDOWS
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
#else
        perror("listen() error");
#endif
        status = SERVER_ERROR;
        return false;
    }
    status = STARTED;
    if (log>1) printf("SimpleHttp::start - server started OK\n");
    return true;
}




/* \brief define route => page string */
void SimpleHttp::page( std::string route, std::string page, std::string header )
{
    page_map[ route ].type = CONTENT;
    page_map[ route ].content = page;
    page_map[ route ].header = header;
    if (log>1) printf("server page: %s %s %s\n",route.c_str(), page.c_str(), header.c_str());
}

/* \brief route => file */
void SimpleHttp::file( std::string route, std::string filename, std::string header )
{
    page_map[ route ].type = FILENAME;
    page_map[ route ].filename = filename;
    page_map[ route ].header = header;
    if (log>1) printf("server file: %s %s %s\n",route.c_str(), filename.c_str(), header.c_str());
}

/* \brief route => function callback */
void SimpleHttp::page( std::string route,
         void (*handle)(SimpleHttp *server, SOCKET_TYPE n, std::string route,
            std::map <std::string, std::string> *param,
            std::string req, //!< not null terminated ...
            void *context )  )
{
    page_funct[ route ] = handle;
    if (log>1) printf("server callback: %s\n",route.c_str() );
}



// // get sockaddr, IPv4 or IPv6:
// inline void * _get_in_addr(struct sockaddr *sa)
// {
//     if (sa->sa_family == AF_INET)
//         return &(((struct sockaddr_in*)sa)->sin_addr);
//     return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }


/* \brief poll for and handle http server events (fork or thread)  */
bool SimpleHttp::handleEvents()
{
    if ( status == INIT ) {
        if (log>1) printf("handleEvents: needs start(), so starting ...\n");
        start();
    }
    if ( (status != STARTED) && (status != SERVER_ERROR) ) {
        if (log) printf("handleEvents: status isn't STARTED (or SERVER_ERROR) so exiting now\n");
        return false;
    }

    struct sockaddr_in clientaddr;  
    struct hostent *hostp; /* client host info */
    socklen_t addrlen;

    addrlen = sizeof(clientaddr);
    SOCKET_TYPE client_socket = accept(listen_socket,
            (struct sockaddr *) &clientaddr, &addrlen);

#ifndef MS_WINDOWS
    if ( client_socket<0 ) {
        // something other than accepting a connection happened ....
        if ( !(    (errno == EAGAIN )
                || (errno == EWOULDBLOCK ) ) ) {
            if (log) printf("handleEvents: accept returns %d, errno=%d\n",
                    (int)client_socket,errno);
            perror ("accept() error"); // was error, print
        }
        return false; // nothing done (but didn't block - not an error here)
    }
#else
    if ( client_socket == INVALID_SOCKET) {
        int nError=WSAGetLastError();
        if ( nError != WSAEWOULDBLOCK ) {
            if (log) printf("accept failed with error: %d\n", WSAGetLastError());
        }
        // = nothing done (but didn't block - not an error here)
        return false;
    }
#endif

    ///// connection accepted; client_socket /////

    //char s[INET6_ADDRSTRLEN]; // IPv6 ?
    //inet_ntop(_p->ai_family, _get_in_addr((struct sockaddr *) &clientaddr), s, sizeof s);
    std::string ip_addr_str( inet_ntoa( ((struct sockaddr_in*)&clientaddr)->sin_addr ) );
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    // gethostbyaddr: determine who sent the message 
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
            sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if ( hostp != NULL )
        http_host = hostp->h_name;

#define LOG_IT if (log) fprintf(stdout,"%d/%02d/%02d %02d:%02d:%02d|%s|%s", \
        timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,  timeinfo->tm_mday, \
        timeinfo->tm_hour,  timeinfo->tm_min,  timeinfo->tm_sec, \
        ip_addr_str.c_str(), http_host.c_str() )

#ifdef USE_STD_THREAD
    // pthreaded server (needs -lpthread )
    LOG_IT;
    std::thread client( &SimpleHttp::respond, this, client_socket );
    client.detach();
#else
    // forking server
    if ( fork()==0 ) {
        // now we're in the child process ...
        LOG_IT;
        CLOSE( listen_socket ); // and/or shutdown ?
        respond( client_socket );
        exit(0);
    }
    // parent process continues:
    CLOSE(client_socket);
#endif
    return true; // accepted http socket request
}


//!> low-level socket send
int SimpleHttp::http_send(SOCKET_TYPE client_socket, std::string s)
{
    return int( SOCKET_SEND(client_socket, s.c_str(), s.size() ) );
}

int SimpleHttp::http_send(SOCKET_TYPE client_socket, char *buf, size_t buf_size )
{
    return int( SOCKET_SEND(client_socket, buf, buf_size ) );
}

//!> http header
int SimpleHttp::http_send_ok(SOCKET_TYPE client_socket, std::string header )
{
    int rv = 0;
    rv += http_send(client_socket, "HTTP/1.0 200 OK\n" );
    if (header.size()>0)
        rv += http_send(client_socket, header);
    rv += http_send(client_socket, "\n" );
    return rv;
}


//!> client connection
void SimpleHttp::respond( SOCKET_TYPE client_socket )
{
    char *reqline[3];
    int inDataLength, bytes_read;

    //char mesg[maxRecvBufferSize];
    std::string mesg_str( maxRecvBufferSize, ' ' );


    if (log>2) printf("respond %d\n",client_socket);

    memset( (void*)mesg_str.c_str(), (int)'\0', maxRecvBufferSize );

    inDataLength = recv( client_socket, (char *)mesg_str.c_str(), maxRecvBufferSize, 0);
#ifndef MS_WINDOWS
    if (inDataLength<0) // receive error ?
    { 
        if (log) fprintf(stdout,("recv() error\n"));
        status = SERVER_ERROR;
    }
    else if (inDataLength==0) {    // receive socket closed
        if (log) fprintf(stdout,"Client disconnected unexpectedly.\n");
        status = SERVER_ERROR; // ?
    }
#else // the MS_WINDOWS winsock2 way:
    int nError=WSAGetLastError();
    if ( nError != WSAEWOULDBLOCK && nError != 0 )
    {
        if (log) fprintf(stdout,"Client disconnected unexpectedly. (%d)\n",nError);
        status = SERVER_ERROR; // ?
    }
    else
    if ( inDataLength <= 0 ) {
        if (log>1) fprintf(stdout,"recv returned: %d - nothng done\n",inDataLength );
    }
#endif
    else  // message recieved ok
    {
        reqline[0] = strtok( (char *)mesg_str.c_str(), " \t\n");
        bool is_get = strncmp(reqline[0], "GET\0", 4)==0;
        bool is_post = strncmp(reqline[0], "POST\0", 5)==0 ;
        if ( is_get || is_post ) 
        {   // get or post:
            if (log>1) printf("%s", mesg_str.c_str());
            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0
                    && strncmp( reqline[2], "HTTP/1.1", 8)!=0 ) {
                SOCKET_SEND(client_socket, "HTTP/1.0 400 Bad Request\n", 25);
            }
            else {
                if (log==1 && is_get ) printf(" %s",reqline[1] );
                if (log>1) printf("  ok http %s req\n",reqline[0]);

                std::string route = std::string( reqline[1] );

                std::map <std::string, std::string> params;
                if (is_get) {
                    parseUrlKeyValuePairs( route, params, true );
                    if (log>3) printf("  %d key value pairs\n", (int)params.size() );
                    std::size_t found = route.find("?");
                    if (found!=std::string::npos)
                        route.resize( found );
                }
// see http://code.tutsplus.com/tutorials/http-headers-for-dummies--net-8039
                if ( page_funct.find( route ) != page_funct.end() ) {
                    SIMPLEHTTP_CALLBACK handle 
                        = page_funct[route];
                    if (log>2) printf("   handle \"%s\" with callback\n", route.c_str());
                    (handle)( this, client_socket, route,
                              (is_get ?&params :NULL),        // mesg maybe not null terminated:
                              (is_post 
                               // ? std::string(reinterpret_cast<char*>(mesg), inDataLength)
                               ? std::string(mesg_str, inDataLength)
                               : std::string() ),
                              context );
                }
                else
                    if ( is_get && ( page_map.find( route ) != page_map.end() ) ) {
                        if (log>2) printf("   handle \"%s\" with page_map\n", route.c_str());
                        page_info pg = page_map[route];
                        http_send_ok( client_socket, pg.header );
                        if ( pg.type == CONTENT ) {
                            if (log>2) printf("   CONTENT\n");
                            SOCKET_SEND( client_socket, pg.content.c_str(), pg.content.size() );
                        }
                        else
                        if ( pg.type == FILENAME ) {
                            if (log>2) printf("   handle \"%s\" as filename: %s\n",
                                             route.c_str(), pg.filename.c_str() );
                            int fd;
                            if ( ( fd = OPEN( pg.filename.c_str(), O_RDONLY
#ifdef MS_WINDOWS
                                            |O_BINARY
#endif
                                            ) ) != -1 ) 
                            {
                                char data_to_send[READ_BUF_SIZE];
                                if (log>3) printf("   open ok\n");
                                while ( ( bytes_read = READ(fd, data_to_send, READ_BUF_SIZE)) > 0 ) {
                                    if (log>4) printf("   read %d bytes\n",bytes_read);
                                    SOCKET_SEND(client_socket, data_to_send, bytes_read);
                                }
                                CLOSE(fd);
                            }
                            else {
                                printf("   %s - can't open...\n", pg.filename.c_str());
                                perror("can't open pg.filename ...");
#define NOT_FOUND_404   SOCKET_SEND(client_socket, "HTTP/1.0 404 File Not Found\n", 23)
                                NOT_FOUND_404;
                            }
                        }
                    }
                    else {
                        if (log) printf("   \"%s\" - not found", route.c_str());
                        NOT_FOUND_404;
                    }
            }
        }
    }
    
#ifdef MS_WINDOWS
    closesocket(client_socket);
#else
    shutdown (client_socket, SHUT_RDWR);
    CLOSE(client_socket);
#endif
    if (log>1) {
        printf("   respond() - done.\n\n");
    } else
        if (log==1) printf("\n");
    if (log) fflush(stdout);
}



/* did server halt? */
bool SimpleHttp::is_stopped()
{
    return status == STOP;
}

/* request halt of event loop */
void SimpleHttp::stop()
{
    status = STOP;
}

/* \brief poll for http server events, handle, repeat ... */
void SimpleHttp::eventLoop()
{
    for (;;) {
        handleEvents();
        if (is_stopped()) break;
        usleep( 1000 * 250 ); 
    }
}


void SimpleHttp::closeServer()
{
    stop();
    if ( ( listen_socket != SOCKET_ERROR )
            && (listen_socket != INVALID_SOCKET ) ) {
#ifdef MS_WINDOWS
        closesocket( listen_socket );
#else
        shutdown( listen_socket, SHUT_RDWR );
#endif
        listen_socket = INVALID_SOCKET;
    }
    status = CLOSED;
}


// adapted from:
//  http://www.guyrutenberg.com/2007/09/07/introduction-to-c-cgi-processing-forms/getposth/
std::string  SimpleHttp:: urlDecode(std::string str)
{
    std::string temp;
    register int i;
    char tmp[5], tmpchar;
    strcpy(tmp,"0x");
    int size = str.size();
    for (i=0; i<size; i++) {
        if (str[i]=='%') {
            if (i+2<size) {
                tmp[2]=str[i+1];
                tmp[3] = str[i+2];
                tmp[4] = '\0';
                tmpchar = (char)strtol(tmp,NULL,0);
                temp+=tmpchar;
                i += 2;
                continue;
            } else {
                break;
            }
        } else if (str[i]=='+') {
            temp+=' ';
        } else {
            temp+=str[i];
        }
    }
    return temp;
}

// (adapted, Ibid.)
unsigned int  SimpleHttp:: parseUrlKeyValuePairs( std::string srcStr, // source: url string
        std::map <std::string, std::string> &dstValue,  // destination: value[key]
        bool find_start )                   // ignore initial string 
{
    unsigned int nPairs = 0;
    std::string tmpkey, tmpvalue;
    std::string *tmpstr = &tmpkey;
    dstValue.clear();

    register char* cp = (char *)srcStr.c_str();
    if (cp==NULL) return 0;

    if (find_start){
        register char* sp = cp;
        while (*sp != '\0') {
            if ( ( *sp=='?' ) || ( *sp=='&' ) ) {
                sp++;
                cp = sp; // found start of key=value pairs ...
                break;
            }
            sp++;
        }
        if ( *sp == '\0' ) return 0; // no pairs
    }

    while (*cp != '\0') {
        if (*cp=='&') {
            if (tmpkey.size() != 0 ) {
                dstValue[urlDecode(tmpkey)] = urlDecode(tmpvalue);
                nPairs++;
            }
            tmpkey.clear();
            tmpvalue.clear();
            tmpstr = &tmpkey;
        } else if (*cp=='=') {
            tmpstr = &tmpvalue;
        } else {
            (*tmpstr) += (*cp);
        }
        cp++;
    }

    //enter the last pair to the map
    if (tmpkey.size() != 0 ) {
        dstValue[urlDecode(tmpkey)] = urlDecode(tmpvalue);
        nPairs++;
    }

    return nPairs;
}
