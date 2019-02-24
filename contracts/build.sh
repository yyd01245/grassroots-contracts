# -contract=<string>       - Contract name
# -o=<string>              - Write output to <file>
# -abigen                  - Generate ABI
# -I=<string>              - Add directory to include search path
# -L=<string>              - Add directory to library search path

#eosio.cdt v1.5.0
eosio-cpp -I="./include/" -o="grassroots.wasm" -contract="grassroots" -abigen ./grassroots/src/grassroots.cpp