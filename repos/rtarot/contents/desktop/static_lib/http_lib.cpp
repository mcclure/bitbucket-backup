/*
 *  Http put/get mini lib
 *  written by L. Demailly
 *  (c) 1998 Laurent Demailly - http://www.demailly.com/~dl/
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *  see LICENSE for terms, conditions and DISCLAIMER OF ALL WARRANTIES
 * (license is at Laurent Demailly's website)
 *
 * $Id: http_lib.c,v 3.5 1998/09/23 06:19:15 dl Exp $ 
 *
 * Description : Use http protocol, connects to server to echange data
 *
 *
 * Modified 11-20-10 by Andi McClure for Jumpcore library:
 * Converted to C++ (for Android); null-terminate result strings,
 * Added "putget" method
 * Windows compatibility
 * Remove buffer overrun!!!
 *
 * $Log: http_lib.c,v $
 * Revision 3.5  1998/09/23 06:19:15  dl
 * portability and http 1.x (1.1 and later) compatibility
 *
 * Revision 3.4  1998/09/23 05:44:27  dl
 * added support for HTTP/1.x answers
 *
 * Revision 3.3  1996/04/25 19:07:22  dl
 * using intermediate variable for htons (port) so it does not yell
 * on freebsd  (thx pp for report)
 *
 * Revision 3.2  1996/04/24  13:56:08  dl
 * added proxy support through http_proxy_server & http_proxy_port
 * some httpd *needs* cr+lf so provide them
 * simplification + cleanup
 *
 * Revision 3.1  1996/04/18  13:53:13  dl
 * http-tiny release 1.0
 *
 *
 */

// static const char *rcsid="$Id: http_lib.c,v 3.5 1998/09/23 06:19:15 dl Exp $";

// #define VERBOSE

/* http_lib - Http data exchanges mini library.
 */

#ifdef WINDOWS

#include <winsock.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// From mingw mailing list
static const char *win32_strerror(DWORD dw)
{
	static char tmp[4096];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
				  tmp, sizeof(tmp), NULL);
	return tmp;
}

// NOTICE HOW UGLY THIS IS: Normal file operations in this file will fail. Currently however there are none.
#define close closesocket

#else

#ifndef OSK
/* unix */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

//static int http_read_line (int fd,char *buffer, int max) ;
//static int http_read_buffer (int fd,char *buffer, int max) ;
#else
/* OS/9 includes */
#include <modes.h>
#include <types.h>
#include <machine/reg.h>
#include <INET/socket.h>
#include <INET/in.h>
#include <INET/netdb.h>
#include <INET/pwd.h>
extern char *malloc();
#endif /* OS9/Unix */
#endif

#include <stdio.h>

#include "http_lib.h"

#define SERVER_DEFAULT "adonis"

/* pointer to a mallocated string containing server name or NULL */
char *http_server=NULL ;
/* server port number */
int  http_port=5757;
/* pointer to proxy server name or NULL */
char *http_proxy_server=NULL;
/* proxy server port number or 0 */
int http_proxy_port=0;
/* user agent id string */
static const char *http_user_agent="adlib/3 ($Date: 1998/09/23 06:19:15 $)";

/*
 * read a line from file descriptor
 * returns the number of bytes read. negative if a read error occured
 * before the end of line or the max.
 * cariage returns (CR) are ignored.
 */
static int http_read_line (int fd, /* file descriptor to read from */
                           char *buffer, /* placeholder for data */ 
                           int max) /* max number of bytes to read */
{ /* not efficient on long lines (multiple unbuffered 1 char reads) */
  int n=0;
  while (n<max) {
    if (recv(fd,buffer,1,0)!=1) {
      n= -n;
      break;
    }
    n++;
    if (*buffer=='\015') continue; /* ignore CR */
    if (*buffer=='\012') break;    /* LF is the separator */
    buffer++;
  }
  *buffer=0;
  return n;
}


/*
 * read data from file descriptor
 * retries reading until the number of bytes requested is read.
 * returns the number of bytes read. negative if a read error (EOF) occured
 * before the requested length.
 */
static int http_read_buffer (int fd, /* file descriptor to read from */
                             char *buffer,  /* placeholder for data */
                             int length)  /* number of bytes to read */
{
  int n,r;
  for (n=0; n<length; n+=r) {
    r=recv(fd,buffer,length-n,0);
    if (r<=0) return -n;
    buffer+=r;
  }
  return n;
}


typedef enum 
{
  CLOSE,  /* Close the socket after the query (for put) */
  KEEP_OPEN /* Keep it open */
} querymode;

#ifndef OSK

//static http_retcode http_query(const char *command, char *url,
//			       const char *additional_header, querymode mode, 
//			       char* data, int length, int *pfd);
#endif

/* beware that filename+type+rest of header must not exceed MAXBUF */
/* so we limit filename to 256 and type to 64 chars in put & get */
#define MAXBUF 512

/*
 * Pseudo general http query
 *
 * send a command and additional headers to the http server.
 * optionally through the proxy (if http_proxy_server and http_proxy_port are
 * set).
 *
 * Limitations: the url is truncated to first 256 chars and
 * the server name to 128 in case of proxy request.
 */
static http_retcode http_query(const char *command, /* command to send  */
                               char *url, /* url / filename queried  */
                               const char *additional_header, /* additional header */
                               querymode mode, /* type of query */
                               const char *data, /* Data to send after header. If NULL, not data is sent */
                               int length, /* size of data */
                               int *pfd) /* pointer to variable where to set file descriptor value */
{
  int     s;
  struct  hostent *hp;
  struct  sockaddr_in     server;
  char header[MAXBUF];
  int  hlg;
  http_retcode ret = OK0;
  int  proxy=(http_proxy_server!=NULL && http_proxy_port!=0);
  int  port = proxy ? http_proxy_port : http_port ;
  
  if (pfd) *pfd=-1;

  /* get host info by name :*/
  if ((hp = gethostbyname( proxy ? http_proxy_server 
			         : ( http_server ? http_server 
				                 : SERVER_DEFAULT )
                         ))) {
    memset((char *) &server,0, sizeof(server));
    memmove((char *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = (unsigned short) htons( port );
  } else
    return ERRHOST;

  /* create socket */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return ERRSOCK;
  setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0);

  /* connect to server */
  if (connect(s, (sockaddr*)&server, sizeof(server)) < 0) // Original http_lib lacked sockaddr cast-- is this safe?
    ret=ERRCONN;
  else {
    if (pfd) *pfd=s;
    
    /* create header */
    if (proxy) {
      sprintf(header,
"%s http://%.128s:%d/%.256s HTTP/1.0\015\012User-Agent: %s\015\012%s\015\012",
	      command,
	      http_server,
	      http_port,
	      url,
	      http_user_agent,
	      additional_header
	      );
    } else {
      sprintf(header,
"%s /%.256s HTTP/1.0\015\012User-Agent: %s\015\012%s\015\012",
	      command,
	      url,
	      http_user_agent,
	      additional_header
	      );
    }
    
    hlg=strlen(header);

    /* send header */
    if (send(s,header,hlg,0)!=hlg)
      ret= ERRWRHD;

    /* send data */
    else if (length && data && (send(s,data,length,0)!=length) ) 
      ret= ERRWRDT;

    else {
      /* read result & check */
      ret=(http_retcode)http_read_line(s,header,MAXBUF-1);
#ifdef VERBOSE
      fputs(header,stderr);
      putc('\n',stderr);
#endif	
      if (ret<=0) 
	ret=ERRRDHD;
      else if (sscanf(header,"HTTP/1.%*d %03d",(int*)&ret)!=1) 
	  ret=ERRPAHD;
      else if (mode==KEEP_OPEN)
	return ret;
    }
  }
  /* close socket */
  close(s);
  return ret;
}


/*
 * Put data on the server
 *
 * This function sends data to the http data server.
 * The data will be stored under the ressource name filename.
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters 
 *              and type to 64.
 */
http_retcode http_put(char *filename, /* name of the ressource to create */
                      const char *data, /* pointer to the data to send   */
                      int length, /* length of the data to send  */
                      int overwrite, /* flag to request to overwrite the ressource if it was already existing */
                      const char *type) /* type of the data, if NULL default type is used */
{
  char header[MAXBUF];
  if (type) 
    sprintf(header,"Content-length: %d\015\012Content-type: %.64s\015\012%s",
	    length,
	    type  ,
	    overwrite ? "Control: overwrite=1\015\012" : ""
	    );
  else
    sprintf(header,"Content-length: %d\015\012%s",length,
	    overwrite ? "Control: overwrite=1\015\012" : ""
	    );
  return http_query("PUT",filename,header,CLOSE, data, length, NULL);
}


/*
 * Get data from the server
 *
 * This function gets data from the http data server.
 * The data is read from the ressource named filename.
 * Address of new new allocated memory block is filled in pdata
 * whose length is returned via plength.
 * 
 * returns a negative error code or a positive code from the server
 * 
 *
 * limitations: filename is truncated to first 256 characters
 */
http_retcode http_get(char *filename, /* name of the ressource to read */
                      char **pdata, /* address of a pointer variable which will be set to point toward allocated memory containing read data.*/
                      int *plength, /* address of integer variable which will be set to length of the read data */
                      char *typebuf) /* allocated buffer where the read data type is returned. If NULL, the type is not returned */
{
  http_retcode ret;
  
  char header[MAXBUF];
  char *pc;
  int  fd;
  int  n,length=-1;

  if (!pdata) return ERRNULL; else *pdata=NULL;
  if (plength) *plength=0;
  if (typebuf) *typebuf='\0';

  ret=http_query("GET",filename,"",KEEP_OPEN, NULL, 0, &fd);
  if (ret==200) {
    while (1) {
      n=http_read_line(fd,header,MAXBUF-1);
#ifdef VERBOSE
      fputs(header,stderr);
      putc('\n',stderr);
#endif	
      if (n<=0) {
	close(fd);
	return ERRRDHD;
      }
      /* empty line ? (=> end of header) */
      if ( n>0 && (*header)=='\0') break;
      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
        for (pc=header; (*pc!=':' && *pc) ; pc++) { *pc=tolower(*pc); }
      sscanf(header,"content-length: %d",&length);
//      if (typebuf) sscanf(header,"content-type: %s",typebuf); // Holy crud unsafe!
    }
    if (length<=0) {
      close(fd);
      return ERRNOLG;
    }
    if (plength) *plength=length;
    if (!(*pdata=(char *)malloc(length+1))) {
      close(fd);
      return ERRMEM;
    }
    (*pdata)[length] = '\0';
    n=http_read_buffer(fd,*pdata,length);
    close(fd);
    if (n!=length) ret=ERRRDDT;
  } else if (ret>=0) close(fd);
  return ret;
}

// Combines _put and _get. TODO: This could be done without code duplication.
http_retcode http_putget(char *filename, /* name of the ressource to create */
                      const char *data, /* pointer to the data to send   */
                      int inlength, /* length of the data to send  */
                      int overwrite, /* flag to request to overwrite the ressource if it was already existing */
                      const char *type, /* type of the data, if NULL default type is used */
                      char **pdata, /* address of a pointer variable which will be set to point toward allocated memory containing read data.*/
                      int *plength, /* address of integer variable which will be set to length of the read data */
                      char *typebuf) /* allocated buffer where the read data type is returned. If NULL, the type is not returned */
{
    char header[MAXBUF];
    if (type) 
        sprintf(header,"Content-length: %d\015\012Content-type: %.64s\015\012%s",
                inlength,
                type  ,
                overwrite ? "Control: overwrite=1\015\012" : ""
                );
    else
        sprintf(header,"Content-length: %d\015\012%s",inlength,
                overwrite ? "Control: overwrite=1\015\012" : ""
                );
    
    http_retcode ret;
    
    char *pc;
    int  fd;
    int  n,length=-1;
    
    if (!pdata) return ERRNULL; else *pdata=NULL;
    if (plength) *plength=0;
    if (typebuf) *typebuf='\0';
    
    ret=http_query("PUT",filename,header,KEEP_OPEN, data, inlength, &fd);
    if (ret==200) {
        while (1) {
            n=http_read_line(fd,header,MAXBUF-1); // Note: Header overwritten here
#ifdef VERBOSE
//            fprintf(stderr, "%d:[%s]\n", n, header); // Clearer than the original verbose...
            fputs(header,stderr);
            putc('\n',stderr);
#endif	
            if (n<=0) {
//                perror("Putget");
                close(fd);
                return ERRRDHD;
            }
            /* empty line ? (=> end of header) */
            if ( n>0 && (*header)=='\0') break;
            /* try to parse some keywords : */
            /* convert to lower case 'till a : is found or end of string */
            for (pc=header; (*pc!=':' && *pc) ; pc++) { *pc=tolower(*pc); }
            sscanf(header,"content-length: %d",&length);
//            if (typebuf) sscanf(header,"content-type: %s",typebuf); // Again: NO
        }
        if (length<=0) {
            close(fd);
            return ERRNOLG;
        }
        if (plength) *plength=length;
        if (!(*pdata=(char *)malloc(length+1))) {
            close(fd);
            return ERRMEM;
        }
        (*pdata)[length] = '\0';
        n=http_read_buffer(fd,*pdata,length);
        close(fd);
        if (n!=length) ret=ERRRDDT;
    } else if (ret>=0) close(fd);
    return ret;    
}


/*
 * Request the header
 *
 * This function outputs the header of thehttp data server.
 * The header is from the ressource named filename.
 * The length and type of data is eventually returned (like for http_get(3))
 *
 * returns a negative error code or a positive code from the server
 * 
 * limitations: filename is truncated to first 256 characters
 */
http_retcode http_head(char *filename, /* name of the ressource to read */
                       int *plength, /* address of integer variable which will be set to length of the data */
                       char *typebuf) /* allocated buffer where the data type is returned. If NULL, the type is not returned */
{
/* mostly copied from http_get : */
  http_retcode ret;
  
  char header[MAXBUF];
  char *pc;
  int  fd;
  int  n,length=-1;

  if (plength) *plength=0;
  if (typebuf) *typebuf='\0';

  ret=http_query("HEAD",filename,"",KEEP_OPEN, NULL, 0, &fd);
  if (ret==200) {
    while (1) {
      n=http_read_line(fd,header,MAXBUF-1);
#ifdef VERBOSE
      fputs(header,stderr);
      putc('\n',stderr);
#endif	
      if (n<=0) {
	close(fd);
	return ERRRDHD;
      }
      /* empty line ? (=> end of header) */
      if ( n>0 && (*header)=='\0') break;
      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
      for (pc=header; (*pc!=':' && *pc) ; pc++) *pc=tolower(*pc);
      sscanf(header,"content-length: %d",&length);
//      if (typebuf) sscanf(header,"content-type: %s",typebuf); // Again: NO
    }
    if (plength) *plength=length;
    close(fd);
  } else if (ret>=0) close(fd);
  return ret;
}



/*
 * Delete data on the server
 *
 * This function request a DELETE on the http data server.
 *
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters 
 */

http_retcode http_delete(char *filename) /* name of the ressource to create */
{
  return http_query("DELETE",filename,"",CLOSE, NULL, 0, NULL);
}



/* parses an url : setting the http_server and http_port global variables
 * and returning the filename to pass to http_get/put/...
 * returns a negative error code or 0 if sucessfully parsed.
 */
http_retcode http_parse_url(char *url,     /* writeable copy of an url */
                            char **pfilename)    /* address of a pointer that will be filled with allocated filename
                                                  * the pointer must be equal to NULL before calling or it will be 
                                                  * automatically freed (free(3))
                                                  */
{
  char *pc,c;
  
  http_port=80;
  if (http_server) {
    free(http_server);
    http_server=NULL;
  }
  if (*pfilename) {
    free(*pfilename);
    *pfilename=NULL;
  }
  
  if (strncasecmp("http://",url,7)) {
#ifdef VERBOSE
    fprintf(stderr,"invalid url (must start with 'http://')\n");
#endif
    return ERRURLH;
  }
  url+=7;
  for (pc=url,c=*pc; (c && c!=':' && c!='/');) c=*pc++;
  *(pc-1)=0;
  if (c==':') {
    if (sscanf(pc,"%d",&http_port)!=1) {
#ifdef VERBOSE
      fprintf(stderr,"invalid port in url\n");
#endif
      return ERRURLP;
    }
    for (pc++; (*pc && *pc!='/') ; pc++) ;
    if (*pc) pc++;
  }

  http_server=strdup(url);
  *pfilename= strdup ( c ? pc : "") ;

#ifdef VERBOSE
  fprintf(stderr,"host=(%s), port=%d, filename=(%s)\n",
	    http_server,http_port,*pfilename);
#endif
  return OK0;
}

