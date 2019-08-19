# 2019 DEFCON CTF Challenge -- Super Smash OOOs

## Overview
This repo contains Super Smash OOOs challenge from the 2019 DEFCON CTF Finals held in Las Vegas. The challenge was released on August 10th at 7:30 PM, which was near the end of the second day of the competition.  The competitors were initially given the `service/src/includes/sendcc.wasm` and `service/src/cgi-bin/process.js`. The next morning they were given `service/src/includes/sendcc.js`.

The description given to the competitors was the following 
```
The biggest pay-per-view event of the year live streaming Super Smash OOOs is starting in a few minutes.  
But who wants to pay $59.99, when you can watch it for free? 
```

![Super-smash-ooos-screen-shot](Super-smash-ooos-3.png "Super Smash OOOs Screen Shot")

## Environment
This enviornment has a front-end, back-end, and a credit card processing server (CC-Processor). The front-end uses Apache2 for static requests. In addition, Apache2 uses the CGI protocol to invoke back-end nodejs program.  The nodejs program calls a web assembly binary (WASM) that parses the request (form data and headers), builds an ISO8583 packet, and sends the formatted credit card data to the CC-Processor. Apache2 uses CGI instead of reverse proxying to connect to the nodejs program because the WASM's environment and the nodejs environment were too intertwined, which caused the WASM to continue running even after the nodejs responded to a request. 

To receve the CGI requests, the nodejs back-end uses a modified version of cgi-node (http://www.cgi-node.org/home). cgi-node enables nodejs applications to communicate with a webserver using the CGI protocol. cgi-node's source code was altered to work as a module instead of working as an execution environment. The changes made it easier to configure Apache2 and allowed the nodejs application to use node as the execution environment. The challenge does not use any of the session capabilities of cgi-node and they can be removed to help with debugging.       

The service runs in a single docker container but the back-end and CC-Processor communicate via web sockets over tcp/ip.  

From the service directory, the service container can be built and run.
```
docker build -t dc2019f-super-smash-ooos . && docker run -it --rm --name sssooos dc2019f-super-smash-ooos
```
It will run on port 8888.

## Vulnerability 1 - Pwn'ing the Web Assembly
The first vulnerability uses a buffer overflow in a heap value to overwrite the error callback with pointer for the success callback. The first step was to find a buffer overflow in the `CCInfo.checkout()` method, which is called from `process.js`. In `checkout()`, a variable is initialized to 18 bytes, but the length supplied by the iso8583 api uses an incorrect value of 32 bytes. The overflow, just happens, to overwrite the format string used for sprintf'ing a value to create a log message. Leveraging the format string, an attacker can overflow the log message and change the error callback pointer to the same value as the success callback pointer.  
```
    [CC Processing Error Callback pointer = 2]
    [CC Processing Success Callback pointer = 1]
    [ISO Message -- 1024 buffer ]
    [Log Message -- 256 bytes]
    [Format String Literal ]
    [CC NUM [with partial overwrite of format string ]]
    [ Other CC info …]
```
Which were allocated onto the heap using to the following in `checkout()`:
``` 
    ccnumOut = (char *) malloc(18 * sizeof(char));
    fmtstr = (char *) malloc(14 * sizeof(char));
    logmsg = (char *) malloc(0x100);
    packBuf = (DL_UINT8 *) malloc(0x400 * sizeof(char));
    successCallbackPtr = (int*) malloc (sizeof(int*));
    failCallbackPtr = (int*) malloc (sizeof(int*));
```

With the error callback pointer overwritten, once the CC-Processor returns an error, the WASM still calls the success function, which allows the attacker to receive the flag.

## Patch 1
The buffer overflow vulnerability can be patched by changing the value in the iso8583 api field to 18.

## Vulnerability 2 - SQL Injection
The second vulnerability was along a different path, the referrer header value. Discovering the CC-Processor was using a database took a bit of investigation.  By searching the WASM for the error messages presented to the user on the webpage, an attacker could discover a list of error messages used by the WASM to translate the CC-Processor's error codes into english. The error messages in the WASM reference the `accounts` table and the fields `pan, balance, and credit_limit`. 

If the attacker adds a `'` into the referrer the resulting error message is different than the rest indicating "major error". The "major error" message (instead of a validation error) indicates the `'` broke something. Based on this and the database information above it is possible to craft a query that inserts an attacker chosen credit card into the database. After inserting a credit card with a valid bin range and check digit, it is possible to watch the video and capture the flag by using the previously inserted credit card.

## Patch 2
The sql vulnerability can be patched by altering a regex that attempted to replace http with https to remove any `'` found in the referrer.  
For an example of the patch see the bin diff between `patches/sql_safe_sendcc.wasm.accepted` and `patches/sendcc.wasm.accepted`.

## Exploits
Example exploits for both vulnerabilities are available in the `remote-interaction/` folder.
 






