# amon-anonymizer

**amon-anonymizer** is a C++ command-line tool designed to anonymize sensitive information in PCAP network capture files. It protects privacy by transforming identifiable network data while preserving packet structure and protocol parsing capabilities.

## Overview

### What it Anonymizes

| Data Type | Anonymization Method |
|-----------|---------------------|
| **IPv4 addresses** | CryptoPAN algorithm (prefix-preserving) |
| **IPv6 addresses** | CryptoPAN algorithm (prefix-preserving) |
| **MAC addresses** | Rijndael cipher |
| **Payload data** | Rijndael cipher |

### Layer Handling

**Supported Layers**: For protocols that the anonymizer understands (listed in Supported Protocols), the packet structure is preserved. Header fields are selectively anonymized (e.g., IP addresses, MAC addresses, checksums) while maintaining protocol validity and parsing capability.

**Unsupported Layers**: For protocols that are not explicitly supported, all data is anonymized preventively as payload. This ensures that no sensitive information leaks through unrecognized protocol structures.

## Supported Protocols

### Link Layer Types (DLT)

| DLT Constant | Description |
|--------------|-------------|
| `DLT_EN10MB` | Ethernet |
| `DLT_IPV4` | Raw IPv4 |
| `DLT_IPV6` | Raw IPv6 |
| `DLT_MPLS` | MPLS |
| `DLT_RAW` | Raw |

### Network/Transport Protocols

| Protocol | Anonymization |
|----------|---------------|
| Ethernet | Source/Dest MAC |
| VLAN (802.1Q) | Not anonymized, just counted in stats |
| MPLS | Not anonymized, just counted in stats |
| GRE | Not anonymized, just counted in stats |
| IPv4 | Source/Dest IP, checksum + options block |
| IPv6 | Source/Dest IP |
| IPv6 Segment Routing Header | IP addresses |
| IPv6 extension headers | All extension header fields except "next header" and "length" |
| TCP | Checksum, options + application payload |
| UDP | Checksum + application payload |
| ICMP | Payload |
| ICMPv6 | Payload |
| AuthenticationHeader | Integrity check value |
| Encapsulating Security Payload | Encrypted payload |

## Build Instructions

```bash
# Build from project root
mkdir build && cd build
cmake ..
make amon-anonymizer

# Binary location
./build/apps/amon-anonymizer/amon-anonymizer
```

## Usage

```
Usage: amon-anonymizer ARGS <INPUT_FILE> <OUTPUT_FILE>

Positional arguments:
  INPUT_FILE                            The input file
  OUTPUT_FILE                           The output file

Keyword arguments (ARGS):
  -k, --key KEY                         The anonymization key, 32 characters long (default: randomly generated)
  --skip-erroneous                      Skip erroneous packets (default: false)
  -h, --help                            Print this help message
```

### Example

```bash
./amon-anonymizer capture.pcap anonymized.pcap -k "MySecretKey123456789012345678901"
```

## Dependencies

### Build Dependencies (C++)

| Dependency | Description |
|------------|-------------|
| **amon-lib** | Internal library providing PacketParser, PcapReader/PcapWriter, layer definitions |
| **libpcap** | PCAP file format support |

### Test Dependencies (Python)

| Dependency | Version |
|------------|---------|
| **pytest** | >= 9.0.2 |
| **scapy** | >= 2.7.0 |
| **Python** | 3.12 |

## Testing

Tests are implemented in Python using pytest and Scapy.

### Test Strategy

1. **Parsing Verification**: Ensures supported protocol layers parse identically before/after anonymization
2. **Address Anonymization**: Verifies no original IP/MAC addresses appear in output
3. **Payload Anonymization**: Confirms payload bytes differ (>90% byte difference ratio)
4. **Packet Count Integrity**: Validates same number of packets in input/output

### Running Tests

```bash
cd apps/amon-anonymizer
uv run pytest test/test_amon_anonymizer.py -v
```

## License

See the project's main LICENSE file for license information.
