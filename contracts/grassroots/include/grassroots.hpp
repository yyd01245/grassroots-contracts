/**
 * Grassroots is a crowdfunding development platform for EOSIO software.
 * 
 * @author Craig Branscom
 * @contract grassroots
 * @copyright defined in LICENSE.txt
 */

#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/ignore.hpp>

using namespace std;
using namespace eosio;

class [[eosio::contract("grassroots")]] grassroots : public contract {

public:

    grassroots(name self, name code, datastream<const char*> ds);

    ~grassroots();

    // const symbol EOS_SYM = symbol("EOS", 4);
    // const symbol BOS_SYM = symbol("BOS", 4);
    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol CORE_SYM = TLOS_SYM; //TODO: get_core_sym()

    const name ADMIN_NAME = name("gograssroots");
    const symbol ROOT_SYM = symbol("ROOT", 0);
    const asset PROJECT_FEE = asset(250000, CORE_SYM); //25 TLOS
    const asset RAM_FEE = asset(1000, CORE_SYM); //0.1 TLOS

    enum PROJECT_STATUS : uint8_t {
        SETUP, //0
        OPEN, //1
        FUNDED, //2
        FAILED, //3
        CANCELLED //4
    };

    //======================== tables ========================

    //@scope get_self().value
    //@ram 
    TABLE project {
        name project_name;
        name category;
        name creator;
        string title;
        string description;
        string link;
        asset requested;
        asset received;
        uint32_t contributors;
        uint32_t donors;
        uint32_t begin_time;
        uint32_t end_time;
        uint8_t project_status;
        uint32_t last_edit;

        uint64_t primary_key() const { return project_name.value; }
        uint64_t by_cat() const { return category.value; }
        uint64_t by_end_time() const { return static_cast<uint64_t>(end_time); }
        //TODO: make by_creator() index?
        EOSLIB_SERIALIZE(project, (project_name)(category)(creator)
            (title)(description)(link)(requested)(received)(contributors)(donors)
            (begin_time)(end_time)(project_status)(last_edit))
    };

    //TODO: creator stats? creator table?

    //@scope get_self().value
    //@ram 
    TABLE account { //rename to profile?
        name account_name;
        asset balance;
        asset dividends;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(account, (account_name)(balance)(dividends))
    };

    //@scope project_name.value
    //@ram 
    TABLE donation {
        name donor;
        asset total;

        uint64_t primary_key() const { return donor.value; }
        EOSLIB_SERIALIZE(donation, (donor)(total))
    };

    //@scope project_name.value
    //@ram
    // TABLE item {
    //     name item_name;
    //     string info;
    //     string meta;
    //     uint16_t supply;

    //     uint64_t primary_key() const { return item_name.value; }
    //     EOSLIB_SERIALIZE(item, (item_name)(info)(meta)(supply))
    // };

    //@scope project_name.value
    //@ram 
    // TABLE preorder {
    //     name contributor;
    //     name item_name;
    //     uint8_t quantity;
    //     asset total;

    //     uint64_t primary_key() const { return contributor.value; }
    //     uint64_t by_account() const { return contributor.value; }
    //     uint128_t by_order() const {
	// 		uint128_t acc_name = static_cast<uint128_t>(contributor.value);
    //         uint128_t rwd_name = static_cast<uint128_t>(reward_name.value);
	// 		return (acc_name << 64) | rwd_name;
	// 	}
    //     EOSLIB_SERIALIZE(contribution, (contrib_id)(project_name)
    //         (contributor)(reward_name)(total))
    // };

    typedef multi_index<name("accounts"), account> accounts;

    typedef multi_index<name("projects"), project,
        indexed_by<name("bycategory"), const_mem_fun<project, uint64_t, &project::by_cat>>,
        indexed_by<name("byendtime"), const_mem_fun<project, uint64_t, &project::by_end_time>>
    > projects;

    typedef multi_index<name("donations"), donation> donations;

    //items

    // typedef multi_index<name("preorders"), preorder,
    //     indexed_by<name("byproject"), const_mem_fun<contribution, uint64_t, &contribution::by_project>>,
    //     indexed_by<name("byaccount"), const_mem_fun<contribution, uint64_t, &contribution::by_account>>,
    //     indexed_by<name("byorder"), const_mem_fun<contribution, uint128_t, &contribution::by_order>>
    // > preorders;

    //======================== project actions ========================

    //create a new project
    ACTION newproject(name project_name, name category, name creator, 
        string title, string description, asset requested);

    //add a reward package to a project
    // ACTION addreward(name project_name, name creator, name reward_name, asset price,
    //      string info, string meta, int32_t stock);

    //remove a reward package from a project
    // ACTION rmvreward(name project_name, name creator, name reward_name);

    //TODO: make optional params
    //edit the content of the project
    // ACTION editproject(name project_name, name creator,
    //     string new_title, string new_desc, string new_link, asset new_requested);

    //opens the project up for funding for the specified number of days
    ACTION openfunding(name project_name, name creator, uint8_t length_in_days);

    //closes the project after the end time and renders a final project status
    // ACTION closefunding(name project_name, name creator);

    //marks a project as cancelled and releases all funds received, if any
    ACTION cancelproj(name project_name, name creator);

    //deletes a project completely
    //can only be called before funding is open
    ACTION deleteproj(name project_name, name creator);

   //======================== account actions ========================

    //registers an account in Grassroots
    ACTION registeracct(name account_name);

    //donates a specific amount to a project without a reward
    //is only be returned if the project fails to get funded ???
    ACTION donate(name project_name, name donor, asset amount, string memo);

    //reclaims an entire donation from a project
    // ACTION reclaim(name project_name, name donor);

    //withdraws unspent grassroots balance back to eosio.token account
    ACTION withdraw(name account_name, asset amount);

    //deletes an account from the Grassroots platform
    //returns ram and balance bacck to user, forfeits dividends
    ACTION deleteacct(name account_name);

    //======================== dgoods escrow actions ========================

    //contribute to a project by preordering the selected reward
    // ACTION preorder(name project_name, name contributor, asset amount, string memo);

    //redeems a dgood from a successfully funded project
    // ACTION redeem(name project_name, name contributor, name reward_name);

    //refund a preorder from the project back to the contributor
    // ACTION refund(name project_name, name contributor, name reward);

    //======================== admin actions ========================

    //suspends an account from the platform
    // ACTION suspendacct(name account_to_suspend);

    //adds a new category to the platform
    // ACTION addcategory(name new_category);

    //removes a category from the platform
    // ACTION rmvcategory(name category);

    //========== functions ==========

    //returns true if parameter name is a valid category
    bool is_valid_category(name category);

    //========== reactions ==========

    //catches transfers sent to @gograssroots
    void catch_transfer(name from, asset amount, string memo);

};