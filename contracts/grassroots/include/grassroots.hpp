/**
 * Grassroots is a crowdfunding development platform for EOSIO software and projects.
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

CONTRACT Grassroots : public contract {

public:

    Grassroots(name self, name code, datastream<const char*> ds);

    ~Grassroots();

    // const symbol EOS_SYM = symbol("EOS", 4);
    // const symbol BOS_SYM = symbol("BOS", 4);
    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol CORE_SYM = TLOS_SYM; //TODO: get_core_sym()

    const name ADMIN_NAME = name("gograssroots");
    const symbol ROOTS_SYM = symbol("ROOTS", 0); //TODO: rename to DROPS?
    const asset LISTING_FEE = asset(250000, CORE_SYM); //25 TLOS
    const asset RAM_FEE = asset(1000, CORE_SYM); //0.1 TLOS
    const uint32_t DAY_IN_SECS = 86400;

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

        uint32_t begin_time;
        uint32_t end_time;
        uint8_t status;

        uint64_t primary_key() const { return project_name.value; }
        //TODO: make by_creator() index?
        EOSLIB_SERIALIZE(project, 
            (project_name)(category)(creator)
            (requested)(received)
            (begin_time)(end_time)(status))
    };

    typedef multi_index<name("projects"), project> projects_table;

    //@scope get_self().value
    //@ram 
    TABLE account {
        name account_name;
        asset balance;
        asset rewards;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(account, (account_name)(balance)(rewards))
    };

    typedef multi_index<name("accounts"), account> accounts_table;

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

        //TODO: project counts by category?

        uint64_t primary_key() const { return category_name.value; }
        EOSLIB_SERIALIZE(category, (category_name))
    };

    typedef multi_index<name("categories"), category> categories_table;

    //@scope get_self().value
    //@ram 
    TABLE featured {
        // uint64_t featured_id;
        name project_name;
        uint32_t featured_until;

        uint64_t primary_key() const { return project_name.value; }
        EOSLIB_SERIALIZE(featured, (project_name)(featured_until))
    };

    typedef multi_index<name("featured"), featured> featured_table;

    //======================== project actions ========================

    //create a new project
    ACTION newproject(name project_name, name category, name creator, 
        string title, string description, asset requested);

    //TODO: make optional params
    //update the content of the project
    ACTION updateproj(name project_name, name creator,
        string new_title, string new_desc, string new_link, asset new_requested);

    //opens the project up for funding for the specified number of days
    ACTION openfunding(name project_name, name creator, uint8_t length_in_days);

    //marks a project as cancelled and releases all funds received, if any
    ACTION cancelproj(name project_name, name creator);

    //deletes a project completely
    //can only be called before funding is open
    ACTION deleteproj(name project_name, name creator);

   //======================== account actions ========================

    //registers a new account in Grassroots
    ACTION registeracct(name account_name);

    //withdraws unspent grassroots balance back to eosio.token account
    ACTION withdraw(name account_name, asset amount);

    //deletes an account from the Grassroots platform
    //returns ram and balance back to user, forfeits rewards
    ACTION deleteacct(name account_name);

    //======================== donation actions ========================

    //donates a specific amount to a project without a preorder
    //is returned if the project fails to get funded
    ACTION donate(name project_name, name donor, asset amount, string memo);

    //reclaims an entire donation from a project
    ACTION undonate(name project_name, name donor, string memo);

    //======================== preorder actions ========================

    

    //======================== admin actions ========================

    //suspends an account from the platform
    // ACTION suspendacct(name account_to_suspend, string memo);

    //restores an account to good standing
    // ACTION restoreacct(name account_to_restore, string memo);

    //adds a new category to the platform
    ACTION addcategory(name new_category);

    //removes a category from the platform
    ACTION rmvcategory(name category);

    //========== functions ==========

    //returns true if parameter name is a valid category
    bool is_valid_category(name category);

    //========== reactions ==========

    //catches transfers sent to @gograssroots
    void catch_transfer(name from, name to, asset quantity, string memo);

    //========== migration actions ==========

    ACTION rmvaccount(name account_name);

    ACTION rmvproject(name project_name);

    ACTION rmvdonation(uint64_t donation_id);

    ACTION addfeatured(name project_name, uint32_t added_seconds);

};