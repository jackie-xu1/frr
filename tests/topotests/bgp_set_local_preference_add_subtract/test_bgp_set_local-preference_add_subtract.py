#!/usr/bin/env python

#
# bgp_set_local-preference_add_subtract.py
# Part of NetDEF Topology Tests
#
# Copyright (c) 2020 by
# Donatas Abraitis <donatas.abraitis@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted, provided
# that the above copyright notice and this permission notice appear
# in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND NETDEF DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL NETDEF BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.
#

"""
bgp_set_local-preference_add_subtract.py:
Test if we can add/subtract the value to/from an existing
LOCAL_PREF in route-maps.
"""

import functools
import json
import os
import sys

import pytest
from lib import topotest
from lib.topogen import Topogen, TopoRouter, get_topogen

CWD = os.path.dirname(os.path.realpath(__file__))
def build_topo(tgen):
    for routern in range(1, 4):
        tgen.add_router("r{}".format(routern))

    switch = tgen.add_switch("s1")
    switch.add_link(tgen.gears["r1"])
    switch.add_link(tgen.gears["r2"])
    switch.add_link(tgen.gears["r3"])


def setup_module(mod):
    tgen = Topogen(build_topo, mod.__name__)
    tgen.start_topology()

    router_list = tgen.routers()

    for i, (rname, router) in enumerate(router_list.items(), 1):
        router.load_config(
            TopoRouter.RD_ZEBRA, os.path.join(CWD, "{}/zebra.conf".format(rname))
        )
        router.load_config(
            TopoRouter.RD_BGP, os.path.join(CWD, "{}/bgpd.conf".format(rname))
        )

    tgen.start_router()


def teardown_module(mod):
    tgen = get_topogen()
    tgen.stop_topology()


def test_bgp_set_local_preference():
    tgen = get_topogen()

    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    router = tgen.gears["r1"]

    def _bgp_converge(router):
        output = json.loads(router.vtysh_cmd("show ip bgp neighbor json"))
        expected = {
            "192.168.255.2": {
                "bgpState": "Established",
                "addressFamilyInfo": {"ipv4Unicast": {"acceptedPrefixCounter": 2}},
            },
            "192.168.255.3": {
                "bgpState": "Established",
                "addressFamilyInfo": {"ipv4Unicast": {"acceptedPrefixCounter": 2}},
            },
        }
        return topotest.json_cmp(output, expected)

    def _bgp_check_local_preference(router):
        output = json.loads(router.vtysh_cmd("show ip bgp 172.16.255.254/32 json"))
        expected = {
            "paths": [
                {"locPrf": 50, "nexthops": [{"ip": "192.168.255.3"}]},
                {"locPrf": 150, "nexthops": [{"ip": "192.168.255.2"}]},
            ]
        }
        return topotest.json_cmp(output, expected)

    test_func = functools.partial(_bgp_converge, router)
    success, result = topotest.run_and_expect(test_func, None, count=15, wait=0.5)

    assert result is None, 'Failed to see BGP convergence in "{}"'.format(router)

    test_func = functools.partial(_bgp_check_local_preference, router)
    success, result = topotest.run_and_expect(test_func, None, count=15, wait=0.5)

    assert result is None, 'Failed to see applied BGP local-preference in "{}"'.format(
        router
    )


if __name__ == "__main__":
    args = ["-s"] + sys.argv[1:]
    sys.exit(pytest.main(args))
