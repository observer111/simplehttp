/*! \file SimpleHttp.hpp
    \brief dev header for SimpleHttp server
*/
#ifndef _SIMPLEHTTP_HPP
#define _SIMPLEHTTP_HPP 1

#ifndef MS_WINDOWS
 #if defined(WINDOWS) || defined(_WIN) || defined(WIN) \
  || defined(_WIN32) || defined(WIN32) \
  || defined(_WIN64) || defined(WIN64)
 # define MS_WINDOWS
 #endif
#endif

#ifdef MS_WINDOWS
#pragma warning (disable: 4251)  // warnings about exposing stl in dll
# undef UNICODE
# define WIN32_LEAN_AND_MEAN
// Need to link with Ws2_32.lib
# pragma comment (lib, "Ws2_32.lib") // visual c++ only
// #pragma comment (lib, "Mswsock.lib") // visual c++ only
#  ifdef BUILDING_DLL
#   define EXPORT_MARKER __declspec(dllexport)
#  else
#   define EXPORT_MARKER __declspec(dllimport)
#  endif
//# define SOCKET_TYPE SOCKET
# define SOCKET_TYPE unsigned int
# define SOCKET_SEND(s,b,l)    send(SOCKET(s),b,l,0)     
# define OPEN _open
# define READ _read
# define CLOSE _close
//
#else // linux,etc:
//
# include <arpa/inet.h>
# define EXPORT_MARKER  
# define SOCKET_TYPE int
# define SOCKET_ERROR -1
# define INVALID_SOCKET -1
# define SOCKET_SEND(s,b,l)    write(s,b,l)
# define OPEN open
# define READ read
# define CLOSE close
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>

enum page_type { CONTENT, FILENAME };
enum status_type { INIT, STARTED, STOP, SERVER_ERROR, CLOSED };

//!> static page info
typedef struct {
    page_type type;
    std::string header;
    std::string content;
    std::string filename;
} page_info;


class EXPORT_MARKER SimpleHttp;


//!> call back type
typedef void ( * SIMPLEHTTP_CALLBACK ) (
  SimpleHttp *server,
  SOCKET_TYPE fd, std::string route, 
  std::map <std::string, std::string> *param, //!< parsed if a GET; empty if a POST
  std::string req, //!< empty if a GET; full request if a POST n.b.: NOT null terminated
  void *context //!< a pointer ...
  );


//!> simple forking or threaded HTTP server
class EXPORT_MARKER SimpleHttp {
    private:
        std::map< std::string, page_info > page_map;
        std::map< std::string, SIMPLEHTTP_CALLBACK > page_funct; //!< pg name => callback
        status_type status;
        static void set_nonblock(SOCKET_TYPE socket);
#ifdef MS_WINDOWS
        static void usleep (long usec);
#endif
        void init();
    public:
        SimpleHttp();               //!< create server (at port 80)
        SimpleHttp( int port );     //!< create server at port
        ~SimpleHttp();

        bool start();               //!< start the server

        void file( std::string route, std::string filename, std::string header="" ); //!< serve a file
        void page( std::string route, std::string page, std::string header="" );    //!< serve a page
        void page( std::string route, SIMPLEHTTP_CALLBACK);
                                    //!< serve with callback

        bool handleEvents();    //!< process (fork) pending server events, non-blocking
        bool is_stopped();      //!< is the server in the STOP status ?
        void stop();            //!< request that the server stop/halt
        void eventLoop();       //!< handle events until stopped

        void closeServer();     //!< shut down serrver, close port etc

        int http_send_ok(SOCKET_TYPE fd, std::string header="" );
                    //!< send http ok, header to client
        int http_send(SOCKET_TYPE fd, std::string s);
                    //!< send s to client
        int http_send(SOCKET_TYPE client_socket, char *buf, size_t buf_size );
                    //!< send s to client

        void respond( SOCKET_TYPE fd );

        int port;                       //!< server port
        SOCKET_TYPE listen_socket;      //!< server is listening on this socket
        std::string http_host;
        size_t maxRecvBufferSize;       //!< max recv message (post) size (!)
        bool tcp_nodelay;               //!< use TCP_NODELAY (Nagle) ?
        int max_getaddr_tries;          //!< max number of tines to try to get addr
        int getaddr_retry_wait_secs;  //!< wait after bind error before retry

        unsigned int log; //!< messages to stdout if > 0
        void *context; //!< ptr passed to callbacks


        static unsigned int  parseUrlKeyValuePairs( std::string srcStr, //!< source: url string
                std::map <std::string, std::string> &dstValue,  //!< destination: value[key]
                bool find_start=false      //!< ignore initial string 
                );
        static std::string  urlDecode(std::string str);
};

#endif // _SIMPLEHTTP_HPP
