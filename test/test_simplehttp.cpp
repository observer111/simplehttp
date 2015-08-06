#include <SimpleHttp.hpp>
#include <stdlib.h>
#include <iostream>
// demo libsimplehttp, how to serve http with a C function, etc.

void next_page( SimpleHttp *s, SOCKET_TYPE fd, std::string route,
        std::map <std::string, std::string> *param, std::string request, void *context)
{
    s->http_send_ok(fd);
    s->http_send(fd, "<html><body>"
            "This is another page...<br>"
            "<a href='/'>Previous Page<a>"
            "</body></html>" );
}

int main( int argc, char *argv[] ) {   
    SimpleHttp server( argc >= 2 ? atoi(argv[1]) : 9191 );  // define server obj, port
    server.log = 1; // 1 log-style, 2 verbose, 3+ debug
    server.page( "/",                                       // define pages to serve
            "<html><body>"
            "This is a test HTML page for <i>libsimpleserver</i> .<br>"
            "<br>Here is a JPEG Image:<br>" 
            "<img src='image.jpg'><br><br>" 
            "<a href='nextpage.html'>Next Page<a>" 
            "</body></html>" );
    server.page( "/nextpage.html", next_page );             // serve w/ callback
    server.file( "/image.jpg", "image.jpg");                // serve a file
    server.start();
    std::cout << "listening ... http://localhost:"<<server.port<<std::endl;
    server.eventLoop();     // or, use .handleEvent() in your own event loop
}
