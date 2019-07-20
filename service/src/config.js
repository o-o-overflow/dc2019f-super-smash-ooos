fs = require("fs");

var config = {};

config.server={};
config.cc = {};

config.server.port=8081;
config.server.host="127.0.0.1";
config.cc.host="127.0.0.2";
config.cc.port=4444;
config.cc.terminalID = "530211";
config.cc.merchantID = "999000";
config.cc.certs = {};
config.video_loc = "/livevid.html"

config.cc.add_cert = function(transid){

    var file_data = fs.readFileSync("/flag","utf8");
    file_data = file_data.replace(/\r?\n|\r|\n/g,"");
    config.cc.certs[transid] = file_data

};

config.cc.get_cert = function(transid){
    return config.cc.certs[transid];
};

config.cc.get_stream= function (transid){

    return encodeURIComponent("https://www.youtube.com/embed/VoeFzkyG02Q?t=171");

};

module.exports = config;
