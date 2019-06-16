#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include "server.h"
#include "wsgi.h"
#include "filewrapper.h"

void run(PyObject* wsgi_app, int fd, char* host, int port)
{
    ServerInfo info;

    info.wsgi_app = wsgi_app;
    info.sockfd = fd;

    if(strlen(host)) {
        info.host = Py_BuildValue("s", host);
        info.port = Py_BuildValue("i", port);
    }
    else  
        info.host = NULL;

    _initialize_request_module(&info);
    server_run(&info);
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

int makeCSocket(char* sock, char* host, int port) {
    //for sock, host use "" default, not NULL,
    int fd;
    struct sockaddr_in addr;
    int ok = 1;

    if(!strlen(sock)) {
        //use host:port mode
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0) {
            printf("Error in init socket\n");
            return -1;
        }

        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr  = inet_addr(host);

        if(bind(fd, (struct sockaddr* )&addr, sizeof(addr)) < 0) {
            printf("Error in bind socket\n");
            return -1;
        }

        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(int)) < 0) {
            printf("Error in set sockopt\n");
            return -1;
        }

        if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &ok, sizeof(int)) < 0) {
            printf("Error in set sockopt\n");
            return -1;
        }
    }
    else {
        //use unix sock mode, sock must like unix:t1.sock
        printf("still working in unix sock mode\n");
        return -1;
    }

    if(listen(fd, 1024) < 0) {
        printf("Error in listen socket\n");
        return -1;
    }

    return fd;
}

PyObject* makeApp(char* wsgi) {
    //wsgi: just like module.module:callable
    //simplifily wsgi only contains one ':'
    //first import module, and then get the callable object.
    PyObject *pModule = NULL;
    PyObject *pApp = NULL;
    char* t1 = malloc(sizeof(char*) * strlen(wsgi));
    char* t2 = malloc(sizeof(char*) * strlen(wsgi));
    int i = 0;
    
    while(wsgi[i++] != '\0') {
        if(wsgi[i] == ':')
           break;
    }
   
    if(i >= strlen(wsgi)) {
        printf("Error format with wsgi: %s\n", wsgi);
        goto error;
    }

    memcpy(t1, wsgi, i);
    t1[i] = '\0';
    memcpy(t2, wsgi + i + 1, strlen(wsgi) - i + 1);
    t2[strlen(wsgi) - i + 1] = '\0';

#ifdef DEBUG
    printf("%s:%s\n", t1, t2);
#endif

    goto try;
try:
    pModule = PyImport_ImportModule(t1);
    if(PyErr_Occurred()) {
        //if(PyErr_ExceptionMatches(PyExc_ModuleNotFoundError)) {
        PyErr_Print();
        goto error;
        //}
    }

    pApp = PyObject_GetAttrString(pModule, t2);
    if(PyErr_Occurred()) {
        //if(PyErr_ExceptionMatches(PyExc_AttributeError)) {
        PyErr_Print();
        goto error;
        //}
    }

    if(!PyCallable_Check(pApp)) {
        char* s = "";
        sprintf(s, "%s.%s is not callable.", t1, t2);
        PyErr_SetString(PyExc_TypeError, s);
        PyErr_Print();
        goto error;
    }
    goto finally;

finally:
    Py_DECREF(pModule);
    free(t1);
    free(t2);
    return pApp;
error:
    Py_XDECREF(pModule);
    Py_XDECREF(pApp);
    free(t1);
    free(t2);
    return NULL;
}

int main(int argc, char** argv) {
    int fd = 0;

    PyObject *pApp = NULL;

    if(argc < 2) {
        printf("Usage: %s wsgi\n", argv[0]);
        return -1;
    }

    fd = makeCSocket("", "127.0.0.1", 8000);

    if(fd < 0) {
        return -1;
    }
    
    Py_Initialize();
    PyRun_SimpleString("import sys;sys.path.append('.')");

    init_bjoern();
    
    goto try;
try:
    pApp = makeApp(argv[1]);
    if(pApp == NULL)
        goto error;
    //Py_INCREF(pApp);

    run(pApp, fd, "127.0.0.1", 8000);
    if(PyErr_Occurred())
        PyErr_Print();

    if(Py_FinalizeEx() < 0)
        goto error;
    goto finally;

finally:
    Py_DECREF(pApp);
    close(fd);
    return 0;

error:
    Py_XDECREF(pApp);
    close(fd);
    return -1;
}
