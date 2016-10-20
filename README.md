# ethrpc
Fast, ultra-simple C++ RPC retrieval of Ethereum blocks and transactions. Easily extended to include transaction receipts and traces.

### Usage

Download the code into a newFolder.

```
cd ./newFolder
make clean
make
```

This should produce a file in the current folder called ./ethrpc

Run the program thus:

```
./ethrpc start_block_num [end_block_num]
```

If you provide two parameters you will get all blocks within that range. If you provide one parameter only, you will get just one block. If you ask for a range, you will get a JSON array.

### Other Options

There is some commented out code in the file ethrpc.cpp that will allow you to do three things:

1. Get a single transaction by hash.
2. Get a transaction's receipt given a hash.
3. Get a trace of a transaction given a hash. *(See note below)*

These functions may be used to get important, difficult to get information such as a transaction's *error state* as well as *internal transactions* made against a contract. We will expand in this as we move forward with our project.

### Note on traceTransaction

The 'geth' node, unless it is started with a particular flag, does not support the traceTransaction call. If you want to traceTransactions, please start 'geth' with this command line (or something similar):

```
geth --rpc --rpcapi="eth,net,web3,debug"
```
Without the 'debug' flag, traceTransaction will not work.
