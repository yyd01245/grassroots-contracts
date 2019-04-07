/**
 * Grassroots is a crowdfunding development platform for EOSIO software.
 * 
 * @author Craig Branscom
 * @contract grassroots
 * @copyright defined in LICENSE.txt
 */

#include "../include/grassroots.hpp"

grassroots::grassroots(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

grassroots::~grassroots() {}

//======================== project actions ========================

void grassroots::newproject(name project_name, name category, name creator, 
    string title, string description, asset requested) {
    //authenticate
    require_auth(creator);

    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not registered");

    //get projects
    projects_table projects(get_self(), get_self().value);
    auto proj = projects.find(project_name.value);

    //validate
    check(proj == projects.end(), "project name already taken");
    check(is_valid_category(category), "invalid category");
    check(title != "", "title cannot be blank");
    check(description != "", "description cannot be blank");
    check(requested.symbol == CORE_SYM, "can only request amounts in native currency");
    check(requested > asset(0, CORE_SYM), "must request positive amount");

    //emplace new project, ram paid by creator
    projects.emplace(creator, [&](auto& row) {
        row.project_name = project_name;
        row.category = category;
        row.creator = creator;
        row.title = title;
        row.description = description;
        row.link = "";
        row.requested = requested;
        row.received = asset(0, CORE_SYM);
        row.donations = 0;
        row.preorders = 0;
        row.begin_time = 0;
        row.end_time = 0;
        row.status = SETUP;
    });
}

void grassroots::updateproj(name project_name, name creator,
        string new_title, string new_desc, string new_link, asset new_requested) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(proj.creator == creator, "cannot update another account's project");

    //validate
    check(new_title != "", "title cannot be blank");
    check(new_desc != "", "description cannot be blank");
    check(new_link != "", "link cannot be blank");
    check(new_requested >= asset(0, CORE_SYM), "must request a positive amount");
    // check(proj.project_status == SETUP, "cannot update project after funding has opened");

    //update project info
    projects.modify(proj, same_payer, [&](auto& row) {
        row.title = new_title;
        row.description = new_desc;
        row.link = new_link;
        row.requested = new_requested;
    });
}

void grassroots::openfunding(name project_name, name creator, uint8_t length_in_days) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not registered");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can open project for funding");

    //validate
    check(length_in_days >= 1 && length_in_days <= 180, "project length must be between 1 and 180 days");
    check(acc.balance >= PROJECT_FEE, "insufficient balance to cover project fee");

    //charge project fee
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= PROJECT_FEE;
    });

    //modify project times and status
    projects.modify(proj, same_payer, [&](auto& row) {
        row.begin_time = now();
        row.end_time = now() + uint32_t(length_in_days * 86400);
        row.status = FUNDING;
    });
}

void grassroots::cancelproj(name project_name, name creator) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can cancel the project");

    //validate
    check(proj.end_time > now(), "cannot cancel after project's end time");
    check(proj.status == FUNDING, "cannot cancel a project after it's been funded");

    //update project status to cancelled
    projects.modify(proj, same_payer, [&](auto& row) {
        row.status = CANCELLED;
    });
}

void grassroots::deleteproj(name project_name, name creator) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can delete the project");

    //validate
    check(proj.status == SETUP, "can only delete projects in SETUP");

    //delete project
    projects.erase(proj);
}

//======================== account actions ========================

void grassroots::registeracct(name account_name) {
    //authenticate
    require_auth(account_name);

    //check account doesn't already exist
    accounts_table accounts(get_self(), get_self().value);
    auto acc = accounts.find(account_name.value);

    //validate
    check(acc == accounts.end(), "account is already registered");

    //emplace new account, ram paid by user
    accounts.emplace(account_name, [&](auto& row) {
        row.account_name = account_name;
        row.balance = asset(0, CORE_SYM);
        row.rewards = asset(0, ROOTS_SYM);
    });
}

void grassroots::withdraw(name account_name, asset amount) {
    //authenticate
    require_auth(account_name);
    
    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not found");

    //validate
    check(acc.balance >= amount, "insufficient balance");
    check(amount > asset(0, CORE_SYM), "must withdraw a positive amount");

    //update balances
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= amount;
    });

    //transfer to eosio.token
    //inline trx requires gograssroots@active to have gograssroots@eosio.code
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		account_name, //to
		amount, //quantity
        std::string("grassroots withdrawal") //memo
	)).send();
}

void grassroots::deleteacct(name account_name) {
    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not registered");

    //authenticate
    require_auth(account_name);
    check(acc.account_name == account_name, "cannot delete someone else's account");

    //validate

    //have to save profile params for inline, can't read acc to fill params after erase
    auto to = acc.account_name;
    auto quantity = acc.balance;

    //forfeit rewards to @gograssroots
    accounts_table admin_acc(get_self(), get_self().value);
    auto& admin = admin_acc.get(ADMIN_NAME.value, "admin account not registered");

    //add deleted accounts rewards to @gograssroots
    admin_acc.modify(admin, same_payer, [&](auto& row) {
        row.rewards += acc.rewards;
    });

    //delete account
    accounts.erase(acc);

    //transfer remaining balance back to eosio.token
    //inline trx requires gograssroots@active to have gograssroots@eosio.code
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		to, //to
		quantity, //quantity
        std::string("balance from deleted account") //memo
	)).send();
}

//======================== donation actions ========================

void grassroots::donate(name project_name, name donor, asset amount, string memo) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(donor.value, "account not registered");

    //find donation
    donations_table donations(get_self(), get_self().value);
    auto by_donor = donations.get_index<name("bydonor")>();
    auto don = by_donor.find(project_name.value);

    //authenticate
    require_auth(donor);
    check(acc.account_name == donor, "cannot donate from someone else's account");

    //validate
    check(proj.end_time > now(), "project funding is over");
    check(amount > asset(0, CORE_SYM), "must donate a positive amount");
    check(acc.balance >= amount, "insufficient balance");

    uint8_t new_status = proj.status;
    uint32_t new_donors = 0;

    //update donations
    if (don == by_donor.end()) { //donation not found for project
        //increment project donors
        new_donors = 1;

        //emplace new donation
        donations.emplace(donor, [&](auto& row) {
            row.donation_id = donations.available_primary_key();
            row.donor = donor;
            row.project_name = project_name;
            row.total = amount;
        });
    } else { //previous donation to project exists
        //update donation total
        by_donor.modify(don, same_payer, [&](auto& row) {
            row.total += amount;
        });
    }

    //update project status if now fully funded
    if (proj.received + amount >= proj.requested) {
        new_status = FUNDED;
    }

    //subtract donation from balance
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= amount;
    });

    //add donation to project, update status if changed
    projects.modify(proj, same_payer, [&](auto& row) {
        row.received += amount;
        row.donations += new_donors;
        row.status = new_status;
    });
}

void grassroots::undonate(name project_name, name donor, string memo) {
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(donor.value, "account not registered");

    //find donation
    donations_table donations(get_self(), get_self().value);
    auto by_donor = donations.get_index<name("bydonor")>();
    auto& don = by_donor.get(project_name.value, "donation not found");

    //authenticate
    require_auth(donor);
    check(acc.account_name == donor, "cannot undonate another account's donation");

    //validate
    check(proj.status == FUNDING, "project has already been funded");

    //remove donation from project
    projects.modify(proj, same_payer, [&](auto& row) {
        row.received -= don.total;
        row.donations -= 1;
    });

    //debit donation amount back to account balance
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance += don.total;
    });

    //delete donation record
    donations.erase(don);
}

//======================== preorder actions ========================



//======================== admin actions ========================

void grassroots::addcategory(name new_category) {
    //authenticate
    require_auth(ADMIN_NAME);

    //get categories table
    categories_table categories(get_self(), get_self().value);
    auto cat = categories.find(new_category.value);

    //validate
    check(cat == categories.end(), "category name already exists");

    //emplace new category
    categories.emplace(ADMIN_NAME, [&](auto& row) {
        row.category_name = new_category;
    });
}

void grassroots::rmvcategory(name category) {
    //authenticate
    require_auth(ADMIN_NAME);

    //get categories table
    categories_table categories(get_self(), get_self().value);
    auto& cat = categories.get(category.value, "category not found");

    //remove category
    categories.erase(cat);
}

//========== functions ==========

bool grassroots::is_valid_category(name category) {

    /**
     * Categories:
     * 
     * apps, games, research, technology, environment,
     * audio, video, publishing, marketing, expansion
     */

    //get categories table
    categories_table categories(get_self(), get_self().value);
    auto cat = categories.find(category.value);

    return cat != categories.end();
}

//========== reactions ==========

void grassroots::catch_transfer(name from, name to, asset quantity, string memo) {
    //check for account
    accounts_table accounts(get_self(), get_self().value);
    auto acc = accounts.find(from.value);

    if (acc != accounts.end()) { //account is already registered
        //update balance
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.balance += quantity;
        });
    } else if (acc == accounts.end() && memo == "register account") { //register new account
        //check amount covers fee
        check(quantity >= RAM_FEE, "must transfer at least 0.1 TLOS to cover ram fee");

        //emplace new account, ram paid by contract
        accounts.emplace(get_self(), [&](auto& row) {
            row.account_name = from;
            row.balance = quantity - RAM_FEE;
            row.rewards = asset(0, ROOTS_SYM);
        });
    }
}

//========== migration actions ==========

void grassroots::rmvaccount(name account_name) {
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not found");
    accounts.erase(acc);
}

void grassroots::rmvproject(name project_name) {
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");
    projects.erase(proj);
}

void grassroots::rmvdonation(uint64_t donation_id) {
    donations_table donations(get_self(), get_self().value);
    auto& don = donations.get(donation_id, "donation not found");
    donations.erase(don);
}

void grassroots::addfeatured(name project_name, uint32_t added_seconds) {
    featured_table featured(get_self(), get_self().value);
    auto f_itr = featured.find(project_name.value);

    if (f_itr == featured.end()) {
        featured.emplace(get_self(), [&](auto& row) {
            row.project_name = project_name;
            row.featured_until = now() + added_seconds;
        });
    } else {
        featured.modify(f_itr, same_payer, [&](auto& row) {
            row.featured_until = now() + added_seconds;
        });
    }
}

//========== dispatcher ==========

extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        size_t size = action_data_size();
        constexpr size_t max_stack_buffer_size = 512;
        void* buffer = nullptr;
        if( size > 0 ) {
            buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
            read_action_data(buffer, size);
        }
        datastream<const char*> ds((char*)buffer, size);

        if (code == receiver)
        {
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(grassroots, 
                    (newproject)(updateproj)(openfunding)(cancelproj)(deleteproj) //project
                    (registeracct)(withdraw)(deleteacct) //account
                    (donate)(undonate) //donation
                    (addcategory)(rmvcategory) //admin
                    (rmvaccount)(rmvproject)(rmvdonation)(addfeatured)); //migration
            }

        }  else if (code == name("eosio.token").value && action == name("transfer").value) {
            execute_action<grassroots>(eosio::name(receiver), eosio::name(code), &grassroots::catch_transfer);
        }
    }
}
