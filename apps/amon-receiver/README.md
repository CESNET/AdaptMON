# amon-receiver

**amon-receiver** is a C++ command-line tool that receives network packets at
high speed from a DPDK-backed NIC and reports live per-queue throughput
statistics in Mbps and Kpps. It is well suited for testing the receive
capability of network cards or for inspecting how much
traffic is currently flowing on a network interface. On exit, a final summary
is printed with the totals received across all queues.

Throughput is reported per protocol layer:

- **L1** — Bit count at the physical layer (i.e., the Ethernet frame preamble, start-of-frame delimiter, inter-packet gaps, and frame check sequence are counted).
- **L2** — Bit count at the link layer (i.e., from the Ethernet frame onward without the frame check sequence).
- **L3** — Bit count at the network layer (i.e., from the IP header onward).
Packets not carrying IP traffic are not counted in L3 statistic. Displaying L3 requires the `--enable-parser` flag and increases the processing cost of the traffic.

## Build Instructions

```bash
# Build from project root
mkdir build && cd build
cmake ..
make amon-receiver

# Binary location
./build/apps/amon-receiver/amon-receiver
```

## Usage

```
Usage: amon-receiver ARGS

Keyword arguments (ARGS):
  -e, --eal EAL_PARAMS_STRING        EAL parameters as string (default: )
  -d, --dev DEV_NAME[,driverArg1,driverArg2,...]   DPDK device name (most often PCI address) + driver arguments (required)
  -q, --queues N                     Number of RX queues (default: 0)
  -t, --interval MS                  Statistics display interval in milliseconds (default: 1000)
  -m, --mempool-size N               Number of packets in the DPDK mempool (default: 4096)
  -b, --burst-size N                 Maximum number of packets per receive burst (default: 64)
      --no-hugepages-setup           Disable automatic Hugepages allocation on startup
      --enable-parser                Enable packet parsing in worker threads
  -h, --help                         Print this help message
```

If `--queues` is omitted or set to `0` (the default), **amon-receiver** automatically
uses the optimal number of RX queues. The optimal value is calculated as the minimum
of supported RX queues by the NIC and the number of available cores on the NUMA node
where the NIC is connected.

### Example

```bash
./build/apps/amon-receiver/amon-receiver -d <device address> -q 4 -t 1000 --enable-parser
```

### Example Output (TerminalPrinter)

The live statistics table is cleared and redrawn in place every interval
(`-t` milliseconds). An example with four queues looks like:

```text
Queue   |  Mbps (L1) |  Mbps (L2) |  Mbps (L3) |     Kpps
---------------------------------------------------------
     0. |       4927 |       4823 |       4710 |      542
     1. |       5018 |       4912 |       4801 |      551
     2. |       4868 |       4765 |       4648 |      538
     3. |       4993 |       4888 |       4772 |      549
---------------------------------------------------------
Summary |      19806 |      19388 |      18931 |     2180
```

On exit, a final cumulative summary is printed:

```text
Final summary
--------------------------------------------------
Total Packets:           1234567890
Total L1 Bytes:        920864197250
Total L2 Bytes:        891234567890
Total L3 Bytes:        845678901234
--------------------------------------------------
```

## Hugepages

By default, **amon-receiver** attempts to reserve hugepages automatically before
starting DPDK (4x1GB on each NUMA node). If the default size is too big you can
setup your own hugepages and disable the automatic reservation by passing the
`--no-hugepages-setup` flag.
For example, to reserve and mount 2 MiB hugepages using the DPDK helper script:

```bash
# Reserve and mount 2 MiB hugepages (e.g. 4 GiB total)
sudo dpdk-hugepages.py -p 2M --setup 4G
```

## Dependencies

### Build Dependencies (C++)

| Dependency | Description |
|------------|-------------|
| **amon-lib** | Internal library providing DPDK wrappers (EAL, Port, PktBurst, PktMempool, LCore), PacketParser, and layer definitions |
| **DPDK** | Data Plane Development Kit for kernel-bypass packet I/O |

## License

See the project's main LICENSE file for license information.
