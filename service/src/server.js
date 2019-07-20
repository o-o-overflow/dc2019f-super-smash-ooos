'use strict';
var config = require("./config");

global.queryData = [];
global.binaryData = "";

const express = require('express');

const app = express();

const wasm_launcher = "./sendcc.js";

function convertIntPtr(charPtr){
    var convertedValue = ""
    for (let pointer = 0; pointer < 20; pointer++) {

        var tmp2 = Module.HEAPU32[charPtr / Uint32Array.BYTES_PER_ELEMENT  + pointer];

        if (tmp2 >= 0x20 && tmp2 < 0x7f){
            convertedValue += String.fromCharCode(tmp2) + "";
        } else {
            break;
        }
    }
    return convertedValue;
}

function processRequest(req, resp, next){
    var Module = require(wasm_launcher);
    global.Module = Module;
    Module['onRuntimeInitialized'] = onRuntimeInitialized;
    var transactionId = "";
    function successCB(ptrRespMessage, responseCode, ptrResponseTransId ) {

        var responseTransID = convertIntPtr(ptrResponseTransId);
        var cert = "";

        if (transactionId === responseTransID){
            cert = config.cc.get_cert(responseTransID);
            resp.redirect(config.video_loc + "?message=Viewing authorized&cert=" + cert + "&stream=" + config.cc.get_stream(responseTransID));
        } else {
            resp.redirect("/purchase.html?message=Authorized id did not match sent id")
        }

        delete require.cache[require.resolve(wasm_launcher)];
        next();
    }

    function failCB(ptrRespMessage, responseCode, responseTransId ) {

        var responseMessage = "";

        for (let pointer = 0; pointer < 1000; pointer++) {

            var tmp2 = Module.HEAPU32[ptrRespMessage/ Uint32Array.BYTES_PER_ELEMENT  + pointer];

            if (tmp2 >= 0x20 && tmp2 < 0x7f){
                responseMessage += String.fromCharCode(tmp2) + "";
            } else {
                break;
            }
        }

        var body = "\nresponse=" +responseCode+ "," + responseMessage + "\n";

        resp.redirect("/purchase.html?message=" + responseMessage + " please re-enter the information.");

        delete require.cache[require.resolve(wasm_launcher)];
        next();

    }

    function onRuntimeInitialized() {
        // init inputs
        try {
            var amount = 5995;
            var terminalID = config.cc.terminalID;
            var merchantID = config.cc.merchantID;

            var c = new this.CCInfo(req, amount.toString(), terminalID, merchantID);

            c.successCallback = successCB;
            c.failCallback = failCB;

            transactionId = c.checkout(config.cc.host, config.cc.port);

            config.cc.add_cert(transactionId);

        } catch (e) {
            console.log(e);
            throw e;
        }

    }

}

function errorHandler(err, req, res, next) {
    if (res.headersSent) {
        return next(err)
    }
    delete require.cache[require.resolve(wasm_launcher)];

    if (err.message !== undefined && err.stack !== undefined) {
        res.status(500);
        var errmsg = err.message
        if (err.stack.search("at run ") > -1 && err.message.search("Cannot read property 'apply' of undefined") > -1) {
            errmsg = "_compute_otp is undefined and cannot be applied";
        }
    }

    var outs = "<!DOCTYPE html>\n" +
        "<html lang=\"en\">\n" +
        "<head>\n" +
        "<meta charset=\"utf-8\">\n" +
        "<title>Error</title>\n" +
        "</head>\n" +
        "<body>\n" +
        "<pre>" + errmsg + "\n" + "</pre>\n</body>\n</html>\n";

    res.send(outs)


}

app.listen(config.server.port, config.server.host, function () {
   console.log("App listening on " + config.server.host + ":" + config.server.port );
});

app.get('/cc', processRequest);

app.use(errorHandler);
