#include "common.h"
#include "py2py3.h"

#define UNHEX(c) ((c >= '0' && c <= '9') ? (c - '0') : \
                  (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : \
                  (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : NOHEX)
#define NOHEX ((char) -1)

size_t unquote_url_inplace(char* url, size_t len)
{
    for(char* p = url, *end = url + len; url != end; ++url, ++p) {
        if(*url == '%') {
            if(url >= end - 2) {
                /* Less than two characters left after the '%' */
                return 0;
            }
            char a = UNHEX(url[1]);
            char b = UNHEX(url[2]);
            if(a == NOHEX || b == NOHEX) return 0;
            *p = a * 16 + b;
            url += 2;
            len -= 2;
        } else {
            *p = *url;
        }
    }
    return len;
}

void _init_common()
{

#define _(name) _##name = _PEP3333_String_FromUTF8String(#name)
    _(REMOTE_ADDR);
    _(PATH_INFO);
    _(QUERY_STRING);
    _(close);

    _(REQUEST_METHOD);
    _(SERVER_PROTOCOL);
    _(SERVER_NAME);
    _(SERVER_PORT);
    _(GET);
    _(HTTP_CONTENT_LENGTH);
    _(CONTENT_LENGTH);
    _(HTTP_CONTENT_TYPE);
    _(CONTENT_TYPE);
    _(HTTP_);
    _(http);

    _(BytesIO);
    _(write);
    _(read);
    _(seek);
#undef _

    _HTTP_1_1 = _PEP3333_String_FromUTF8String("HTTP/1.1");
    _HTTP_1_0 = _PEP3333_String_FromUTF8String("HTTP/1.0");
    _wsgi_input = _PEP3333_String_FromUTF8String("wsgi.input");
    _empty_string = _PEP3333_String_FromUTF8String("");
    _empty_bytes = _PEP3333_Bytes_FromString("");
}
