# Super Smash OOOs

The virtual fighting event of the decade is coming soon.  Will you be able to see the battle royale between team EMACS and team VIM?

You could just pay the $59.99 with your credit card, but who wants to do that?  What if it's not as good as promised?

## Environment
This enviornment has a front-end, back-end, and cc-processor.  The front-end uses Apache2 for static requests and connecting to the back-end. The web server uses cgi to call the back-end nodejs program.  The nodejs program calls a wasm that initiates a web socket to the cc-processor.  

Apache uses CGI instead of reverse proxying to connect to the nodejs program because the wasm's environment and the nodejs environment were too intertwined, which caused the wasm to continue running even after the nodejs responded to a request. 

The application uses cgi-node and old project that enabled CGI to work with nodejs applications. The source code was altered to work as a module instead so that node could be directly used as the execution enviornment, which made configuration of Apache2 more straight-forward. We do not use any of the session capabilities of cgi-node and they can be removed if it helps with debugging.       

During the competition the competitors were initially given the `service/src/includes/sendcc.wasm` and `service/src/cgi-bin/process.js` about 30 minutes before the end of the competition on Saturday night.  The next morning they were given `service/src/includes/sendcc.js`.

The enviornment all runs in a single docker container but the back-end and cc-processor communicate via web sockets over tcp/ip.  

## Vulnerability 1
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
Which were allocated onto the heap using:
``` 
        ccnumOut = (char *) malloc(18 * sizeof(char));
        fmtstr = (char *) malloc(14 * sizeof(char));
        logmsg = (char *) malloc(0x100);
        packBuf = (DL_UINT8 *) malloc(0x400 * sizeof(char));
        successCallbackPtr = (int*) malloc (sizeof(int*));
        failCallbackPtr = (int*) malloc (sizeof(int*));
```

With the error callback pointer overwritten, once the CC-processor returns an error, the wasm still calls the success function, which allows the attacker to receive the flag.

#Patch 1
The buffer overflow vulnerability can be patched by changing the value in the iso8583 api field to 18.

##Vulnerability 2
The second vulnerability was along a different path, the referrer header value, and was designed to be more subtle than the first vulnerability. Searching for the error messages provided trough the interface in the wasm, the attacker could discover a table of error messages used by the wasm to commicate the CC-processing error. Several of the error messages contained references to different tables used by the CC-processor. If the attacker injected a `'` into the referrer the resulting error message was different than the rest indicating "major error".  Using the information provided in the error messages, it is further possible to determine a table existed called `accounts` and it at least had a `pan, balance, and credit_limit` fields. However, creating an insert with a valid CC number (valid bin and check digit) and using the sql injection to add it to the accounts table it was possible to create your own credit card to charge.

#Patch 2
The sql vulnerability can be patched by altering a regex that attempted to replace http with https to remove any `'` found in the referrer.  
For an example of the patch see the bin diff between `patches/sql_safe_sendcc.wasm.accepted` and `patches/sendcc.wasm.accepted`.

#Exploits
Example exploits for both vulnerabilities are available in the `remote-interaction/` folder.
 






