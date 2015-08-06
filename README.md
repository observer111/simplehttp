# simplehttp

libsimplehttp

Implements the SimpleHttp class, a lightweight, embeddable,  C++ http server.

The idea here is to use C++ syntax and classes to make it easy to embed
tiny, stand-alone, custom http servers in C++ programs.

   * simple, lightweight
   * embeddable
   * compile option: forking server, or serve using pthreads (std::thread)
   * makes it easy to add a modern browser (or app webkit) UI to your C++ code

installation (the usual for cmake)

    cd libsimplehttp # or wherever
    cmake .
    make
    make install

examples

    see the test directory;

        test/test_simplehttp.cpp     # test libsimplehttp
