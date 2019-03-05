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

    //get projects
    projects projects(get_self(), get_self().value);
    auto proj = projects.find(project_name.value);

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not registered");

    //validate
    check(proj == projects.end(), "project name already taken");
    check(is_valid_category(category), "invalid category");
    check(title != "", "title cannot be blank");
    check(description != "", "description cannot be blank");
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
        row.contributors = 0;
        row.donors = 0;
        row.begin_time = 0;
        row.end_time = 0;
        row.project_status = SETUP;
        row.last_edit = now();
    });
}

// void grassroots::editproject(name project_name, name creator,
//         string new_title, string new_desc, string new_link, asset new_requested) {
//     //get project
//     projects projects(get_self(), get_self().value);
//     auto& proj = projects.get(project_name.value, "project not found");

//     //authenticate
//     require_auth(creator);
//     check(proj.creator == creator, "cannot add tiers to another account's project");

//     //validate
//     check(new_title != "", "title cannot be blank");
//     check(new_desc != "", "description cannot be blank");
//     check(new_link != "", "info_link cannot be blank");
//     check(new_requested >= asset(0, TLOS_SYM), "must request a positive amount");
//     check(proj.begin_time == 0 && proj.end_time == 0 && proj.project_status == SETUP,
//          "cannot edit project after readying");

//     //update project info
//     projects.modify(proj, same_payer, [&](auto& row) {
//         row.title = new_title;
//         row.description = new_desc;
//         row.info_link = new_link;
//         row.requested = new_requested;
//         row.last_edit = now();
//     });
// }

void grassroots::openfunding(name project_name, name creator, uint8_t length_in_days) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not registered");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can open project for funding");

    //validate
    check(length_in_days >= 1 && length_in_days <= 180, "project length must be between 1 and 180 days");
    check(acc.balance >= PROJECT_FEE, "insufficient balance to cover project fee");

    //update project
    projects.modify(proj, same_payer, [&](auto& row) {
        row.begin_time = now();
        row.end_time = now() + uint32_t(length_in_days * 86400);
        row.project_status = OPEN;
        row.last_edit = now();
    });

    //charge project fee
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= PROJECT_FEE;
    });
}

// void grassroots::closefunding(name project_name, name creator) {
//     //get project
//     projects projects(get_self(), get_self().value);
//     auto& proj = projects.get(project_name.value, "project not found");

//     //get account
//     accounts accounts(get_self(), get_self().value);
//     auto& acc = accounts.get(creator.value, "account not found");

//     //authenticate
//     require_auth(creator);
//     check(creator == proj.creator, "only project creator can close the project");

//     //validate
//     check(now() > proj.end_time, "can't close project until past end time");

//     //determine new status
//     uint8_t new_status;
//     if (proj.received >= proj.requested) {
//         new_status = FUNDED;

//         //update account balance
//         accounts.modify(acc, same_payer, [&](auto& row) {
//             row.balance += proj.received;
//         });

//     } else {
//         new_status = FAILED;
//     }

//     //update project status
//     projects.modify(proj, same_payer, [&](auto& row) {
//         row.project_status = new_status;
//         row.last_edit = now();
//     });
// }

void grassroots::cancelproj(name project_name, name creator) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can cancel the project");
    check(proj.end_time > now(), "cannot cancel after project's end time");

    //update project status to cancelled
    projects.modify(proj, same_payer, [&](auto& row) {
        row.project_status = CANCELLED;
        row.last_edit = now();
    });
}

void grassroots::deleteproj(name project_name, name creator) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can delete the project");
    check(proj.project_status == SETUP, "can only delete projects in SETUP");

    //delete project
    projects.erase(proj);
}

//======================== account actions ========================

void grassroots::registeracct(name account_name) {
    //authenticate
    require_auth(account_name);

    //check account doesn't already exist
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.find(account_name.value);
    check(acc == accounts.end(), "account is already registered");

    //emplace new account, ram paid by user
    accounts.emplace(account_name, [&](auto& row) {
        row.account_name = account_name;
        row.balance = asset(0, CORE_SYM);
        row.dividends = asset(0, ROOT_SYM);
    });
}

void grassroots::donate(name project_name, name donor, asset amount, string memo) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(donor.value, "account not registered");

    //find donation
    donations donations(get_self(), project_name.value);
    auto don = donations.find(donor.value);

    //authenticate
    require_auth(donor);
    check(acc.account_name == donor, "cannot donate from someone else's account");

    //validate
    check(proj.project_status == OPEN, "project is not open for funding");
    check(proj.end_time > now(), "project funding is over");
    check(amount > asset(0, CORE_SYM), "must donate a positive amount");
    check(acc.balance >= amount, "insufficient balance");

    uint8_t new_status = proj.project_status;
    uint32_t new_donors = 0;

    //update donations
    if (don == donations.end()) { //donation not found for project
        //increment project donors
        new_donors = 1;

        //emplace new donation
        donations.emplace(donor, [&](auto& row) {
            row.donor = donor;
            row.total = amount;
        });
    } else { //previous donation to project exists
        //update donation total
        donations.modify(don, same_payer, [&](auto& row) {
            row.total += amount;
        });
    }

    //update status if project is now funded
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
        row.donors += new_donors;
        row.project_status = new_status;
    });
}

// void grassroots::preorder(name project_name, name contributor, asset amount, string memo) {
    //TODO: implement
// }

// void grassroots::refund(name project_name, name contributor, name tier_name) {
    //TODO: implement
// }

// void grassroots::redeem(name project_name, name contributor, name reward_name) {
    //TODO: implement
// }

void grassroots::withdraw(name account_name, asset amount) {
    //authenticate
    require_auth(account_name);
    
    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not registered");

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
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not registered");

    //authenticate
    require_auth(account_name);
    check(acc.account_name == account_name, "cannot withdraw from someone else's account");

    //have to save account params for inline, can't read acc to fill params after erase
    auto to = acc.account_name;
    auto quantity = acc.balance;

    //TODO: forfeit dividends to @gograssroots
    

    //delete account
    accounts.erase(acc);

    //transfer remaining balance back to eosio.token
    //inline trx requires gograssroots@active to have gograssroots@eosio.code
    // action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
	// 	get_self(), //from
	// 	to, //to
	// 	quantity, //quantity
    //     std::string("balance from cancelled account") //memo
	// )).send();
}

//========== functions ==========

//TODO: make this function more elegant
bool grassroots::is_valid_category(name category) {
    
    if (category == name("games") || 
        category == name("apps") || 
        category == name("research") || 
        category == name("technology") || 
        category == name("environment") ||
        category == name("audio") || 
        category == name("video") || 
        category == name("publishing")) {
        return true;
    }

    return false;
}

//========== reactions ==========

void grassroots::catch_transfer(name from, asset amount, string memo) {
    //check for account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.find(from.value);

    if (acc != accounts.end()) { //account is registered
        //update balance
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.balance += amount;
        });
    } else if (acc == accounts.end() && memo == "register account") { //register new account
        //check amount covers fee
        check(amount >= RAM_FEE, "must transfer at least 0.1 TLOS to cover ram fee");

        //emplace new account, ram paid by contract
        accounts.emplace(get_self(), [&](auto& row) {
            row.account_name = from;
            row.balance = amount - RAM_FEE;
            row.dividends = asset(0, ROOT_SYM);
        });
    }
}

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
                    (newproject)(openfunding)(cancelproj)(deleteproj)
                    (registeracct)(donate)(withdraw)(deleteacct));
            }

        } else if (code == name("eosio.token").value && action == name("transfer").value) {

            struct transfer_args {
                name from;
                name to;
                asset quantity;
                string memo;
            };
            
            grassroots grassroots(name(receiver), name(code), ds);
            auto args = unpack_action_data<transfer_args>();
            if (args.to == grassroots.ADMIN_NAME) {
                grassroots.catch_transfer(args.from, args.quantity, args.memo);
            }
        }
    }
}
