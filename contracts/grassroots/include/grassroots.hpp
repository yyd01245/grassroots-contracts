/**
 * Grassroots is a crowdfunding and development platform for EOSIO software.
 * 
 * @author Craig Branscom
 * @contract Grassroots
 * @copyright defined in LICENSE.txt
 */

#pragma once
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>
#include <eosio/ignore.hpp>

using namespace std;
using namespace eosio;

CONTRACT grassroots : public contract {
    public:

    grassroots(name self, name code, datastream<const char*> ds);

    ~grassroots();

    //admin accounts
    const name ADMIN_NAME = name("gograssroots");

    //symbols
    // const symbol EOS_SYM = symbol("EOS", 4);
    // const symbol BOS_SYM = symbol("BOS", 4);
    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol CORE_SYM = TLOS_SYM; //TODO: get_core_sym()
    const symbol ROOTS_SYM = symbol("ROOTS", 0); //TODO: rename?

    //fees
    const asset LISTING_FEE = asset(250000, CORE_SYM); //25 TLOS
    const asset RAM_FEE = asset(1000, CORE_SYM); //0.1 TLOS

    //thresholds
    const asset MIN_PROJECT_REQUESTED = asset(100000, CORE_SYM); //100 TLOS

    //units
    const uint32_t DAY_SEC = 86400;

    //project statuses
    enum PROJECT_STATUS : uint8_t {
        SETUP, //0
        FUNDING, //1
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
        asset requested;
        asset received;
        uint8_t status;
        uint32_t donations;
        time_point_sec begin_time; //rename to funding_open?
        time_point_sec end_time; //rename to funding_close?
        string uri;

        //TODO: allow multi asset contributions?
        //map<symbol, asset> requested;
        //map<symbol, asset> received;
        //double progress;

        uint64_t primary_key() const { return project_name.value; }
        //TODO: make by_creator() index?
        EOSLIB_SERIALIZE(project,
            (project_name)(category)(creator)
            (requested)(received)(status)(donations)
            (begin_time)(end_time)
            (uri))
    };

    typedef multi_index<name("projects"), project> projects_table;

    //@scope owner.value
    //@ram 
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance))
    };

    typedef multi_index<name("accounts"), account> accounts_table;

    //@scope get_self().value
    //@ram
    // TABLE worker {
    //     name account_name;
    //     //TODO: skills and endorsements?
    //     uint64_t primary_key() const { return account_name.value; }
    //     EOSLIB_SERIALIZE(worker, (account_name))
    // };
    // typedef multi_index<name("workers"), worker> workers_table;

    //@scope get_self().value
    //@ram 
    TABLE donation {
        uint64_t donation_id;
        name donor;
        name project_name;
        asset total;

        uint64_t primary_key() const { return donation_id; }
        uint64_t by_donor() const { return donor.value; }
        uint64_t by_project() const { return project_name.value; }
        EOSLIB_SERIALIZE(donation, (donation_id)(donor)(project_name)(total))
    };

    typedef multi_index<name("donations"), donation,
        indexed_by<name("bydonor"), const_mem_fun<donation, uint64_t, &donation::by_donor>>,
        indexed_by<name("byproject"), const_mem_fun<donation, uint64_t, &donation::by_project>>
    > donations_table;

    //@scope get_self().value
    //@ram
    TABLE category {
        name category_name;

        uint64_t primary_key() const { return category_name.value; }
        EOSLIB_SERIALIZE(category, (category_name))
    };

    typedef multi_index<name("categories"), category> categories_table;

    //@scope get_self().value
    //@ram 
    // TABLE featured {
    //     name project_name;
    //     time_point_sec featured_until;
    //     uint64_t primary_key() const { return project_name.value; }
    //     EOSLIB_SERIALIZE(featured, (project_name)(featured_until))
    // };
    // typedef multi_index<name("featured"), featured> featured_table;



    //======================== project actions ========================

    //create a new project
    ACTION newproject(name project_name, name category, name creator, asset requested, string uri);

    //opens a project for funding
    ACTION openfunding(name project_name, name creator, uint8_t length_in_days);

    //closes a project after funding
    ACTION closeproject(name project_name, name creator);

    //deletes a project if no funding has been received, otherwise cancels it
    ACTION delproject(name project_name, name creator);



   //======================== account actions ========================

    //opens a token account
    ACTION open(name account_name, symbol sym);

    //closes a token account
    ACTION close(name account_name, symbol sym);

    //withdraws balance to eosio.token account
    ACTION withdraw(name account_name, asset amount);



    //======================== donation actions ========================

    //donates a specific amount to a project without a preorder
    //is returned if the project fails to get funded
    ACTION donate(name project_name, name donor, asset amount, string memo);

    //reclaims an entire donation from a project
    ACTION undonate(name project_name, name donor, string memo);

    

    //======================== admin actions ========================

    //adds a new category to the platform
    ACTION addcategory(name new_category);

    //removes a category from the platform
    ACTION rmvcategory(name category);



    //========== functions ==========

    //returns true if parameter name is a valid category
    bool is_valid_category(name category);



    //========== reactions ==========

    //catches eosio.token transfers sent to @gograssroots
    void catch_transfer(name from, name to, asset quantity, string memo);



    //========== migration actions ==========

    ACTION rmvaccount(name account_name);

    ACTION rmvproject(name project_name);

    ACTION rmvdonation(uint64_t donation_id);

    // ACTION addfeatured(name project_name, uint32_t added_seconds);



};