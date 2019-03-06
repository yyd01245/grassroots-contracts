/**
 * dGoods Escrow is a smart contract that allows users to redeem preorder tickets
 * on the Grassroots platform for Fungible, Non-Fungible, or Semi-Fungible dGoods.
 * 
 * @author Craig Branscom
 * @contract dgoodsescrow
 * @copyright defined in LICENSE.txt
 */

#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/singleton.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("dgoodsescrow")]] dgoodsescrow : public contract {

public:

    dgoodsescrow(name self, name code, datastream<const char*> ds);

    ~dgoodsescrow();

    const string contract_symbol = "ROOT";

    //dGoods Spec v0.1

    // scope is self
    TABLE symbolinfo { //TODO: necessary?
        string symbol;
        uint64_t global_id;
    };

    // scope is category, then token_name is unique
    TABLE tokenstats {
        bool fungible; 
        bool burnable; 
        bool transferable; 
        name issuer;
        name token_name;
        uint64_t global_id; //TODO: change to symbol type ?
        uint64_t max_supply;
        double current_supply;

        uint64_t primary_key() const { return token_name.value; }
        EOSLIB_SERIALIZE(tokenstats, (fungible)(burnable)(transferable)
            (issuer)(token_name)(global_id)(max_supply)(current_supply))
    };

    // typedef multi_index<name("accounts"), account> accounts;

    // typedef multi_index<name("categoryinfo"), categoryinfo> categoryinfo;

    // typedef multi_index<name("tokeninfo"), tokeninfo> tokeninfo;

    typedef multi_index<name("tokenstats"), tokenstats> tokenstats;

    typedef singleton<name("symbolinfo"), symbolinfo> symbolinfo;
	symbolinfo _symbolinfo;

    // CREATE: The create method instantiates a token. This is required before any tokens can be 
    // issued and sets properties such as the category, name, maximum supply, who has the ability 
    // to issue tokens, and if the token is fungible or not. Additionally, the first time this is 
    // called the symbol is recorded and subsequent calls must specify that symbol. Symbol must 
    // be A-Z, 7 character max. Name type is a string 12 characters max a-z, 1-5.

    ACTION create(name issuer, name category, name token_name, bool fungible, bool burnable,
        bool transferable, int64_t max_supply);

    // ISSUE: The issue method mints a token and gives ownership to the ‘to’ account name. For a 
    // valid call the symbol, category, and token name must have been first created. Quantity must 
    // be equal to 1 if non-fungible or semi-fungible, otherwise quantity must be greater or equal 
    // to 0.0001.

    ACTION issue(name to, name category, name token_name, double quantity, string metadata_uri, 
        string memo);

    // PAUSEXFER: Pauses all transfers of all tokens. Only callable by the contract. If pause is true, 
    // will pause. If pause is false will unpause transfers.

    ACTION pausexfer(bool pause);

    // BURNNFT: Burn method destroys specified tokens and frees the RAM. Only owner may call burn 
    // function and burnable must be true.

    ACTION burnnft(name owner, vector<uint64_t> tokeninfo_ids);

    // BURN: Burn method destroys fungible tokens and frees the RAM if all are deleted. Only owner may 
    // call Burn function and burnable must be true.

    ACTION burn(name owner, uint64_t global_id, double quantity);

    // TRANSFERNFT: Used to transfer non-fungible tokens. This allows for the ability to batch send tokens 
    // in one function call by passing in a list of token ids. Only the token owner can successfully call 
    // this function and transferable must be true.

    ACTION transfernft(name from, name to, vector<uint64_t> tokeninfo_ids, string memo);

    // TRANSFER: The standard transfer method is callable only on fungible tokens. Instead of specifying 
    // tokens individually, a token is specified by it’s global id followed by an amount desired to be sent.

    ACTION transfer(name from, name to, uint64_t global_id, double quantity, string memo);

};

// scope is self
// TABLE tokeninfo {
//     uint64_t id;
//     uint64_t serial_number;
//     name owner;
//     name category;
//     name token_name;
//     string metadata_uri;

//     uint64_t primary_key() const { return id; }
//     uint64_t get_owner() const { return owner.value; }
// };

// scope is self
// TABLE categoryinfo {
//   name category;
//   uint64_t primary_key() const { return category.value; }
// };

// scope is owner
// TABLE account {
//     name category;
//     name token_name;
//     uint64_t global_id;
//     double amount;
//     uint64_t primary_key() const { return global_id; }
// };

//Metadata Templates

//2D Game Asset

// {
//     // Required Fields
//     "name": string; identifies the asset the token represents
//     "description": string; short description of the asset the token represents
//     "imageSmall": URI pointing to image resource size 150 x 150
//     "imageLarge": URI pointing to image resource size 1024 x 1024
//     "details": Key Value pairs to render in a detail view, could be things like
//     {"strength": 5}
//     // Optional Fields
//     "authenticityImage": URI pointing to resource with mime type image representing
//     certificate of authenticity
// }