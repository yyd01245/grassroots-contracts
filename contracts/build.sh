#! /bin/bash

if [[ "$1" == "grassroots" ]]; then
    contract=grassroots
elif [[ "$1" == "taskmarket" ]]; then
    contract=taskmarket
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
# -R=<string>              - Add a resource path for inclusion

#eosio.cdt v1.5.0
eosio-cpp -I="./$contract/include/" -R="./$contract/resources" -o="./build/$contract/$contract.wasm" -contract="Grassroots" -abigen ./$contract/src/$contract.cpp