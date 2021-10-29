#!/usr/bin/python

#
# Copyright (c) 2021 by VMware, Inc. ("VMware")
# Used Copyright (c) 2018 by Network Device Education Foundation,
# Inc. ("NetDEF") in this file.
#
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted, provided
# that the above copyright notice and this permission notice appear
# in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND VMWARE DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL VMWARE BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.
#
"""

1. Verify mgmt commit check.
2. Verify mgmt commit apply.
3. Verify mgmt commit abort.
4. Verify mgmt delete config.
5. Kill mgmtd - verify that static routes are intact.
6. Kill mgmtd - verify that watch frr restarts.
7. Show and CLI - Execute all the newly introduced commands of mgmtd.
8. Verify mgmt rollback functionality.

"""
import sys
import time
import os
import pytest
import platform

# Save the Current Working Directory to find configuration files.
CWD = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(CWD, "../"))
sys.path.append(os.path.join(CWD, "../lib/"))

# pylint: disable=C0413
# Import topogen and topotest helpers
from lib.topogen import Topogen, get_topogen
from lib.topotest import version_cmp

# Import topoJson from lib, to create topology and initial configuration
from lib.common_config import (
    start_topology,
    write_test_header,
    write_test_footer,
    reset_config_on_routers,
    verify_rib,
    create_static_routes,
    check_address_types,
    step,
    shutdown_bringup_interface,
    stop_router,
    start_router,
    apply_raw_config,
    kill_router_daemons,
    start_router_daemons,
)
from lib.topolog import logger
from lib.bgp import verify_bgp_convergence, create_router_bgp, verify_bgp_rib
from lib.topojson import build_config_from_json

pytestmark = [pytest.mark.bgpd, pytest.mark.staticd]

# Global variables
ADDR_TYPES = check_address_types()
NETWORK = {"ipv4": ["11.0.20.1/32", "11.0.20.2/32"], "ipv6": ["2::1/128", "2::2/128"]}
NETWORK2 = {"ipv4": "11.0.20.1/32", "ipv6": "2::1/128"}
PREFIX1 = {"ipv4": "110.0.20.1/32", "ipv6": "20::1/128"}


def setup_module(mod):
    """
    Sets up the pytest environment.

    * `mod`: module name
    """

    testsuite_run_time = time.asctime(time.localtime(time.time()))
    logger.info("Testsuite start time: {}".format(testsuite_run_time))
    logger.info("=" * 40)

    logger.info("Running setup_module to create topology")

    # This function initiates the topology build with Topogen...
    json_file = "{}/yang_mgmt.json".format(CWD)
    tgen = Topogen(json_file, mod.__name__)
    global topo
    topo = tgen.json_topo
    # ... and here it calls Mininet initialization functions.

    # Starting topology, create tmp files which are loaded to routers
    #  to start deamons and then start routers
    start_topology(tgen)

    # Creating configuration from JSON
    build_config_from_json(tgen, topo)

    if version_cmp(platform.release(), "4.19") < 0:
        error_msg = (
            'These tests will not run. (have kernel "{}", '
            "requires kernel >= 4.19)".format(platform.release())
        )
        pytest.skip(error_msg)

    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    logger.info("Running setup_module() done")


def teardown_module(mod):
    """
    Teardown the pytest environment.

    * `mod`: module name
    """

    logger.info("Running teardown_module to delete topology: %s", mod)

    tgen = get_topogen()

    # Stop toplogy and Remove tmp files
    tgen.stop_topology()

    logger.info(
        "Testsuite end time: {}".format(time.asctime(time.localtime(time.time())))
    )
    logger.info("=" * 40)


def populate_nh():
    """
    Populate nexthops.
    """

    next_hop_ip = {
        "nh1": {
            "ipv4": topo["routers"]["r1"]["links"]["r2-link0"]["ipv4"].split("/")[0],
            "ipv6": topo["routers"]["r1"]["links"]["r2-link0"]["ipv6"].split("/")[0],
        },
        "nh2": {
            "ipv4": topo["routers"]["r1"]["links"]["r2-link1"]["ipv4"].split("/")[0],
            "ipv6": topo["routers"]["r1"]["links"]["r2-link1"]["ipv6"].split("/")[0],
        },
    }
    return next_hop_ip


#####################################################
#
#   Testcases
#
#####################################################


def test_mgmt_commit_check(request):
    """
    Verify mgmt commit check.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt Commit check")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt set-config xpath   /frr-routing:routing/control-plane-protocols/control-plane-protocol[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.2/32'][afi-safi='frr-routing:ipv4-unicast']/path-list[table-id='0'][distance='1']/frr-nexthops/nexthop[nh-type='blackhole'][vrf='default'][gateway=''][interface='(null)']/vrf value default"
                "mgmt commit-check"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("Mgmt Commit check")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt set-config xpath   /frr-routing:routing/control-plane-protocols/control-plane-protocol[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.2/32'][afi-safi='frr-routing:ipv4-unicast']/path-list[table-id='0'][distance='1']/frr-nexthops/nexthop[nh-type='blackhole'][vrf='default'][gateway=''][interface='(null)']/vrf value default"
                "mgmt commit-check"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("verify that the route is not configured, as commit apply not done.")

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.1/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is not True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_mgmt_commit_apply(request):
    """
    Verify mgmt commit apply.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt Commit apply")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt set-config xpath   /frr-routing:routing/control-plane-protocols/control-plane-protocol[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.2/32'][afi-safi='frr-routing:ipv4-unicast']/path-list[table-id='0'][distance='1']/frr-nexthops/nexthop[nh-type='blackhole'][vrf='default'][gateway=''][interface='(null)']/vrf value default"
                "mgmt commit-apply"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("Mgmt Commit apply")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt set-config xpath   /frr-routing:routing/control-plane-protocols/control-plane-protocol[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.2/32'][afi-safi='frr-routing:ipv4-unicast']/path-list[table-id='0'][distance='1']/frr-nexthops/nexthop[nh-type='blackhole'][vrf='default'][gateway=''][interface='(null)']/vrf value default"
                "mgmt commit-apply"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("verify that the route is configured")

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.2/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_mgmt_commit_abort(request):
    """
    Verify mgmt commit abort.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt Commit abort")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt set-config xpath   /frr-routing:routing/control-plane-protocols/control-plane-protocol[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.3/32'][afi-safi='frr-routing:ipv4-unicast']/path-list[table-id='0'][distance='1']/frr-nexthops/nexthop[nh-type='blackhole'][vrf='default'][gateway=''][interface='(null)']/vrf value default"
                "mgmt commit-abort"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("verify that the route is not configured")

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.3/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is not True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_mgmt_delete_config(request):
    """
    Verify mgmt delete config.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt delete config")
    raw_config = {
        "r1": {
            "raw_config": [
                "mgmt  delete-config xpath /frr-routing:routing/control-plane-protocols/control-plane-protoco[type='frr-staticd:staticd'][name='staticd'][vrf='default']/frr-staticd:staticd/route-list[prefix='192.1.1.3/32'][afi-safi='frr-routing:ipv4-unicast']"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("verify that the route is deleted configured")

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.3/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is not True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_mgmt_chaos_kill_daemon(request):
    """
    Kill mgmtd - verify that static routes are intact

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)
    next_hop_ip = populate_nh()

    step(
        "Configure IPv4 static route (10.1.1.1) in R2 with next hop N1"
        "(28.1.1.2 ) and N2 (29.1.1.2) , Static route next-hop present on"
        "R1"
    )
    step("ex :- ip route 10.1.1.1/24 28.1.1.2 & ip route 10.1.1.1/24 29.1.1.1")
    for addr_type in ADDR_TYPES:
        input_dict_4 = {
            "r2": {
                "static_routes": [
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": "Null0",
                    },
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": "Null0",
                    },
                ]
            }
        }

        logger.info("Configure static routes")
        result = create_static_routes(tgen, input_dict_4)
        assert result is True, "Testcase {} : Failed \n Error: {}".format(
            tc_name, result
        )

        step(
            "On R2, static route installed in RIB using show ip route"
            " with 2 ECMP next hop "
        )
        nh = [next_hop_ip["nh1"][addr_type], next_hop_ip["nh2"][addr_type]]
        dut = "r2"
        protocol = "static"
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("Kill mgmt daemon on R2.")
    kill_router_daemons(tgen, "r2", ["mgmtd"])

    for addr_type in ADDR_TYPES:
        step("Verify routes are intact in zebra.")
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("Bring up mgmtd daemon on R2.")
    start_router_daemons(tgen, "r2", ["mgmtd"])

    for addr_type in ADDR_TYPES:

        step("Verify routes are intact in zebra.")
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("Kill static daemon on R2.")
    kill_router_daemons(tgen, "r2", ["staticd"])

    for addr_type in ADDR_TYPES:
        step("Verify routes are intact in zebra.")
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is not True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("Bring up staticd daemon on R2.")
    start_router_daemons(tgen, "r2", ["staticd"])

    for addr_type in ADDR_TYPES:
        step("Verify routes are intact in zebra.")
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("Kill zebra daemon on R2.")
    kill_router_daemons(tgen, "r2", ["zebra"])

    for addr_type in ADDR_TYPES:
        step("Verify routes are removed from zebra.")
        result = verify_rib(tgen, addr_type, dut, input_dict_4, protocol=protocol)
        assert (
            result is not True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_mgmt_chaos_stop_start_frr(request):
    """
    Kill mgmtd - verify that watch frr restarts.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)
    next_hop_ip = populate_nh()

    step(
        "Configure IPv4 static route (10.1.1.1) in R2 with next hop N1"
        "(28.1.1.2 ) and N2 (29.1.1.2) , Static route next-hop present on"
        "R1"
    )
    step("ex :- ip route 10.1.1.1/24 28.1.1.2 & ip route 10.1.1.1/24 29.1.1.1")
    for addr_type in ADDR_TYPES:
        input_dict_4 = {
            "r2": {
                "static_routes": [
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": next_hop_ip["nh1"][addr_type],
                    },
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": next_hop_ip["nh2"][addr_type],
                    },
                ]
            }
        }

        logger.info("Configure static routes")
        result = create_static_routes(tgen, input_dict_4)
        assert result is True, "Testcase {} : Failed \n Error: {}".format(
            tc_name, result
        )

        step(
            "On R2, static route installed in RIB using show ip route"
            " with 2 ECMP next hop "
        )
        nh = [next_hop_ip["nh1"][addr_type], next_hop_ip["nh2"][addr_type]]
        dut = "r2"
        protocol = "static"
        result = verify_rib(
            tgen, addr_type, dut, input_dict_4, next_hop=nh, protocol=protocol
        )
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

        step("Restart frr on R1")
        stop_router(tgen, "r1")
        start_router(tgen, "r1")

        step("Verify routes are intact in zebra.")
        result = verify_rib(
            tgen, addr_type, dut, input_dict_4, next_hop=nh, protocol=protocol
        )
        assert (
            result is True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

        input_dict_4 = {
            "r2": {
                "static_routes": [
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": next_hop_ip["nh1"][addr_type],
                        "delete": True,
                    },
                    {
                        "network": NETWORK[addr_type],
                        "next_hop": next_hop_ip["nh2"][addr_type],
                        "delete": True,
                    },
                ]
            }
        }

        logger.info("Configure static routes")
        result = create_static_routes(tgen, input_dict_4)
        assert result is True, "Testcase {} : Failed \n Error: {}".format(
            tc_name, result
        )

        step("Verify routes are deleted from zebra.")
        result = verify_rib(
            tgen, addr_type, dut, input_dict_4, next_hop=nh, protocol=protocol
        )
        assert (
            result is not True
        ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


def test_show_cli(request):
    """
    This is to test all the clis for mgmt.
    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt cli check")
    raw_config = {
        "r1": {
            "raw_config": [
                "debug mgmt all"
                "debug mgmt backend"
                "debug mgmt database"
                "debug mgmt frontend"
                "debug mgmt transaction"
                "do show mgmt backend-adapter all"
                "do show mgmt backend-yang-xpath-registry"
                "do show mgmt commit-history"
                "do show mgmt database all"
                "do show mgmt database candidate"
                "do show mgmt database operational"
                "do show mgmt database running"
                "do show mgmt database-contents db-name running"
                "do show mgmt frontend-adapter all detail"
                "do show mgmt frontend-adapter all"
                "do show mgmt get-config running"
                "do show mgmt get-data running xpath /"
                "do show mgmt transaction all"
                "do show mgmt yang-xpath-subscription /"
                "mgmt commit-abort"
                "mgmt commit-apply"
                "mgmt commit-check"
                "mgmt delete-config xpath WORD"
                "mgmt load-config file WORD merge"
                "mgmt load-config file WORD replace"
                "mgmt lock-database candidate"
                "mgmt rollback last 2"
                "mgmt rollback last"
                "mgmt save-config db-name WORD file WORD"
                "mgmt set-config xpath WORD value WORD"
                "mgmt unlock-database candidate"
            ]
        }
    }

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    write_test_footer(tc_name)


def test_mgmt_rollback(request):
    """
    Verify mgmt rollback functionality.

    """
    tc_name = request.node.name
    write_test_header(tc_name)
    tgen = get_topogen()
    # Don't run this test if we have any failure.
    if tgen.routers_have_failure():
        pytest.skip(tgen.errors)

    reset_config_on_routers(tgen)

    step("Mgmt Commit check")
    raw_config = {"r1": {"raw_config": ["ip route 192.168.1.100/32 Null0"]}}

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    raw_config = {"r1": {"raw_config": ["ip route 192.168.1.101/32 Null0"]}}

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    raw_config = {"r1": {"raw_config": ["ip route 192.168.1.102/32 Null0"]}}

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    step("verify that the route is not configured, as commit apply not done.")

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.100/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    step("perform rollback on DUT")
    raw_config = {"r1": {"raw_config": ["mgmt rollback last"]}}

    result = apply_raw_config(tgen, raw_config)
    assert result is True, "Testcase {} : Failed Error: {}".format(tc_name, result)

    dut = "r1"
    protocol = "static"
    input_dict_4 = {
        "r2": {
            "static_routes": [
                {
                    "network": "192.168.1.102/32",
                    "next_hop": "Null0",
                }
            ]
        }
    }
    result = verify_rib(tgen, "ipv4", dut, input_dict_4, protocol=protocol)
    assert (
        result is not True
    ), "Testcase {} : Failed" "Error: Routes is missing in RIB".format(tc_name)

    write_test_footer(tc_name)


if __name__ == "__main__":
    args = ["-s"] + sys.argv[1:]
    sys.exit(pytest.main(args))
