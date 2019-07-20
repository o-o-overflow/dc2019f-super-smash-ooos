#!/usr/bin/env python

# WS server example
import traceback
import re
import sys
import urllib.parse

from py8583.py8583 import Iso8583, MemDump, Str2Bcd, ParseError as py8583ParseError

from py8583.py8583spec import IsoSpec1987ASCII, IsoSpec1987BCD
import MySQLdb
from binascii import unhexlify
import asyncio
import websockets
import string
from time import sleep
from urllib.parse import urlparse

bindata = open("bins.dat","r").read().split("\n")

async def process_iso_message(websocket, path):

    try:
        len = await websocket.recv()
        #print(f"< {len}")

        data = await websocket.recv()

        # conn, addr = s.accept()
        # print('Connected: ' + addr[0] + ':' + str(addr[1]))

        # send("HAHAHAHA")
        # data = data.hex().encode("ascii")
        #print("Received Data: type={} len={} data={}".format(type(data), len, repr(data)))
        try :
            isoPacket = Iso8583(data, IsoSpec1987BCD())
            transinfo = build_trans_info(isoPacket)
            response_code, approval_code = validate_transaction(transinfo)
            isoResponse = build_iso_response(transinfo, response_code, approval_code)
        except py8583ParseError as pe:
            isoResponse = Iso8583(IsoSpec=IsoSpec1987BCD())
            isoResponse.MTI("1210")
            isoResponse.Field(38, 1)
            isoResponse.FieldData(38, 0)  # approval code

            isoResponse.Field(39, 1)
            if str(pe).find("F2 is larger than maximum") > -1 or str(pe).find("Cannot parse F2:") > -1:
                response_code = 11
            else:
                response_code = 12
            isoResponse.FieldData(39, response_code)  # response code

            print("[CC]ERROR, responding with a {}".format(response_code))
            #print(pe)
            traceback.print_exc()

        respISOOut = isoResponse.BuildIso()


        resp_len = 0
        for x in respISOOut:
            resp_len += 1

        print("[CC]GOING OUT type={} len={} responseISO={}".format(type(respISOOut), resp_len, respISOOut))

        await websocket.send(resp_len.to_bytes(4, byteorder="little"))
        print("[CC]Sent response length")
        await websocket.send(respISOOut)
        print("[CC]Sent iso message")


    except KeyboardInterrupt:
        print("[CC]W: Ctrl-C received, stopping…")

        # s.close()
    except Exception as ex:
        print(ex)
        traceback.print_exc()



def validate_bin(bdata, ccnum):
    for d in bdata:
        if len(d) > 3:
            binrange, owner = d.split("\t")
            p = re.compile("^" + binrange)
            if p.match(ccnum):
                return True
    return False

def build_trans_info(isoPacket):
    tinfo = {}
    tinfo['mti'] = isoPacket.MTI()
    try:
        tinfo['ccnum'] = str(isoPacket.FieldData(2))
    except py8583ParseError as pe:
        tinfo['ccnum'] = "0000000000000000"
        print("[CC]ERROR, responding with a 12")
        print(pe)
        traceback.print_exc()

    tinfo['amount'] = isoPacket.FieldData(4)
    tinfo['audit'] = isoPacket.FieldData(11)
    try:
        tinfo['transid'] = isoPacket.FieldData(37)
    except py8583ParseError as pe:
        tinfo['transid'] = "000000000000"
        print("[CC]ERROR, responding with a 12")
        print(pe)
        traceback.print_exc()

    tinfo['terminalid'] = isoPacket.FieldData(41)

    tinfo['merchantid'] = isoPacket.FieldData(42)
    tinfo['merchantloc'] = isoPacket.FieldData(43)


    tinfo['location'] = unhexlify(isoPacket.FieldData(125)).decode("latin1")
    print("[CC]found # == {}".format(tinfo['location'].find("#") > -1))

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
    # if response_code == 74:
    #     return ""
    #     #isoRespPacket.Field(45, 1)  # 40

    print("[CC]\tresponseCode = {}".format(response_code))
    isoRespPacket.FieldData(39, response_code)  # response code


    return isoRespPacket

def valid_checkdigit(ccnum):
    """ checks to make sure that the card passes a luhn mod-10 checksum """

    sum = 0
    num_digits = len(ccnum)
    oddeven = num_digits & 1

    for count in range(0, num_digits):
        digit = int(ccnum[count])

        if not ((count & 1) ^ oddeven):
            digit = digit * 2
        if digit > 9:
            digit = digit - 9

        sum = sum + digit

    return ((sum % 10) == 0)

def pan_exists(db, ccnum ):

    cursor = db.cursor()
    sql = "SELECT pan FROM accounts WHERE pan = %s "

    cursor.execute(sql,(ccnum,))

    return cursor.rowcount >= 1


def sufficient_balance(db, ccnum, amount):

    cursor = db.cursor()
    sql = "SELECT balance, credit_limit FROM accounts WHERE pan = %s "

    cursor.execute(sql,(ccnum,))
    data = cursor.fetchone()
    if not data:
        return False
    else:
        amt = float(amount) / float(100)
        cred_remaining = data[1] - data[0]
        print("[CC] funds : {} {} {} {} ".format(data[1], data[0], cred_remaining, amt ))
        if amt < cred_remaining:
            return True

    return False


def validate_referer(db, referrer):
    try:
        cursor = db.cursor()
        o = urlparse(referrer)
        ref_path = o.path
        ref_path = urllib.parse.unquote(ref_path)

        sql = "SELECT '' FROM permitted WHERE valid_referer = '{}' ".format(ref_path)

        if True:
            pass

        print("[CC]\tLocation_SQL:{}".format(sql))

        cursor.execute(sql)
        data = cursor.fetchone()
        if not data:
            return False
        else:
            return True

    except Exception as ex:
        print("[CC]ERROR: {}".format(ex))
        traceback.print_exc()
        raise Exception("SQLERROR")



def validate_transaction(tinfo):
    db = MySQLdb.connect("localhost", "ccuser", "ccuser", "ccdb")

    if tinfo['ccnum'] == "0000000000000000":
        return 11, 0

    if tinfo['transid'] == "000000000000":
        return 12, 0

    if not validate_bin(bindata, tinfo['ccnum']):
        return 15, 0

    if not valid_checkdigit(tinfo['ccnum']):
        return 14, 0

    try:
        valid_referer = validate_referer(db, tinfo['location'])
    except Exception:
        return 74, 0

    if valid_referer:
        if pan_exists(db, tinfo['ccnum']):
            if sufficient_balance(db, tinfo['ccnum'], tinfo['amount']):
                # passes
                return 0, 1001
            else:
                return 51, 0 # insufficient funds
        else:
            return 16, 0 # The pan was not found in accounts table
    else:
        if tinfo['location'].find("'") > -1:
            return 74, 0
        return 36, 0 # invalid location / referrer

    return 96, 0

    if tinfo['ccnum'] == "63603378901234566":
        return 0, 1001

    #print(tinfo['ccnum'])
    return 16, 0

print("[CC]STARTING UP")

if len(sys.argv) <= 1:
    ip = "127.0.0.1"
else:
    ip = sys.argv[1]

print("[CC]Starting up on {}:{}".format(ip, 4444))
start_server = websockets.serve(process_iso_message, ip, 4444)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()

print("[CC]FINISHING")