#! /usr/bin/env python
import os
import sys
import signal
import logging
import traceback
from time import sleep
from os.path import exists
from subprocess import Popen, PIPE, call

server_js = os.path.dirname(os.path.abspath(__file__)) + "/run_nodejs_server.sh"

"""
This tests various error messages from the database.
"""


def test_bad_cc_bin():

    stdout = make_contact("1770330000000002", "21","12")
    try:
        assert stdout.find(b"/purchase.html?message=No%20such%20issuer") > -1
    except AssertionError as ae:
        print("-" * 40)

        print(stdout)
        print("result={}".format(stdout.find(b"/purchase.html?message=No%20such%20issuer")))
        print("-" * 40)
        raise ae


def test_referrer_not_permitted():
    stdout = make_contact("374289111111115", "21", "12",
                          "http://ooos.crabshack.com/ccform.html")

    try:
        assert stdout.find(b"/purchase.html?message=Location%20not%20found%20in%20permitted%20table") > -1
    except AssertionError as ae:
        print(stdout)
        raise ae


def test_bad_cc_checkdig():

    stdout = make_contact("6360330000000009", "21","12")
    try:
        assert stdout.find(b"/purchase.html?message=Invalid%20pan%20") > -1
    except AssertionError as ae:
        print(stdout)
        raise ae


def test_badacct():

    stdout = make_contact("6360330000000002", "21","12")
    try:
        assert stdout.find(b"/purchase.html?message=The%20pan%20was%20not%20found%20in%20accounts%20table%") > -1
    except AssertionError as ae:
        print(stdout)
        raise ae


#
#
def make_contact(ccnum, expMM, expYY, referer_val="http://goforit.com/purchase.html"):
    url = "http://{}:{}/cc/process.js?card-number={}&expiry-year={}&expiry-month={}".format(sys.argv[1], sys.argv[2], ccnum, expYY, expMM)

    cmd = ["wget", "-O", "-", "--header", "Referer: {}".format(referer_val), url]
    p = Popen(cmd, stdout=PIPE, stderr=PIPE)

    stdout, stderr = p.communicate()

    return stdout + stderr


def run_all():
    # cmd = [server_js]  # start server
    # server_stdout = open("/tmp/server_stdout.log", "w")
    #
    # pserver = Popen(cmd, stdout=server_stdout, stderr=server_stdout)
    # sleep(.2)
    current_fn = ""
    try:

        functions = globals()
        all_functions = dict(filter((lambda kv: kv[0].startswith('test_')), functions.items()))
        for f in sorted(all_functions.keys()):
            if hasattr(all_functions[f], '__call__'):
                current_fn = f
                all_functions[f]()

    except Exception as ex:
        print("Error while processing " + current_fn)
        print(ex)
        traceback.print_exc()
        print("Error while processing " + current_fn)
        exit(99)
    finally:
        pass
        # if exists("/tmp/server_stdout.log"):
        #     print("-"*50 + "SERVER OUTPUT" + "-"*50)
        #     print(open("/tmp/server_stdout.log","r").read())
        #     print("-"*100)
        #call(["pkill", "node"])

if __name__ == "__main__":
    logging.getLogger("CCtests").setLevel("DEBUG")

    run_all()







