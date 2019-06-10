#include <Python.h>
#include <stdio.h>
#include <string.h>
#include "server.h"
#include "wsgi.h"
#include "filewrapper.h"

static PyObject*
run(PyObject* wsgi_app, int fd, char* host, int port)
{
    ServerInfo info;

    info.wsgi_app = wsgi_app;
    info.sockfd = fd;
    if (info.sockfd < 0) {
        return NULL;
    }

    if(strlen(host)) {
        info.host = Py_BuildValue("s", host);
        info.port = Py_BuildValue("i", port);
    }
    else  
      info.host = NULL;

    _initialize_request_module(&info);
    server_run(&info);

    Py_RETURN_NONE;
}

void init_bjoern(void)
{
    _init_common();
    _init_filewrapper();

    PyType_Ready(&FileWrapper_Type);
    assert(FileWrapper_Type.tp_flags & Py_TPFLAGS_READY);
    PyType_Ready(&StartResponse_Type);
    assert(StartResponse_Type.tp_flags & Py_TPFLAGS_READY);
    Py_INCREF(&FileWrapper_Type);
    Py_INCREF(&StartResponse_Type);
}

int main(int argc, char** argv) {
    printf("hello\n");

    return 0;
}
