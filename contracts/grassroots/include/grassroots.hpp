/**
 * 
 * 
 * @author Craig Branscom
 * @copyright 
 */

#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("grassroots")]] grassroots : public contract {

public:

    grassroots(name self, name code, datastream<const char*> ds);

    ~grassroots();

    //const symbol SYM = symbol("ROOT", 3); //probably not gonna have a token, but we'll see
    //const name ADMIN_NAME = name("grassrootsio");
    //const symbol EOS_SYM = symbol("EOS", 4);
    //const symbol BOS_SYM = symbol("BOS", 4);

    const symbol TLOS_SYM = symbol("TLOS", 4);
    const asset PROJECT_FEE = asset(250000, TLOS_SYM); //25 TLOS

    struct tier {
        name tier_name;
        asset price;
        string description;
        uint8_t pledges_left;

        EOSLIB_SERIALIZE(tier, (tier_name)(price)(description)(pledges_left))
    };


    //@scope get_self().value
    TABLE account {
        name account_name;
        asset unspent_balance;
        asset spent_balance;
        //asset dividends;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(account, (account_name)(unspent_balance)(spent_balance))
    };

    //@scope get_self().value
    TABLE project {
        name project_name;
        name category;
        name creator;

        string title;
        string description;
        string info_link;

        vector<tier> tiers;
        asset requested;
        asset received;

        uint32_t begin_time;
        uint32_t end_time;
        uint32_t last_edit;

        uint64_t primary_key() const { return project_name.value; }
        uint64_t by_cat() const { return category.value; }
        //TODO: make by_creator() index?
        //TODO: make by_end_time() index?
        EOSLIB_SERIALIZE(project, (project_name)(category)(creator)
            (title)(description)(info_link)(tiers)
            (requested)(received)
            (begin_time)(end_time)(last_edit))
    };

    //@scope get_self().value
    TABLE pledge {
        uint64_t pledge_id;
        name project_name;
        name pledger;
        asset amount;
        uint32_t timestamp;

        uint64_t primary_key() const { return pledge_id; }
        uint64_t by_project() const { return project_name.value; }
        uint64_t by_account() const { return pledger.value; }
        uint128_t by_pledge() const {
			uint128_t proj_name = static_cast<uint128_t>(project_name.value);
			uint128_t acc_name = static_cast<uint128_t>(pledger.value);
			return (proj_name << 64) | acc_name;
		}
        EOSLIB_SERIALIZE(pledge, (pledge_id)(project_name)(pledger)(amount)(timestamp))
    };

    typedef multi_index<name("accounts"), account> accounts;

    typedef multi_index<name("projects"), project,
        indexed_by<name("bycategory"), const_mem_fun<project, uint64_t, &project::by_cat>>
        > projects;

    typedef multi_index<name("pledges"), pledge,
        indexed_by<name("byproject"), const_mem_fun<pledge, uint64_t, &pledge::by_project>>,
        indexed_by<name("byaccount"), const_mem_fun<pledge, uint64_t, &pledge::by_account>>,
        indexed_by<name("bypledge"), const_mem_fun<pledge, uint128_t, &pledge::by_pledge>>
        > pledges;


    ACTION newaccount(name new_account_name);

    ACTION pledge(name project_name, name tier_name, name pledger);

    ACTION unpledge(name project_name, asset amount);

    ACTION withdraw(name account_name, asset amount);


    ACTION newproject(name project_name, name category, name creator,
        string title, string description, string info_link, asset requested);

    ACTION addtier(name project_name, name creator, 
        name tier_name, asset price, string description, uint8_t pledges);

    ACTION updateinfo(name project_name, name creator,
        string title, string description, string info_link, asset requested);

    ACTION readyproject(name project_name, name creator, uint8_t length_in_days);

    ACTION rmvproject(name project_name, name creator);

    //Functions
    bool is_valid_category(name category);
    bool is_tier_in_project(name tier_name, vector<tier> tiers);
    //vector<tier> emplace_tier_in_order(tier new_tier, vector<tier> tiers);
    int get_tier_idx(name tier_name, vector<tier> tiers);

    //Reactions
    void catch_transfer(name from, asset amount);

};