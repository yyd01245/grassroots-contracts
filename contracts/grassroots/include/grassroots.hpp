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
        name creator;

        asset requested;
        vector<asset> milestones;
        asset received;

        uint32_t donations;
        uint32_t workers;
        uint32_t preorders;

        uint32_t begin_time;
        uint32_t end_time;

        uint8_t status;

        //TODO: categories vector? use tags?
        //TODO: 

        uint64_t primary_key() const { return project_name.value; }
        //TODO: make by_creator() index?
        EOSLIB_SERIALIZE(project, 
            (project_name)(creator)
            (begin_time)(end_time)
            (status))
    };

    typedef multi_index<name("projects"), project> projects_table;

    //@scope account_name.value
    //@ram 
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return account_name.value; }
        EOSLIB_SERIALIZE(account, (balance))
    };

    typedef multi_index<name("accounts"), account> accounts_table;

    //@scope get_self().value //TODO: symbol.code.raw?
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
    TABLE skill {
        name skill_name;

        uint64_t primary_key() const { return skill_name.value; } 
    };

    typedef multi_index<name("skills"), skill> skills_table;

    //@scope get_self().value
    //@ram 
    TABLE worker {
        name account_name;

        //TODO: skills vector?
        //TODO: endorsements?

        uint64_t primary_key() const { return account_name.value; }
    };

    typedef multi_index<name("workers"), worker> workers_table;

    //@scope get_self().value
    //@ram 

    //======================== project actions ========================

    //create a new project
    ACTION newproject(name project_name, name category, name creator);

    //add a milestone to a project
    ACTION addmilestone();

    //remove a milestone from a project
    ACTION rmvmilestone();

    //adds tags to a project if not already tagged (demux updater)
    ACTION addtags();

    //removes tags from a project if already tagged (demux updater)
    ACTION removetags();

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

    //donates the specified amount to a project
    //is reclaimable if the project fails to be funded
    ACTION donate(name project_name, name donor, asset amount, string memo);

    //reclaims an entire donation from a project
    ACTION undonate(name project_name, name donor);



    //======================== worker actions ========================

    //register a new worker profile
    ACTION registerwrkr(name account_name);

    //adds a skill to a worker profile
    ACTION addskills(name account_name, vector<name> skills);

    //removes skills from a worker profile
    ACTION removeskills(name account_name, vector<name> skills);

    //endorse another worker's skill
    ACTION endorseskill(name endorser, name worker, name skill);

    //apply for a job on a project
    ACTION applyforjob();




    //======================== admin actions ========================

    //adds a new category to the platform
    ACTION addcategory(name new_category);

    //removes a category from the platform
    ACTION rmvcategory(name category);

    //TODO: add and remove new skills



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



};