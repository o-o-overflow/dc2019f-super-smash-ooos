//
// DL ISO8583 library demo
//
extern "C" {

#include "iso-8583/dl_iso8583.h"
#include "iso-8583/dl_iso8583_defs_1987.h"
#include "iso-8583/dl_output.h" // for 'DL_OUTPUT_Hex'
#include "socket_communication.h"

}
#include <regex>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <SDL.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;

#endif

using namespace std;

struct msghdr hdr;
struct iovec iov[1];
struct sockaddr_in si_host;
int sock;

int loopcnt = 1;
fd_set fdr;
fd_set fdw;

typedef enum {
    MSG_READ,
    MSG_WRITE,
    FINISH,
    NONE
} msg_state_t;

typedef struct {
    int fd;
    msg_t msg;
    msg_state_t state;
} server_t;

server_t server;
msg_t iso_network_msg;
int echo_read;
int echo_wrote;

void finish(int result) {
    if (server.fd) {
        close(server.fd);
        server.fd = 0;
    }
    //printf("\tfinishing and cancelling main loop");
    emscripten_cancel_main_loop();
    //REPORT_RESULT(result);

}

char *transid;
//void(**successCallback)(int *, int);
//void(**failCallback)(int *, int);
int *successCallbackPtr = 0;
int *failCallbackPtr = 0;
char *logmsg, *fmtstr, *ccnumOut ;

#define MAXFNS 5
string str_transid;
const char ptrn_str[] = "http://"; // const char ptrn_str[] = "...'...";
regex ptrn(ptrn_str);

DL_ISO8583_HANDLER isoHandler;


//int *EMSCRIPTEN_KEEPALIVE processCC(int *ccnum_in, int *expdate_in, int *amount_in, void(*successCB)(int *, int), void(*failCB)(int *, int));


void stringToPacked(unsigned char outstr[], char *rawInput, int len) {
    for (size_t count = 0; count < len; count++) {
        sscanf(rawInput, "%2hhx", &outstr[count]);
        rawInput += 2;
    }
}

static char *genTransactionId(char *transid, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            transid[n] = charset[key];
        }
        transid[size] = '\0';
    }
    return transid;
}

void convert(char *chrarr, int *intarr, int len) {
    int tmp = 1;
    int i = 0, resindex = 0;
    //printf("\tconversion='");
    for (i = 0; i < len; i++) {
        tmp = intarr[i];
        chrarr[resindex++] = (tmp);
//        if (tmp >= 0x20 && tmp < 0x20){
//           printf("%c",tmp);
//        } else if (tmp > 0){
//          printf(" \\x%x ", tmp);
//        } else {
//          printf("\\x00");
//        }
    }
    //chrarr[resindex] = "\x00";
    //printf("' == '%s'\n", chrarr);

}

int jsparmLength(int *val, int maxFieldValue) {
    int index = 0;
    while (val[index] != '\x00') {
        index++;
    }
    if (index > maxFieldValue){
        return maxFieldValue;
    }

    return index;

}


void processServerResponse(msg_t *msg){

    DL_ISO8583_MSG isoResponseMsg;

    //DL_ISO8583_DEFS_1987_GetHandler(&isoHandler);

    DL_ISO8583_MSG_Init(NULL, 0, &isoResponseMsg);

    //printf("len=%d, msg=%s\n",msg->length, msg->buffer);

    unsigned char packedResponse[msg->length];


    DL_OUTPUT_Hex(stdout, NULL, (DL_UINT8*) msg->buffer, msg->length);

    /* unpack ISO message */
    (void) DL_ISO8583_MSG_Unpack(&isoHandler, (DL_UINT8*) msg->buffer, msg->length, &isoResponseMsg);

    /* output ISO message content */
    DL_ISO8583_MSG_Dump(stdout, NULL, &isoHandler, (DL_ISO8583_MSG*) &isoResponseMsg);

    char **respValue;
    respValue = (char **) malloc(sizeof(char **));

    (void) DL_ISO8583_MSG_GetField_Str(37, &isoResponseMsg, (DL_UINT8**) respValue);

    //printf("\nRESPONSES:%p %p '%s' '%s'\n", respValue, *respValue, *respValue, transid);
    int rc = 0;
    if (strncmp(*respValue, transid, 12) != 0) {

        (void) DL_ISO8583_MSG_GetField_Str(39, &isoResponseMsg, (DL_UINT8**) respValue);
        rc = atoi(*respValue);
        if (rc == 11 || rc == 12) {
            // keep proper transaction rc
        } else {
            //printf("transaction Id returned does not match!\n");
            rc = 99;
        }

    } else {
        (void) DL_ISO8583_MSG_GetField_Str(39,  &isoResponseMsg, (DL_UINT8**)respValue);
        rc = atoi(*respValue);
    }

    /* free ISO message */
    DL_ISO8583_MSG_Free(&isoResponseMsg);
    //free(transid);

    char* buffer = getResponseMessage(rc);

    int bufSize = strlen(buffer);
    //printf("Response code=%d, Message=%s msglen=%d\n", rc, buffer, bufSize);
    int result[bufSize + 1];
    for(int i=0; i < bufSize; i++) {
        int tmp = buffer[i];
        result[i] = tmp;// + ((unsigned char) b[i]);
    }
    result[bufSize] = (unsigned int) '\x00';
    auto arrayPtr = &result[0];


    void(*callback)(int *, int, int *);
    int callbackPtr = 0;
    if (rc ==0 && (int) *successCallbackPtr < MAXFNS){

        callbackPtr = *successCallbackPtr;

    } else {
        if ((int) *failCallbackPtr < MAXFNS ){
            //printf("using failed callback\n");
            callbackPtr = *failCallbackPtr;
        } else {
            //printf("Have messed up failcallback, attempting to use 2\n");
            callbackPtr =  2;
        }

    }
    callback = reinterpret_cast<void (*)(int*, int, int*)>(callbackPtr);
    //printf("Before callback: value  %p %p %p \n", callbackPtr, callback, *callback);
    //printf("Before callback: SUCCESS %p %p %p %p \n", &successCallback, successCallback, *successCallback, **successCallback);

    bufSize = strlen(transid);
    int transres[bufSize + 1];
    for(int i=0; i < bufSize; i++) {
        int tmp = transid[i];
        transres[i] = tmp;// + ((unsigned char) b[i]);
    }
    transres[bufSize] = (unsigned int) '\x00';

    callback(&result[0], rc, &transres[0]);

}

void comm_loop() {
    static char out[1024 * 2];
    static int pos = 0;
    fd_set fdr;
    fd_set fdw;
    int res;

    //   make sure that server.fd is ready to read / write
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);

    FD_SET(server.fd, &fdw);

    res = select(server.fd+1, &fdr, &fdw, NULL, NULL);
    //printf("\t\tselect = %d %d \n", res, server.fd);
    if (res == -1) {
        perror("select failed");
      //  printf("\tEXITING b/c slect returned -1\n");
        finish(EXIT_FAILURE);
    }

    if (server.state == MSG_WRITE ) {

        //printf("\t\tWRITE main %d %d %p %p\n", fdw, fdr, &fdw, &fdr);
        if (!FD_ISSET(server.fd, &fdw)) {
            //printf("\tEND of LOOP, WRITE not SET.\n");
            return;
        }

        res = do_msg_write(server.fd, &iso_network_msg, echo_wrote, 0, NULL, 0);

        if (res == -1) {
            return;
        } else if (res == 0) {
            perror("server closed");
            //printf("\terror exiting????\n");
            finish(EXIT_FAILURE);
        }
        //printf("\t\twrote message \n");
        server.state = MSG_READ;
        return;
    }

    if (server.state == MSG_READ) {

        //printf("\t\tREAD main %d %d %p %p\n", fdw, fdr, &fdw, &fdr);

//        if (!FD_ISSET(server.fd, &fdr)) {
//            printf("\tEND of LOOP, NOT F'ing SET\n");
//            return;
//        }

        //printf("\t\tAttempting READ  \n");
        res = do_msg_read(server.fd, &server.msg, echo_read, 0, NULL, NULL);
        //printf("\t\tread message %d\n", res);

        if (res == -1) {
            return;
        } else if (res == 0) {
            perror("server closed");
            finish(EXIT_FAILURE);
        }

        server.state = FINISH;

    }

    if (server.state == FINISH){

        //printf("EXITING WITH SUCCS! \n");

        processServerResponse(&server.msg);

        finish(EXIT_SUCCESS);
        server.state = NONE;
    }
    //printf("\tEND OF LOOP\n");
}

int *transmit_iso_msg(string ccserver_ip, int ccserver_port, char *msg, int msglen) {
    struct sockaddr_in addr;
    int res;
    //printf("transmitting\n");
    memset(&server, 0, sizeof(server_t));
    server.state = MSG_WRITE;

    // setup the message we're going to echo
    memset(&iso_network_msg, 0, sizeof(msg_t));

    iso_network_msg.length = msglen;
    iso_network_msg.buffer = (char*) malloc(iso_network_msg.length);
    memcpy(iso_network_msg.buffer, msg, iso_network_msg.length);

    echo_read = 0;
    echo_wrote = 0;

    server.fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server.fd == -1) {
        perror("cannot create socket");
        finish(EXIT_FAILURE);
    }
    fcntl(server.fd, F_SETFL, O_NONBLOCK);

    // connect the socket
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SOCKK);
    if (inet_pton(AF_INET, ccserver_ip.c_str(), &addr.sin_addr) != 1) {
        perror("inet_pton failed");
        finish(EXIT_FAILURE);
    }

    res = connect(server.fd, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1 && errno != EINPROGRESS) {
        perror("connect failed");
        finish(EXIT_FAILURE);
    }

    {
        int z;
        struct sockaddr_in adr_inet;
        socklen_t len_inet = sizeof adr_inet;
        z = getsockname(server.fd, (struct sockaddr *) &adr_inet, &len_inet);
        if (z != 0) {
            perror("getsockname");
            finish(EXIT_FAILURE);
        }

    }

    {
        int z;
        struct sockaddr_in adr_inet;
        socklen_t len_inet = sizeof adr_inet;
        z = getpeername(server.fd, (struct sockaddr *) &adr_inet, &len_inet);
        if (z != 0) {
            perror("getpeername");
            finish(EXIT_FAILURE);
        }
        char buffer[1000];
        sprintf(buffer, "%s:%u", inet_ntoa(adr_inet.sin_addr), (unsigned) ntohs(adr_inet.sin_port));
        char correct[1000];
        sprintf(correct, "127.0.0.1:%u", SOCKK);
        //printf("got (expected) socket: %s (%s), size %lu (%lu)\n", buffer, correct, strlen(buffer), strlen(correct));

    }

    //printf("FAIL =  %p / %p / %p / %p \n", &failCB, failCB, *failCB);
    //printf("FAIL = val %p / addr%p\n", successCallbackPtr, &successCallbackPtr, failCallbackPtr, &failCallbackPtr );

    //atexit(never);
    //printf("SETTING value for main loop\n");
    emscripten_set_main_loop(comm_loop, 0, 0);
    //emscripten_async_call(comm_loop, (void*) 0, 1);

    return 0;
} // end transmit iso

DL_UINT16 getFieldLen(DL_UINT16 iField){
    const DL_ISO8583_FIELD_DEF *fieldPtr  = DL_ISO8583_GetFieldDef(iField,&isoHandler);
    DL_UINT16 len = ((DL_UINT16 *) fieldPtr)[1];
    return len;
}

void set_referrer(){
    EM_ASM(
        //console.log('hello world! ', testme);

    );

}
//extern "C" {
    //int *EMSCRIPTEN_KEEPALIVE processCC(int *ccnum_in, int *expdate_in, int *amount_in, void(*successCB)(int *, int), void(*failCB)(int *, int)) {

   //int *processCC(string ccnum_in, string expdate_in, string amount_in, void(*successCB)(int *, int), void(*failCB)(int *, int)) {


int main(void) {


    return 0;

}

class CCInfo {
public:
    string ccnum, expdate, amount, location, terminalID, merchantID;
    int successCBNum = 0;
    int failCBNum = 0;
    //val failCBNum;
    int counter;
    val request;
    DL_ISO8583_MSG isoMsg;
    DL_UINT8 *packBuf;

    CCInfo(val request_in, string amount_in, string terminalID_in, string merchantID_in ):
            request(request_in),
            amount(amount_in),
            terminalID(terminalID_in),
            merchantID(merchantID_in)
    {
        //card-number=&card-holders-name=&expiry-month=&expiry-year=&cvc=
        ccnum = request.call<string>("param",val("card-number"));
        ccnum.erase(remove(ccnum.begin(), ccnum.end(), ' '), ccnum.end());
        expdate = request.call<string>("param",val("expiry-year")) + request.call<string>("param",val("expiry-month"));
        location = request.call<string>("header",val("Referer"));
        // dont do nufin
        DL_ISO8583_DEFS_1987_GetHandler(&isoHandler);
    }

    void setSuccessCallback(val jsfunc) {
        val v = val::global("Module");
        successCBNum = v.call<int>("addFunction",jsfunc);
    }

    void (*getSuccessCallback(void)) (int*, int) {
        void (*f)(int*,int) = reinterpret_cast<void (*)(int*, int)>(successCBNum);
        return f;

    }

    int getTemplateSuccessCallback(void) const {
        return successCBNum;

    }

    void setFailCallback(val jsfunc) {
        val v = val::global("Module");
        failCBNum = v.call<int>("addFunction",jsfunc);
    }

    void (*getFailCallback(void)) (int*, int) {
        void (*f)(int*,int) = reinterpret_cast<void (*)(int*, int)>(successCBNum);
        return f;

    }

    int getTemplateFailCallback(void) const {
        return successCBNum;

    }


    int generateCCMsg(DL_ISO8583_MSG *isoMsg, char *transid, char* ccnumOut) {
        char timebuffer[10];
        time_t timer;
        struct tm *tm_info;

        time(&timer);
        tm_info = localtime(&timer);

        DL_ISO8583_MSG_Init(NULL, 0, isoMsg);

        (void) DL_ISO8583_MSG_SetField_Str(0, (DL_UINT8*) "1200", &isoHandler, isoMsg);  // request for payment

        (void) DL_ISO8583_MSG_SetField_Str(2, (DL_UINT8*) ccnumOut, &isoHandler, isoMsg);
        (void) DL_ISO8583_MSG_SetField_Str(4, (DL_UINT8*) amount.c_str(), &isoHandler, isoMsg);  // amount

        //strftime(timebuffer, 7, "%M%M%S", tm_info);

        (void) DL_ISO8583_MSG_SetField_Str(7, (DL_UINT8*) "0814223344", &isoHandler, isoMsg);  // trans day/time MMDDHHMMSS eg date Aug 14, 22:33:44

        (void) DL_ISO8583_MSG_SetField_Str(11, (DL_UINT8*)"666677", &isoHandler, isoMsg);  // audit no or trace

        strftime(timebuffer, 7, "%H%M%S", tm_info);

        (void) DL_ISO8583_MSG_SetField_Str(12, (DL_UINT8*)timebuffer, &isoHandler, isoMsg);  // time 13:27:10
        (void) DL_ISO8583_MSG_SetField_Str(17, (DL_UINT8*)expdate.c_str(), &isoHandler, isoMsg);  // expiration date

        (void) DL_ISO8583_MSG_SetField_Str(37, (DL_UINT8*)transid, &isoHandler, isoMsg);    // transaction ID (retreval reference number)
        //printf("Errorcode=%d\n",errorcode);

        (void) DL_ISO8583_MSG_SetField_Str(41, (DL_UINT8*)terminalID.c_str(), &isoHandler, isoMsg); // terminal ID
        (void) DL_ISO8583_MSG_SetField_Str(42, (DL_UINT8*)merchantID.c_str(), &isoHandler, isoMsg); // merchant ID

        //free(timebuffer);
        location = regex_replace(location, ptrn, "https://");
        (void) DL_ISO8583_MSG_SetField_Str(125, (DL_UINT8*)location.c_str(), &isoHandler, isoMsg); // private use

        return 0;
    }


    string checkout(string ccserver_ip, int ccserver_port) {
        DL_UINT16 packedSize;
        char *secnd = (char *) malloc(30 * sizeof(char));
        ccnumOut = (char *) malloc(18 * sizeof(char));
        fmtstr = (char *) malloc(14 * sizeof(char));

        //
        logmsg = (char *) malloc(0x100);

        packBuf = (DL_UINT8 *) malloc(0x400 * sizeof(char));
        successCallbackPtr = (int*) malloc (sizeof(int*));
        failCallbackPtr = (int*) malloc (sizeof(int*));

        *successCallbackPtr = successCBNum;
        *failCallbackPtr = failCBNum;
        unsigned long diff = ((unsigned long) fmtstr) - ((unsigned long) ccnumOut);

        //printf("fmstr (%p) - ccnumOut (%p) == %p\n", fmtstr, ccnumOut, diff);
        //printf("failCB (%p) > logmsg (%p) == %p diff = %p\n", failCallbackPtr, logmsg,  ((unsigned long)failCallbackPtr > (unsigned long)logmsg),  ((unsigned long)failCallbackPtr - (unsigned long)logmsg));

        strcpy(secnd, fmtstr);
//        assert(diff > 0);
//        assert(diff < 0x10);
//
//        assert(((unsigned long) failCallbackPtr) > ((unsigned long) logmsg));

        transid = (char *) malloc(13 * sizeof(char));
        genTransactionId(transid, 13);

        int maxCCLen = getFieldLen(2);
        int tmp;
        strcpy(fmtstr,"%s%s%s\x00");

        strncpy(ccnumOut, ccnum.c_str(), maxCCLen);

        //printf("\nfmtstr@ %p, ccnumOut@ %p fmtstr=%s \n\n", fmtstr, ccnumOut, fmtstr);
        //printf("FAIL = val %p / addr %p / %p\n", failCallbackPtr, &failCallbackPtr, *failCallbackPtr );
        //printf("SUCC = val %p / addr %p / %p\n", successCallbackPtr, &successCallbackPtr, *successCallbackPtr );

        //packBuf = (DL_UINT8*) malloc(sizeof(DL_UINT8)*1000);  666666666666666677777777%1300p0

//        printf("&a=%08p, a=%p, *a=%p, &a_in=%p, a_in=%p, *a_in=%p, \n"
//               "&failcb=%d, &failcb=%p, fcb=%p fcb=%p, successCB=%p, &successCB=%p,scb=%p scb=%p\n"
//               "&expdate=%08p, expdate=%p, &amount=%p amount=%p, \n"
//               "01)%010p 02)%010p 03)%010p 04)%010p 05)%010p 06)%010p 07)%010p 08)%010p 09)%010p 10)%010p \n"
//               "11)%010p 12)%010p 13)%010p 14)%010p 15)%010p 16)%010p 17)%010p 18)%010p 19)%010p 20)%010p \n"
//               "21)%010p 22)%010p 23)%010p 24)%010p 25)%010p 26)%010p 27)%010p 28)%010p 29)%010p 30)%010p \n"
//               "31)%010p 32)%010p 33)%010p 34)%010p 35)%010p 36)%010p 37)%010p 38)%010p 39)%010p 40)%010p \n"
//               "41)%010p 42)%010p 43)%010p 44)%010p 45)%010p 46)%010p 47)%010p 48)%010p 49)%010p 50)%010p \n"
//               "51)%010p 52)%010p 53)%010p 54)%010p 55)%010p 56)%010p 57)%010p 58)%010p 59)%010p 60)%010p \n"
//               "61)%010p 62)%010p 63)%010p 64)%010p 65)%010p 66)%010p 67)%010p 68)%010p 69)%010p 70)%010p \n"
//               "71)%010p 72)%010p 73)%010p 74)%010p 75)%010p 76)%010p 77)%010p 78)%010p 79)%010p 80)%010p \n"
//               "81)%010p 82)%010p 83)%010p 84)%010p 85)%010p 86)%010p 87)%010p 88)%010p 89)%010p 90)%010p \n"
//               "91)%010p 92)%010p 93)%010p 94)%010p 95)%010p 96)%010p 97)%010p 98)%010p 99)%010p 100)%010p \n",
//               &ccnumOut, ccnumOut, *ccnumOut, &ccnumOut, ccnumOut, *ccnumOut,
//               failCallbackPtr, &failCallbackPtr,
//               expdateOut); //, expdate, amount, amount);

        //printf("FAIL =  %p / %p / %p / %p \n", &fcb, fcb, *fcb, **fcb);

        sprintf(logmsg, fmtstr, ccnumOut, expdate.c_str(), amount.c_str());

        //printf("After sprintf:\n\tfmtstr='%s'\n\tlogmsg='%s'\n", fmtstr, logmsg);
        //sleep(2);
        tmp = 0x99999999;
        //printf("tmp=%x \n",tmp);

        generateCCMsg(&isoMsg, transid, ccnumOut);

        DL_ISO8583_MSG_Dump(stdout, NULL, &isoHandler, &isoMsg);

        (void) DL_ISO8583_MSG_Pack(&isoHandler, &isoMsg, packBuf, &packedSize);

        DL_OUTPUT_Hex(stdout, NULL, packBuf, packedSize);

        DL_ISO8583_MSG_Free(&isoMsg);

        memset(&si_host, 0, sizeof(struct sockaddr_in));

        si_host.sin_family = AF_INET;
        si_host.sin_port = htons(ccserver_port);

        transmit_iso_msg(ccserver_ip, ccserver_port, (char*)packBuf, packedSize);

        //printf("EXITING Now!\n");

        str_transid = transid;
        return str_transid;

    }
};


EMSCRIPTEN_BINDINGS(fpointer) {
        class_<CCInfo>("CCInfo")
                .constructor<val, string, string, string >()
                .function("checkout", &CCInfo::checkout)
                .property("successCallback", &CCInfo::getTemplateSuccessCallback, &CCInfo::setSuccessCallback)
                .property("failCallback", &CCInfo::getTemplateFailCallback, &CCInfo::setFailCallback)


        ;
}


