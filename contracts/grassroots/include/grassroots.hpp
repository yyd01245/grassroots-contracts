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
#include <eosiolib/transaction.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("grassroots")]] grassroots : public contract {

public:

    grassroots(name self, name code, datastream<const char*> ds);

    ~grassroots();

    //const symbol EOS_SYM = symbol("EOS", 4);
    //const symbol BOS_SYM = symbol("BOS", 4);

    const name ADMIN_NAME = name("grassrootsio");
    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol ROOT_SYM = symbol("ROOT", 0);
    const asset PROJECT_FEE = asset(250000, TLOS_SYM); //25 TLOS

    struct tier {
        name tier_name;
        asset price;
        string description;
        uint16_t remaining;

        EOSLIB_SERIALIZE(tier, (tier_name)(price)(description)(remaining))
    };

    enum PROJECT_STATUS : uint8_t {
        SETUP, //0
        OPEN, //1
        FUNDED, //2
        FAILED //3
    };


    //@scope get_self().value
    TABLE account {
        name account_name;
        asset balance;
        asset dividends;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(account, (account_name)(balance)(dividends))
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
        uint8_t project_status;
        uint32_t last_edit;

        uint64_t primary_key() const { return project_name.value; }
        uint64_t by_cat() const { return category.value; }
        //TODO: make by_creator() index?
        //TODO: make by_end_time() index?
        EOSLIB_SERIALIZE(project, (project_name)(category)(creator)
            (title)(description)(info_link)
            (tiers)(requested)(received)
            (begin_time)(end_time)(project_status)(last_edit))
    };

    //@scope get_self().value
    TABLE contribution {
        uint64_t contrib_id;
        name project_name;
        name contributor;
        name tier_name;
        //asset price;

        uint64_t primary_key() const { return contrib_id; }
        uint64_t by_project() const { return project_name.value; }
        uint64_t by_account() const { return contributor.value; }
        uint128_t by_contrib() const {
			uint128_t proj_name = static_cast<uint128_t>(project_name.value);
			uint128_t acc_name = static_cast<uint128_t>(contributor.value);
			return (proj_name << 64) | acc_name;
		}
        EOSLIB_SERIALIZE(contribution, (contrib_id)(project_name)(contributor)(tier_name))
    };

    typedef multi_index<name("accounts"), account> accounts;

    typedef multi_index<name("projects"), project,
        indexed_by<name("bycategory"), const_mem_fun<project, uint64_t, &project::by_cat>>
        > projects;

    typedef multi_index<name("contribs"), contribution,
        indexed_by<name("byproject"), const_mem_fun<contribution, uint64_t, &contribution::by_project>>,
        indexed_by<name("byaccount"), const_mem_fun<contribution, uint64_t, &contribution::by_account>>,
        indexed_by<name("bycontrib"), const_mem_fun<contribution, uint128_t, &contribution::by_contrib>>
        > contributions;


    ACTION newaccount(name new_account_name);

    ACTION contribute(name project_name, name tier_name, name contributor, string memo);

    ACTION uncontribute(name project_name, name contributor, name tier_name);

    ACTION donate(name project_name, name donor, asset amount, string memo);

    ACTION rmvaccount(name account_name);

    ACTION withdraw(name account_name, asset amount);

    ACTION newproject(name project_name, name category, name creator,
        string title, string description, string info_link, asset requested);

    ACTION addtier(name project_name, name creator, 
        name tier_name, asset price, string description, uint16_t contributions);

    ACTION updateinfo(name project_name, name creator,
        string title, string description, string info_link, asset requested);

    ACTION readyproject(name project_name, name creator, uint8_t length_in_days);

    ACTION closeproject(name project_name, name creator);

    ACTION rmvproject(name project_name, name creator);


    //Functions
    bool is_valid_category(name category);
    bool is_tier_in_project(name tier_name, vector<tier> tiers);
    int get_tier_idx(name tier_name, vector<tier> tiers);


    //Reactions
    void catch_transfer(name from, asset amount);

};