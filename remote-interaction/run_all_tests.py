#! /usr/bin/env python3

import concurrent.futures
import contextlib
import subprocess
import tempfile
import logging
import random
import string
import time
import sys
import re
import os

logging.basicConfig()
_LOG = logging.getLogger("OOO")
_LOG.setLevel("DEBUG")

SERVICE_DIR = os.path.dirname(__file__)


def _run_interaction(values):
    start_time = time.time()
    script, ip, port, cnt = values
    _LOG.info(f"Test #{cnt:03d}, ${ip}, ${script}")

    try:
        cmd = f"{script} {ip} {port}".split(" ")
        # print(f"cmd={cmd}")
        output = subprocess.check_output(cmd)
        if os.path.basename(script).startswith("exploit"):
            # _LOG.info("... checking for the flag")
            assert output.find(
                b"OOO{") > -1, "exploit %s did not print the flag" % script  # pylint:disable=unsupported-membership-test
        elapsed_time = time.time()-start_time
        _LOG.info(f"Test #{cnt:03d} FINISHED in {elapsed_time:03.1f}secs, ${ip}, ${script}")

    except (AssertionError, subprocess.CalledProcessError):
        _LOG.error("check failed. Rerunning without stdio capture for convenience:")
        retval = os.system("%s %s %s" % (script, ip, port))
        _LOG.info("fyi, it returned %d that time", retval)
        raise

def run_interactions():

    _LOG.info("Remote-testing container...")


    n = 32
    interaction_files = ["/check1.py","/check2.py","/check3.py","/exploit1.py","/exploit2.py"]
    input_data = []
    for team_id in range(1, 17):
        ip = f"10.13.37.{team_id}"
        port = 8888
        temp_data = ([(SERVICE_DIR+script, ip, port) for script in interaction_files] * n)[:n]

        for cnt, t in enumerate(temp_data):
            load = (t[0], t[1],  t[2], cnt*team_id)
            input_data.append(load)

    _LOG.info(f"Starting STRESS TESTING with {len(input_data)} total tests across all teams")

    with concurrent.futures.ThreadPoolExecutor(max_workers=n*16) as pool:
        results = pool.map(_run_interaction, input_data)

    _LOG.info("Completed mapping, waiting for execution to complete")
    try:
        for _ in results:
            pass
    except Exception as e:  # pylint:disable=broad-except
        _LOG.error('One iteration returns an exception: %s', str(e), exc_info=True)
        sys.exit(1)

if __name__ == '__main__':
    run_interactions()
