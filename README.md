# Introducing SophiaTX (beta)

SophiaTX is a Delegated Proof of Stake blockchain that uses a "Proof of Brain" social consensus algorithm for token allocation.

  - Currency symbol SPHTX.
  - 10% APR inflation narrowing to 1% APR over 20 years.
  - 75% of inflation to "Proof of Brain" social consensus algorithm.
  - 15% of inflation to stake holders.
  - 10% of inflation to block producers.

# Public Announcement & Discussion

SophiaTX was announced on the
[Bitcointalk forum](https://bitcointalk.org/index.php?topic=2214715.0) prior to
the start of any mining.

# No Support & No Warranty

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

# Whitepaper

You can read the SophiaTX Whitepaper at [SophiaTXWhitePaper.pdf](https://www.sophiatx.com/storage/web/SophiaTX_Whitepaper_v1.9.pdf).

# Quickstart

Just want to get up and running quickly? We have pre-built docker images for your convenience. More details are in our [quickstart guide](doc/quickstartguide.md).

# Downloading

Latest release can be always found [here](https://github.com/SophiaTX/SophiaTX/releases)

# Building

If you would like to build from source, we do have [build instructions](https://github.com/SophiaTX/SophiaTX/blob/master/doc/building.md) for Linux (Ubuntu LTS) and macOS.

# Config File

Run `sophiatxd` once to generate a data directory and config file. The default location is `witness_node_data_dir`. Kill `sophiatxd`. It won't do anything without seed nodes. If you want to modify the config to your liking, we have two example configs. ( [full node](contrib/fullnode_config.ini),[witness node](contrib/witness_config.ini),  ) All options will be present in the default config file and there may be more options needing to be changed from the those configs (some of the options actually used in images are configured via command line).

# Seed Nodes

A list of some seed nodes to get you started can be found in
[doc/seednodes.txt](doc/seednodes.txt).

# CLI Wallet

We provide a basic cli wallet for interfacing with `sophiatxd`. The wallet is self documented via command line help. The node you connect to via the cli wallet needs to be running the `account_by_key_api`, `condenser_api`, and needs to be configured to accept websocket connections via `webserver-ws-endpoint`.

# System Requirements

For a full web node, you need at least 110GB of disk space available. SophiaTXd uses a memory mapped file which currently holds 56GB of data and by default is set to use up to 80GB. The block log of the blockchain itself is a little over 27GB. It's highly recommended to run sophiatxd on a fast disk such as an SSD or by placing the shared memory files in a ramdisk and using the `--shared-file-dir=/path` command line option to specify where. At least 16GB of memory is required for a full web node. Seed nodes (p2p mode) can run with as little as 4GB of memory with a 24 GB state file. Any CPU with decent single core performance should be sufficient. SophiaTXd is constantly growing, so you may find you need more disk space to run a full node. We are also constantly working on optimizing SophiaTX's use of disk space.
```
