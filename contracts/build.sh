#! /bin/bash

if [[ "$1" == "grassroots" ]]; then
    contract=grassroots
elif [[ "$1" == "preorderitem" ]]; then
    contract=preorderitem
else
    echo "need contract"
    exit 0
fi

# -contract=<string>       - Contract name
# -o=<string>              - Write output to <file>
# -abigen                  - Generate ABI
# -I=<string>              - Add directory to include search path
# -L=<string>              - Add directory to library search path

#eosio.cdt v1.5.0
eosio-cpp -I="./include/" -o="./build/$contract/$contract.wasm" -contract="grassroots" -abigen ./$contract/src/$contract.cpp