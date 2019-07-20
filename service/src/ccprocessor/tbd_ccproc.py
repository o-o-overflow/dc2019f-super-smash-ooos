#!/usr/bin/env python

import socket
import sys
import struct
import os
import traceback
import re
from py8583.py8583 import Iso8583, MemDump, Str2Bcd

from py8583.py8583spec import IsoSpec1987ASCII, IsoSpec1987BCD
from flask import Flask, render_template, request
from flask_socketio import SocketIO, send

app = Flask(__name__)
socketio = SocketIO(app)


bindata = open("bins.dat","r").read().split("\n")

@socketio.on('message')
def handle_message(message):
    print('received message: ' + message)

@app.route('/', methods=['GET'])
def main():
    try:
        print('Connected')
        print(request.form)
        #conn, addr = s.accept()
        #print('Connected: ' + addr[0] + ':' + str(addr[1]))
        #send("HAHAHAHA")
        # data = data.hex().encode("ascii")
        print(repr(data))
        isoPacket = Iso8583(data, IsoSpec1987BCD())
        transinfo = build_trans_info(isoPacket)

        response_code, approval_code = validate_transaction(transinfo)

        isoResponse = build_iso_response(transinfo, response_code, approval_code)

        respISOOut = isoResponse.BuildIso()
        print("MY RESPONSE responseISO={}".format(repr(respISOOut)))
        socketio.emit(respISOOut)

    except KeyboardInterrupt:
        print("W: Ctrl-C received, stopping…")

        #s.close()
    except Exception as ex:
        print(ex)
        traceback.print_exc()


if __name__ == '__main__':
    socketio.run(app, "0.0.0.0", port=4444)


def validate_bin(bdata, ccnum):
    for d in bdata:
        binrange, owner = d.split("\t")
        p = re.compile("^" + binrange)
        if p.match(ccnum):
            return True
    return False

def build_trans_info(isoPacket):
    tinfo = {}
    tinfo['mti'] = isoPacket.MTI()
    tinfo['ccnum'] = str(isoPacket.FieldData(2))
    tinfo['audit'] = isoPacket.FieldData(11)

    tinfo['transid'] = isoPacket.FieldData(37)

    tinfo['terminalid'] = isoPacket.FieldData(41)

    tinfo['merchantid'] = isoPacket.FieldData(42)
    tinfo['merchantloc'] = isoPacket.FieldData(43)
    tinfo['additionalinfo'] = isoPacket.FieldData(125)

    return tinfo

def build_iso_response(tinfo, response_code, approval_code):
    isoRespPacket = Iso8583(IsoSpec=IsoSpec1987BCD())

    isoRespPacket.MTI("1210")
    # print(transid)
    # print(type(transid))
    isoRespPacket.Field(11, 1)
    isoRespPacket.FieldData(11, tinfo['audit'])  # audit
    isoRespPacket.Field(37, 1)
    isoRespPacket.FieldData(37, tinfo['transid'])  # audit
    isoRespPacket.Field(41, 1)
    isoRespPacket.FieldData(41, tinfo['terminalid'])  # audit
    isoRespPacket.Field(42, 1)
    isoRespPacket.FieldData(42, tinfo['merchantid'])  # audit

    isoRespPacket.Field(38, 1)
    isoRespPacket.FieldData(38, approval_code)  # approval code

    isoRespPacket.Field(39, 1)
    isoRespPacket.FieldData(39, response_code)  # response code

    return isoRespPacket

def valid_checkdigit(ccnum):
    return True

def validate_transaction(tinfo):

    if not validate_bin(bindata, tinfo['ccnum']):
        return 999, 0

    if not valid_checkdigit(tinfo['ccnum']):
        return 999, 0

    return 0, 393
#
# HOST = ''
# PORT = 4444
#
# s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# try:
#     s.bind((HOST, PORT))
# except socket.error as msg:
#     print ('Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
#     sys.exit()
#
#
# s.listen(10)
#
#
#
# while True:
#     conn = None
#
#
#
# try:
#     s.close()
# except Exception as ex:
#     print("ERROR while trying to close socket")
#     print(ex)
