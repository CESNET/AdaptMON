"""Tests for amon-anonymizer apps. It uses testing pcaps in pcap/ directory.

Author(s): Jan Sobol <sobol@cesnet.cz>

Copyright: (C) 2026 CESNET, z.s.p.o.
SPDX-License-Identifier: BSD-3-Clause
"""

import contextlib
import ipaddress
import logging
import pkgutil
import subprocess
from pathlib import Path

import pytest
import scapy.contrib.mpls
import scapy.layers.inet
import scapy.layers.inet6
import scapy.layers.ipsec
import scapy.layers.l2
from scapy.all import Packet, PacketList, load_contrib, rdpcap

# Load all contrib protocols
for _, module_name, _ in pkgutil.iter_modules(scapy.contrib.__path__):
    with contextlib.suppress(Exception):
        load_contrib(module_name)

RANDOM_KEY = "G1scpjbcLPRKtwelClWOfYDp1YOmJNFE"

logger = logging.getLogger(__name__)

# Mapping of Scapy layer classes -> amon LayerType names.  We use this
# direction so that multiple Scapy classes can map to the same AMON layer; for
# example different IPv6 extension headers are all considered under the generic
# "IPV6" or specific option types where needed.  When checking the output we
# look up the AMON name for an input layer and then ensure the output layer at
# the same index belongs to the same AMON group.
_SCAPY_TO_AMON = {
    scapy.layers.l2.Ether: "ETHERNET",
    scapy.contrib.mpls.MPLS: "MPLS",
    scapy.layers.l2.Dot1Q: "VLAN",
    scapy.layers.l2.GRE: "GRE",
    scapy.layers.inet.IP: "IPV4",
    scapy.layers.inet6.IPv6: "IPV6",
    # IPv6 extension headers often share the same AMON layer; we can add
    # additional entries if there are special mappings.
    scapy.layers.inet6.IPv6ExtHdrRouting: "IPV6_OPT_ROUTING",
    scapy.layers.inet6.IPv6ExtHdrSegmentRouting: "IPV6_OPT_ROUTING",
    scapy.layers.inet6.IPv6ExtHdrFragment: "IPV6_OPT_FRAGMENT",
    scapy.layers.inet6.IPv6ExtHdrHopByHop: "IPV6_OPT_HOPBYHOP",
    scapy.layers.inet6.IPv6ExtHdrDestOpt: "IPV6_OPT_DESTINATION_OPTIONS",
    scapy.layers.inet6.MIP6MH_Generic: "IPV6_OPT_GENERIC",
    scapy.layers.ipsec.AH: "AUTHENTICATION_HEADER",
    scapy.layers.ipsec.ESP: "ESP",
    scapy.layers.inet.UDP: "UDP",
    scapy.layers.inet.TCP: "TCP",
    scapy.layers.inet.ICMP: "ICMP",
    scapy.layers.inet6.ICMPv6Unknown: "ICMPV6",
    scapy.layers.inet6.ICMPv6ND_NA: "ICMPV6",
    scapy.layers.inet6.ICMPv6ND_NS: "ICMPV6",
    scapy.layers.inet6.ICMPv6EchoRequest: "ICMPV6",
    scapy.layers.inet6.ICMPv6MLReport2: "ICMPV6",
}


def _find_pcaps_dir() -> Path:
    return Path(__file__).parent / "pcap"


def _anonymizer_binary() -> Path:
    repo_root = Path(__file__).resolve().parents[3]
    return repo_root / "build" / "apps" / "amon-anonymizer" / "amon-anonymizer"


def _collect_ips(
    pkts: PacketList,
) -> set[ipaddress.IPv4Address | ipaddress.IPv6Address]:
    """Collect all IP addresses found in Scapy packets.

    This inspects every layer's fields and tries to validate any field
    value as an IPv4/IPv6 address using the `ipaddress` module. This
    catches addresses that appear in extension headers or options.
    """
    ips = set()
    for p in pkts:
        for layer in p.iterpayloads():
            # Try to access fields_desc which lists the field definitions
            fields_desc = getattr(layer, "fields_desc", [])
            for field_desc in fields_desc:
                field_name = field_desc.name if hasattr(field_desc, "name") else None
                if field_name:
                    attr = getattr(layer, field_name, None)
                    vals = []
                    if isinstance(attr, list):
                        vals = [str(v).strip() for v in attr]
                    elif isinstance(attr, dict):
                        vals = [str(v).strip() for v in attr.values()]
                    elif attr is not None:
                        vals = [str(attr).strip()]
                    for s in vals:
                        # Try to validate as IP address
                        try:
                            ipaddress.ip_address(s)
                            ips.add(s)
                        except (ValueError, ipaddress.AddressValueError):
                            continue

    return ips


def _collect_macs(pkts: PacketList) -> set[str]:
    """Collect all MAC addresses found in Scapy packets.

    This inspects every layer's fields (especially Ether layers) and identifies
    MAC addresses. MAC addresses are typically in Ethernet layers but can appear
    in other contexts as well.
    """
    macs = set()
    for p in pkts:
        for layer in p.iterpayloads():
            # Try to access fields_desc which lists the field definitions
            fields_desc = getattr(layer, "fields_desc", [])
            for field_desc in fields_desc:
                field_name = field_desc.name if hasattr(field_desc, "name") else None
                if field_name:
                    v = getattr(layer, field_name, None)
                    if v is not None:
                        s = str(v).strip()
                        # Validate MAC address format (XX:XX:XX:XX:XX:XX or similar)
                        if _is_valid_mac(s):
                            macs.add(s)

    return macs


def _is_valid_mac(s: str) -> bool:
    """Check if string is a valid MAC address."""
    parts = s.split(":")
    if len(parts) != 6:
        return False
    try:
        for part in parts:
            int(part, 16)
        return True
    except ValueError:
        return False


def _byte_difference_ratio(a: bytes, b: bytes) -> float:
    """Calculate the ratio of differing bytes between two byte sequences."""
    assert len(a) == len(b)
    diff = sum(x != y for x, y in zip(a, b))
    return diff / len(a)


def _unchanged_blocks(a: bytes, b: bytes, window: int = 3) -> int:
    same = 0
    for i in range(len(a) - window):
        if a[i : i + window] == b[i : i + window]:
            same += 1
    return same


def _run_anonymizer(pcap_file: Path, out_pcap: Path, bin_path: Path) -> None:
    """Invoke the anonymizer and ensure it succeeds."""
    cmd = [str(bin_path), str(pcap_file), str(out_pcap), "-k", RANDOM_KEY]
    logger.info("Running anonymizer: %s", " ".join(cmd))

    res = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    stdout_text = res.stdout.decode(errors="ignore").strip()
    stderr_text = res.stderr.decode(errors="ignore").strip()

    if stdout_text:
        logger.info("Anonymizer stdout:\n%s", stdout_text)
    if stderr_text:
        logger.info("Anonymizer stderr:\n%s", stderr_text)

    assert res.returncode == 0, f"anonymizer failed: {stderr_text}"


def _verify_parsing_and_payload(in_pkts: PacketList, out_pkts: PacketList, pcap_name: str) -> None:
    """Check layer parsing for supported layers and payload anonymization."""
    for pkt_idx, (p_in, p_out) in enumerate(zip(in_pkts, out_pkts)):
        in_layers = p_in.layers()
        out_layers = p_out.layers()
        out_layer_instances = list(p_out.iterpayloads())

        # Require that supported input layers appear at the same index in output.
        first_unsupported_idx = len(in_layers)
        for layer_idx, in_layer_class in enumerate(in_layers):
            if in_layer_class in _SCAPY_TO_AMON:
                amon_name = _SCAPY_TO_AMON[in_layer_class]
                # Output must have a parsed layer at the same index
                assert layer_idx < len(out_layers), (
                    f"Output parsing ended before supported layer '{in_layer_class.__name__}' "
                    f"(amon name: '{amon_name}'), layer {layer_idx}, packet {pkt_idx} "
                    f"in {pcap_name}"
                )
                out_layer_class = out_layers[layer_idx]
                assert in_layer_class == out_layer_class, (
                    f"Supported layer '{in_layer_class.__name__}' (amon name: '{amon_name}') at "
                    f"index {layer_idx} in input not present at same index in output for packet "
                    f"{pkt_idx} of {pcap_name}: {out_layer_class.__name__}"
                )
            else:
                # Parsing stopped at this layer; remaining bytes are payload.
                first_unsupported_idx = layer_idx
                break

        # Ensure the remaining payload bytes were anonymized (i.e. differ between input and output).
        # Compute byte offset of parsed headers using output's parsed layers
        offset = 0
        for i in range(first_unsupported_idx):
            layer = out_layer_instances[i]
            layer_bytes = bytes(layer)
            payload_obj = getattr(layer, "payload", None)
            payload_bytes = bytes(payload_obj) if payload_obj is not None else b""
            header_len = len(layer_bytes) - len(payload_bytes)
            offset += max(0, header_len)

        in_raw = bytes(p_in)
        out_raw = bytes(p_out)
        in_payload = in_raw[offset:]
        out_payload = out_raw[offset:]
        if in_payload:
            assert in_payload != out_payload, (
                f"Payload was not anonymized for packet {pkt_idx} of {pcap_name}"
            )
            # For payloads larger that 24 bytes check ratio of changed bytes to ensure that there
            # are not non-anonymized blocks
            if len(in_payload) >= 24:
                diff_ratio = _byte_difference_ratio(in_payload, out_payload)
                assert diff_ratio > 0.9, (
                    f"Payload anonymization insufficient for packet {pkt_idx} of {pcap_name}: "
                    f"{100 * (1 - diff_ratio):.2f}% bytes are identical"
                )
            unchanged_blocks = _unchanged_blocks(in_payload, out_payload)
            assert unchanged_blocks == 0, (
                f"Found {unchanged_blocks} identical 3-byte blocks in payload for packet {pkt_idx} "
                f"of {pcap_name}, indicating potential non-anonymized data"
            )


def _verify_address_anonymization(
    in_pkts: PacketList,
    out_pkts: PacketList,
    pcap_name: str,
) -> None:
    # Verify all IPs from input do not appear in output (addresses anonymized)
    in_ips = _collect_ips(in_pkts)
    out_ips = _collect_ips(out_pkts)

    # If input contained IPs, none of them should be present in the output
    if in_ips:
        intersection = in_ips & out_ips
        assert not intersection, f"Found unhashed IPs in output for {pcap_name}: {intersection}"

    # Verify all MACs from input do not appear in output (addresses anonymized)
    in_macs = _collect_macs(in_pkts)
    out_macs = _collect_macs(out_pkts)

    # If input contained MACs, none of them should be present in the output
    if in_macs:
        intersection = in_macs & out_macs
        assert not intersection, f"Found unhashed MACs in output for {pcap_name}: {intersection}"


EXCLUDED_TESTS = ["mpls_pw_nocw.pcap"]


@pytest.mark.parametrize(
    "pcap_file",
    [
        p
        for p in sorted(_find_pcaps_dir().iterdir())
        if p.suffix == ".pcap" and p.name not in EXCLUDED_TESTS
    ],
    ids=lambda p: p.name,
)
def test_pcap_anonymization_and_parsing(pcap_file: Path, tmp_path: Path) -> None:
    """Run anonymizer on each pcap and verify parsing + IP anonymization using Scapy."""
    logger.info("Testing pcap anonymization for: %s", pcap_file.name)

    bin_path = _anonymizer_binary()
    assert bin_path.exists(), f"Anonymizer binary not found at {bin_path}. Build project first."
    assert bin_path.is_file(), f"Anonymizer path {bin_path} is not a file."

    out_pcap = tmp_path / pcap_file.name

    # Run anonymizer and validate the return code
    _run_anonymizer(pcap_file, out_pcap, bin_path)

    in_pkts = rdpcap(str(pcap_file))
    out_pkts = rdpcap(str(out_pcap))
    assert len(in_pkts) == len(out_pkts), f"packet count changed for {pcap_file.name}"

    # perform grouped assertions via helper functions
    _verify_parsing_and_payload(in_pkts, out_pkts, pcap_file.name)
    _verify_address_anonymization(in_pkts, out_pkts, pcap_file.name)

    logger.info("Successfully tested: %s", pcap_file.name)


@pytest.mark.xfail
def test_pcap_anonymization_and_parsing_mpls_pw_nocw(tmp_path: Path) -> None:
    """Special case of test above."""
    logger.info("Testing special mpls_pw_nocw case")

    def parse_packet_as_pw_nocw(pkt: Packet) -> Packet:
        layer_instances = list(pkt.iterpayloads())

        last_mpls_idx = None
        for idx, layer in enumerate(layer_instances):
            if isinstance(layer, scapy.contrib.mpls.MPLS):
                last_mpls_idx = idx

        if last_mpls_idx is None:
            return pkt

        # Calculate byte offset to the end of the last MPLS layer
        offset = 0
        for i in range(last_mpls_idx + 1):
            layer = layer_instances[i]
            layer_bytes = bytes(layer)
            payload_obj = getattr(layer, "payload", None)
            payload_bytes = bytes(payload_obj) if payload_obj is not None else b""
            header_len = len(layer_bytes) - len(payload_bytes)
            offset += max(0, header_len)

        raw_packet = bytes(pkt)
        remaining_bytes = raw_packet[offset:] if offset < len(raw_packet) else b""

        return scapy.layers.l2.Ether(raw_packet[:offset]) / scapy.layers.l2.Ether(remaining_bytes)

    pcap_file = _find_pcaps_dir() / "mpls_pw_nocw.pcap"

    bin_path = _anonymizer_binary()
    assert bin_path.exists(), f"Anonymizer binary not found at {bin_path}. Build project first."
    assert bin_path.is_file(), f"Anonymizer path {bin_path} is not a file."

    out_pcap = tmp_path / pcap_file.name

    # Run anonymizer and validate the return code
    _run_anonymizer(pcap_file, out_pcap, bin_path)

    in_pkts = rdpcap(str(pcap_file))
    out_pkts = rdpcap(str(out_pcap))
    assert len(in_pkts) == len(out_pkts), f"packet count changed for {pcap_file.name}"

    in_pkts = PacketList([parse_packet_as_pw_nocw(p) for p in in_pkts])
    out_pkts = PacketList([parse_packet_as_pw_nocw(p) for p in out_pkts])

    # perform grouped assertions via helper functions
    _verify_parsing_and_payload(in_pkts, out_pkts, pcap_file.name)
    _verify_address_anonymization(in_pkts, out_pkts, pcap_file.name)

    logger.info("Successfully tested mpls_pw_nocw case")
