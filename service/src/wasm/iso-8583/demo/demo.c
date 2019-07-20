//
// DL ISO8583 library demo
//

#include "dl_iso8583.h"
#include "dl_iso8583_defs_1993.h"
#include "dl_output.h" // for 'DL_OUTPUT_Hex'
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT 4444

void stringToPacked(unsigned char outstr[], char * rawInput, int len){
  for (size_t count = 0; count < len; count++) {
        sscanf(rawInput, "%2hhx", &outstr[count]);
        rawInput += 2;
    }

}

int sendToServer(char *isomsg, int msglen, char *respmsg){
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , isomsg, msglen, 0 );
    printf("Message Sent\n");
    valread = read( sock , buffer, 1024);
    printf("Returnred %s\n",buffer );
    return 0;
}



int main ( void )
{
	DL_ISO8583_HANDLER isoHandler;
	DL_ISO8583_MSG     isoMsg, isoResponseMsg;
	DL_UINT8           packBuf[1000], packRespBuf[1000];
	DL_UINT16          packedSize, packedRespSize;

	/* get ISO-8583 1993 handler */
	DL_ISO8583_DEFS_1993_GetHandler(&isoHandler);

    //
    // Populate/Pack message
    //

	/* initialise ISO message */
	DL_ISO8583_MSG_Init(NULL,0,&isoMsg);

	/* set ISO message fields */
	(void)DL_ISO8583_MSG_SetField_Str(0,"1200",&isoMsg);  // request for payment
	(void)DL_ISO8583_MSG_SetField_Str(2,"6360337890123456789", &isoMsg);
	(void)DL_ISO8583_MSG_SetField_Str(4,"5699",&isoMsg);  // amount
    (void)DL_ISO8583_MSG_SetField_Str(7,"0814223344",&isoMsg);  // trans day/time MMDDHHMMSS eg date Aug 14, 22:33:44

	(void)DL_ISO8583_MSG_SetField_Str(11,"666677",&isoMsg);  // audit no or trace
    (void)DL_ISO8583_MSG_SetField_Str(12,"132710",&isoMsg);  // time 13:27:10
    (void)DL_ISO8583_MSG_SetField_Str(17,"2105",&isoMsg);  // expiration date
    (void)DL_ISO8583_MSG_SetField_Str(37,"AAAAzzzz00009999",&isoResponseMsg);    // transaction ID (retreval reference number)

	(void)DL_ISO8583_MSG_SetField_Str(39,"4",&isoMsg);    // action code
	(void)DL_ISO8583_MSG_SetField_Str(41,"12345",&isoMsg); // terminal ID
	(void)DL_ISO8583_MSG_SetField_Str(42,"AAAAAAzzzzzz1111",&isoResponseMsg); // merchant ID
	(void)DL_ISO8583_MSG_SetField_Str(43,"OOO Web1",&isoResponseMsg); // merchant location
	(void)DL_ISO8583_MSG_SetField_Str(125,"OOO PPV Sales",&isoMsg); // private use

	/* output ISO message content */
	DL_ISO8583_MSG_Dump(stdout,NULL,&isoHandler,&isoMsg);

	/* pack ISO message */
	(void)DL_ISO8583_MSG_Pack(&isoHandler,&isoMsg,packBuf,&packedSize);
    printf("packed size = %d\n", packedSize);
	/* free ISO message */
	DL_ISO8583_MSG_Free(&isoMsg);

	/* output packed message (in hex) */
	DL_OUTPUT_Hex(stdout,NULL,packBuf,packedSize);
	char * rawResponse;

    sendToServer(packBuf, packedSize,  rawResponse);
    printf(rawResponse);



	//
	// Unpack message
	//

	/* initialise ISO message */
	DL_ISO8583_MSG_Init(NULL,0,&isoResponseMsg);

	// RESPONSE MESSAGE

	(void)DL_ISO8583_MSG_SetField_Str(0,"1210",&isoResponseMsg);  // request for payment
	//(void)DL_ISO8583_MSG_SetField_Str(2,"1234567890123456789", &isoMsg);
	(void)DL_ISO8583_MSG_SetField_Str(4,"5699",&isoResponseMsg);  // amount 56.99
    (void)DL_ISO8583_MSG_SetField_Str(11,"666677",&isoResponseMsg);  // audit no or trace
    (void)DL_ISO8583_MSG_SetField_Str(22,"01",&isoResponseMsg);  // entry mode
    // by extending a field they can overwrite the values

    (void)DL_ISO8583_MSG_SetField_Str(37,"AAAAzzzz00009999",&isoResponseMsg);    // transaction ID (retreval reference number)

	(void)DL_ISO8583_MSG_SetField_Str(39,"0",&isoResponseMsg);    // response code, 0 is good

	(void)DL_ISO8583_MSG_SetField_Str(41,"44445",&isoResponseMsg); // terminal ID


    DL_ISO8583_MSG_Dump(stdout,NULL,&isoHandler,&isoResponseMsg);

	/* pack ISO message */
	(void)DL_ISO8583_MSG_Pack(&isoHandler,&isoMsg,packRespBuf,&packedRespSize);
	/* free ISO message */
	DL_ISO8583_MSG_Free(&isoResponseMsg);

	/* output packed message (in hex) */
	DL_OUTPUT_Hex(stdout,NULL,packRespBuf,packedRespSize);



    0100 	Authorization Request 	Request from a point-of-sale terminal for authorization a purchase
    0110 	Request Response 	Request response to a point-of-sale terminal for authorization a purchase

    char *rawInput = "1234d020000002c0000000000000000000081901234567890123456789000000005699000234000431323334352020203637383930313233342020202020200009424c414820424c4148\x00";
    char * rawInput = "1210B23A80012EA180180400004014000004650000000000002050042813271000057813271004280428060403456174591700012340000=00000023057923104300A1B2C3D4E5 SOLABTEST TEST-3 DF MX010abcdefghij484012B456PRO1+000054Dynamic data generated at Mon Apr 28 13:27:11 CDT 2008ABCField of length 42 0399904ABCD031...and yet another fixed field.\x00";
    int packedInputSize = strlen(rawInput) / 2;
    unsigned char packedInput[packedInputSize];
    stringToPacked(packedInput, rawInput, packedInputSize);

	printf("packed input size=%d\n", packedInputSize, packedInput);

	DL_OUTPUT_Hex(stdout,NULL,packedInput,packedInputSize);

	/* unpack ISO message */
	(void)DL_ISO8583_MSG_Unpack(&isoHandler,packedInput,packedInputSize,&isoMsg);


	/* output ISO message content */
	DL_ISO8583_MSG_Dump(stdout,NULL,&isoHandler,&isoMsg);

	/* free ISO message */
	DL_ISO8583_MSG_Free(&isoMsg);






	return 0;
}
