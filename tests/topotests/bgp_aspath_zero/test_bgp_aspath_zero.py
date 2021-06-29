#!/usr/bin/env python

#
# Copyright (c) 2021 by
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
Test if BGP UPDATE with AS-PATH attribute with value zero (0)
is threated as withdrawal.
"""

import os
import sys
import json
import time
import pytest
import functools

CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(CWD, "../"))

# pylint: disable=C0413
from lib import topotest
from lib.topogen import Topogen, TopoRouter, get_topogen
from lib.topolog import logger
from mininet.topo import Topo

pytestmark = [pytest.mark.bgpd]


class BgpAggregatorAsnZero(Topo):
    def build(self, *_args, **_opts):
        tgen = get_topogen(self)

        r1 = tgen.add_router("r1")
        peer1 = tgen.add_exabgp_peer(
            "peer1", ip="10.0.0.2", defaultRoute="via 10.0.0.1"
        )

        switch = tgen.add_switch("s1")
        switch.add_link(r1)
        switch.add_link(peer1)


def setup_module(mod):
    tgen = Topogen(BgpAggregatorAsnZero, mod.__name__)
    tgen.start_topology()

    router = tgen.gears["r1"]
    router.load_config(TopoRouter.RD_ZEBRA, os.path.join(CWD, "r1/zebra.conf"))
    router.load_config(TopoRouter.RD_BGP, os.path.join(CWD, "r1/bgpd.conf"))
    router.start()

    peer = tgen.gears["peer1"]
    peer.start(os.path.join(CWD, "peer1"), os.path.join(CWD, "exabgp.env"))


def teardown_module(mod):
    tgen = get_topogen()
    tgen.stop_topology()


def test_bgp_aggregator_zero():
    tgen = get_topogen()

    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    def _bgp_converge():
        output = json.loads(
            tgen.gears["r1"].vtysh_cmd("show ip bgp neighbor 10.0.0.2 json")
        )
        expected = {
            "10.0.0.2": {
                "bgpState": "Established",
                "addressFamilyInfo": {"ipv4Unicast": {"acceptedPrefixCounter": 1}},
            }
        }
        return topotest.json_cmp(output, expected)

    test_func = functools.partial(_bgp_converge)
    success, result = topotest.run_and_expect(test_func, None, count=60, wait=0.5)
    assert result is None, "More than one prefix seen at r1, SHOULD be only one."

    def _bgp_has_correct_routes_without_asn_0():
        output = json.loads(tgen.gears["r1"].vtysh_cmd("show ip bgp json"))
        expected = {"routes": {"192.168.100.101/32": [{"valid": True}]}}
        return topotest.json_cmp(output, expected)

    test_func = functools.partial(_bgp_has_correct_routes_without_asn_0)
    success, result = topotest.run_and_expect(test_func, None, count=60, wait=0.5)
    assert result is None, "Failed listing 192.168.100.101/32, SHOULD be accepted."


def test_memory_leak():
    "Run the memory leak test and report results."
    tgen = get_topogen()
    if not tgen.is_memleak_enabled():
        pytest.skip("Memory leak test/report is disabled")

    tgen.report_memory_leaks()


if __name__ == "__main__":
    args = ["-s"] + sys.argv[1:]
    sys.exit(pytest.main(args))
