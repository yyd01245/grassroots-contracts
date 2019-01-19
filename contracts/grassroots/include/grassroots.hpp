/**
 * 
 * 
 * @author Craig Branscom
 * @copyright 
 */

#pragma once
#include <eosiolib/eosio.hpp>
// #include <eosiolib/permission.hpp>
// #include <eosiolib/asset.hpp>
// #include <eosiolib/action.hpp>
// #include <eosiolib/singleton.hpp>
// #include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("grassroots")]] grassroots : public contract {

public:

    grassroots(name self, name code, datastream<const char*> ds);

    ~grassroots();

    //symbol const SYM = symbol("ROOTS", 3); //probs not

    //name const ADMIN_NAME = name("grassrootsio");

    /**
     * 
     * 
     * @scope account.value
     */
    struct [[eosio::table]] account {
        name account_name;
        asset staked_balance;
        asset spent_balance;
        //asset dividends;

        uint64_t primary_key() const { return account_name.value; }
        //TODO: EOSLIB_SERIALIZE(account, ) 
    };


    /**
     * 
     * 
     */
    struct tier {
        asset threshold;
        string reward_info;
    };

    struct perms {
        bool gr;
        bool gw;
        bool gx;
        bool r;
        bool w;
        bool x;
    };

    //Canopy idea, unrelated
    struct canopy_leaf {
        uint64_t leaf_id;
        string ipfs_hash;
        
    };

    /**
     * 
     * 
     * @scope get_self().value
     * @key project_name.value
     */
    struct [[eosio::table]] project {
        name project_name;
        name creator;
        asset requested;
        asset received;
        string info_link;
        uint32_t begin_time;
        uint32_t end_time;
        uint32_t last_edit;

        uint64_t primary_key() const { return project_name.value; }
        //TODO: EOSLIB_SERIALIZE(project, )
    };

    typedef multi_index<name("accounts"), account> accounts_table;

    typedef multi_index<name("projects"), project> projects_table;


    [[eosio::action]]
    void newaccount(name new_account);

    [[eosio::action]]
    void deposit();

    [[eosio::action]]
    void withdraw();

    [[eosio::action]]
    void backproject(name project_name, asset amount);

    [[eosio::action]]
    void returnfunds(name project_name, asset amount);


    [[eosio::action]]
    void newproject(name project_name, asset requested, string info_link);

    [[eosio::action]]
    void editproject();

    [[eosio::action]]
    void rmvproject(name project_name, name creator);

};