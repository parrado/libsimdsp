#include <unistd.h>


#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <sys/types.h>        /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/ashmem.h>
#include "sharedmem.h"
#include <iostream>

#define LOGD(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGW(...) do { printf(__VA_ARGS__); printf("\n"); } while(0)

#define SOCK_PATH "test.socket"



int fd1;

double *buffer;

/**
 * Receives given file descriptior via given socket
 *
 * @param socket to be used for fd sending
 * @param fd to be sent
 * @return sendmsg result
 *
 * @note socket should be (AF_UNIX, SOCK_DGRAM)
 */
int recvfd(char *path) {
    int client_socket, len,rc,fd;
    struct sockaddr_un remote;

    int server_socket;


    char dummy = '$';



    char buf[1];
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    char cms[CMSG_SPACE(sizeof(int))];


    memset(&remote, 'x', sizeof(struct sockaddr_un));

    /****************************************/
    /* Create a UNIX domain datagram socket */
    /****************************************/
    client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket == -1) {
        //std::cout<<"SOCKET ERROR = "<<strerror(errno)<<std::endl;
        return -1;
    }



    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to send to.    */
    /***************************************/

    remote.sun_family = AF_UNIX;

    remote.sun_path[0] = '\0';
    strncpy( remote.sun_path+1, path, strlen(path));
    len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(path);





    rc = connect(client_socket, (struct sockaddr *) &remote, len);
    if(rc == -1){
        //std::cout<<"CONNECT ERROR = "<<strerror(errno)<<std::endl;
        close(client_socket);
        return -1;
    }

  //  std::cout<<"Connected"<<std::endl;

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = (caddr_t) cms;
    msg.msg_controllen = sizeof cms;


    /****************************************/
    /* Read data on the server from clients */
    /* and print the data that was read.    */
    /****************************************/

    //std::cout<<"waiting to recvfrom..."<<std::endl;

    len = recvmsg(client_socket, &msg, 0);

  //  std::cout<<"Received"<<std::endl;

    if (len < 0) {
        //LOGE("recvmsg failed with %s", strerror(errno));
        return -1;
    }

    if (len == 0) {
        //LOGE("recvmsg failed no data");
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    memmove(&fd, CMSG_DATA(cmsg), sizeof(int));



    /*****************************/
    /* Close the socket and exit */
    /*****************************/
    close(client_socket);

    return fd;


}



void main_client() {
     //std::cout<<"Waiting descriptor"<<std::endl;
  //Receives file descriptor
     fd1 = recvfd(SOCK_PATH);



   //mmaps shared memory to a process pointer
   if(fd1!=-1) {

       buffer = (double *) mmap(NULL, NSHARED * sizeof(double), PROT_READ | PROT_WRITE,    MAP_SHARED, fd1, 0);
   }


}

void writeInputSignal(short *x,int n){

  int nSamples=(NSHARED)/4;
  if(n<nSamples)
      nSamples=n;

  memset(buffer,0,(NSHARED)/4*sizeof(double));
  for(int n=0;n<nSamples;n++)
    buffer[n]=(double)x[n]/32767.0;

}

void writeOutputSignal(short *x,int n){

  int nSamples=(NSHARED)/4;
  if(n<nSamples)
      nSamples=n;

  memset(buffer+(NSHARED)/4,0,(NSHARED)/4*sizeof(double));
  for(int n=0;n<nSamples;n++)
    buffer[n+(NSHARED)/4]=(double)x[n]/32767.0;
}

void writeSamplingRate(double fs){
  buffer[SAMPLING_RATE]=fs;
}

double readSamplingRate(){
  return buffer[SAMPLING_RATE];
}

double readInputSource(){
  return buffer[INPUT_SOURCE];
}

double readSineFrequency(){
    return buffer[SINE_FREQUENCY];
}

double readNoiseFlag(){
    return buffer[NOISE_FLAG];
}

double readNoisePower(){
    return buffer[NOISE_POWER];
}
