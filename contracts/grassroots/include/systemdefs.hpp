/**
 * 
 * 
 * @author Craig Branscom
 * @contract grassroots
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace std;
using namespace eosio;

//========== eosio.token ==========

struct account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.raw(); }
};

typedef eosio::multi_index<name("accounts"), account> accounts;
