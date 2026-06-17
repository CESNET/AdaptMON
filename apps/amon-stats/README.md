# amon-stats

**amon-stats** is a C++ command-line tool that reads packets from a PCAP capture file or live from a network interface
and reports protocol layer statistics. It parses every packet, counts how often each protocol layer occurs in each
nesting context, and builds a tree of layer occurrences that can be printed live and saved to YAML.

## Overview

For every packet, amon-stats walks the parsed layer stack (e.g. `ETHERNET -> IPV4 -> TCP -> PAYLOAD_APP`) and increments
a counter for each layer under its parent. The result is a tree where each node holds a protocol layer, its occurrence
count, and its children.

### Output

While running, the tree is redrawn in place using ANSI escape sequences. Each line shows one protocol layer in the form
`LAYER (protocol) [error]: relative% (absolute% (count))`, indented under its parent. The `(protocol)` part only appears
for `UNKNOWN` layers and `[error]` only when an error occurred while parsing the layer:

```
+- ETHERNET: 100.00% (100.00% (5332))
  +- IPV4: 99.96% (99.96% (5330))
  | +- UDP: 97.04% (97.00% (5172))
  | | +- PAYLOAD_APP: 100.00% (97.00% (5172))
  | +- TCP: 2.91% (2.91% (155))
  | | +- PAYLOAD_APP: 60.00% (1.74% (93))
  | +- ICMP: 0.04% (0.04% (2))
  | +- UNKNOWN (0x0002): 0.02% (0.02% (1))
  +- UNKNOWN (0x0806): 0.04% (0.04% (2))
Counted 5332 packets
```

| Field         | Meaning                                                              |
|---------------|----------------------------------------------------------------------|
| **protocol**  | Raw protocol value recovered for `UNKNOWN` layers (e.g. `0x0806`)    |
| **error**     | Shows what error occurred while parsing this layer.                  |
| **relative%** | Share of this layer among its siblings (relative to the parent node) |
| **absolute%** | Share of this layer relative to the total packet count               |
| **count**     | Number of packets that contained this layer in this context          |

A trailing line reports total packets counted and, when reading from a file, percent of the input file processed.

The same tree is also written to YAML, either periodically via `--save-every` and as the final `OUTPUT_FILE`. Each node
holds its `layer` name, `count`, optional `protocol` (for `UNKNOWN` layers), and nested `nodes` for its children:

```yaml
- layer: ETHERNET
  count: 5332
  nodes:
  - layer: IPV4
    count: 5330
    nodes:
    - layer: UDP
      count: 5172
      nodes:
      - layer: PAYLOAD_APP
        count: 5172
    - layer: TCP
      count: 155
      nodes:
      - layer: PAYLOAD_APP
        count: 93
    - layer: ICMP
      count: 2
    - layer: UNKNOWN
      count: 1
      protocol: "0x0002"
  - layer: UNKNOWN
    count: 2
    protocol: "0x0806"
```

### Unknown Layers

When the parser encounters a protocol it does not recognize, the layer is recorded as `UNKNOWN` and the raw protocol
bytes are recovered from the next-protocol field of the preceding layer (e.g. EtherType, IP protocol number, IPv6
next-header). The raw value is shown alongside the layer (e.g. `UNKNOWN (0x0806)`) and stored in the YAML output as a
`protocol` field.

## Build Instructions

```bash
# Build from project root
mkdir build && cd build
cmake ..
make amon-stats

# Binary location
./build/apps/amon-stats/amon-stats
```

## Usage

```
Usage: amon-stats ARGS [OUTPUT_FILE]

Positional arguments:
  OUTPUT_FILE                           Path to the output file with YAML statistics

Keyword arguments (ARGS):
  -f, --file FILE                       Read packets from the given PCAP file
  -i, --interface INTERFACE             Capture packets live from the given network interface
  -p, --print-every SECONDS             How often to print statistics, in seconds (default: don't print)
  -s, --save-every SECONDS              How often to save statistics to a file, in seconds (default: don't save)
  -c, --save-into FOLDER                Folder for periodic saves (default: current working directory)
  -h, --help                            Print this help message
```

Exactly one input source must be given: `--file` and `--interface` are mutually exclusive, and one of them is required.

Periodic saves are written into the `--save-into` folder as timestamped files named `YYYY-MM-DD_HH-MM-SS.yaml`. The
final statistics are written to `OUTPUT_FILE` if provided, then printed once to stdout.

Send `SIGINT` (Ctrl-C) or `SIGTERM` to stop processing early; statistics gathered so far are still finalized and
written.

### Example

```bash
# Read from a PCAP file, print stats every 2 seconds, save snapshots every
# 60 seconds into ./snapshots, write final result to result.yaml
./amon-stats -f capture.pcap -p 2 -s 60 -c ./snapshots result.yaml

# Capture live from an interface (stop with Ctrl-C), print stats every second.
# Live capture requires elevated privileges (CAP_NET_RAW); without them pcap fails
# with "pcap_activate() failed: You don't have permission to capture on that device".
sudo ./amon-stats -i eth0 -p 1 result.yaml
```

## Dependencies

### Build Dependencies (C++)

| Dependency   | Description                                                                                            |
|--------------|--------------------------------------------------------------------------------------------------------|
| **amon-lib** | Internal library providing PacketParser, PcapReader, layer definitions, SignalHandler and ANSI helpers |
| **libpcap**  | PCAP file format support                                                                               |

## License

See the project's main LICENSE file for license information.
