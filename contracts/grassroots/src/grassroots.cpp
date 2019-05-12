/**
 * Grassroots is a crowdfunding and development platform for EOSIO software.
 * 
 * @author Craig Branscom
 * @contract Grassroots
 * @copyright defined in LICENSE.txt
 */

#include "../include/grassroots.hpp";

// grassroots::grassroots(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

// grassroots::~grassroots() {}

//======================== project actions ========================

ACTION grassroots::newproject(name project_name, name category, name creator, asset requested, string uri) {
    //authenticate
    require_auth(creator);

    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acct = accounts.get(requested.symbol.code().raw(), "account not found");

    //get projects
    projects_table projects(get_self(), get_self().value);
    auto proj = projects.find(project_name.value);

    //validate
    check(proj == projects.end(), "project name already taken");
    check(is_valid_category(category), "invalid category");
    check(requested.symbol == CORE_SYM, "can only request amounts in native currency");
    check(requested > asset(0, CORE_SYM), "must request positive amount");
    check(requested >= MIN_PROJECT_REQUESTED, "must request minimum amount");

    //emplace new project, ram paid by creator
    projects.emplace(creator, [&](auto& row) {
        row.project_name = project_name;
        row.category = category;
        row.creator = creator;
        row.requested = requested;
        row.received = asset(0, CORE_SYM);
        row.status = SETUP;
        row.donations = 0;
        row.funding_open = time_point_sec(0);
        row.funding_close = time_point_sec(0);
        row.uri = uri;
    });
}

ACTION grassroots::openfunding(name project_name, name creator, uint8_t length_in_days) {
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
    check(acc.balance >= LISTING_FEE, "insufficient balance to cover listing fee");

    //charge project fee
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= LISTING_FEE;
    });

    //modify project times and status
    projects.modify(proj, same_payer, [&](auto& row) {
        row.funding_open = time_point_sec(current_time_point());
        row.funding_close = time_point_sec(current_time_point()) + uint32_t(length_in_days * 86400);
        row.status = FUNDING;
    });
}

ACTION grassroots::delproject(name project_name, name creator) {
    
    //authenticate
    require_auth(creator);
    
    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //validate
    check(creator == proj.creator, "only project creator can delete");

    //erase project
    projects.erase(proj);
}



//======================== account actions ========================

ACTION grassroots::open(name account_name, symbol sym) {
    //authenticate
    require_auth(account_name);

    //find account
    accounts_table accounts(get_self(), account_name.value);
    auto acct = accounts.find(sym.code().raw());

    //validate
    check(acct == accounts.end(), "account is already open");
    
    //TODO: check symbol is allowed currency

    //emplace new account, ram paid by user
    accounts.emplace(account_name, [&](auto& row) {
        row.balance = asset(0, sym);
    });
}

ACTION grassroots::close(name account_name, symbol sym) {
    //authenticate
    require_auth(account_name);

    //get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(sym.code().raw(), "account not found");

    //validate
    // check(acct.balance == asset(0, sym), "balance must be empty to close");

    //erase account
    accounts.erase(acct);
}

ACTION grassroots::withdraw(name account_name, asset amount) {
    //authenticate
    require_auth(account_name);
    
    //get account
    accounts_table accounts(get_self(), get_self().value);
    auto& acct = accounts.get(account_name.value, "account not found");

    //validate
    check(acct.balance >= amount, "insufficient balance");
    check(amount > asset(0, CORE_SYM), "must withdraw a positive amount");

    //update balance
    accounts.modify(acct, same_payer, [&](auto& row) {
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



//======================== donation actions ========================

ACTION grassroots::donate(name project_name, name donor, asset amount, string memo) {
    //authenticate
    require_auth(donor);

    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts_table accounts(get_self(), donor.value);
    auto& acct = accounts.get(proj.requested.symbol.code().raw(), "account not found");

    //find donation
    donations_table donations(get_self(), get_self().value);
    auto by_donor = donations.get_index<name("bydonor")>();
    auto don = by_donor.find(project_name.value);

    //validate
    check(proj.funding_close > time_point_sec(current_time_point()), "project funding is over");
    check(amount > asset(0, CORE_SYM), "must donate a positive amount");
    check(acct.balance >= amount, "insufficient balance");

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
    accounts.modify(acct, same_payer, [&](auto& row) {
        row.balance -= amount;
    });

    //add donation to project, update status if changed
    projects.modify(proj, same_payer, [&](auto& row) {
        row.received += amount;
        row.donations += new_donors;
        row.status = new_status;
    });
}

ACTION grassroots::undonate(name project_name, name donor, string memo) {
    //authenticate
    require_auth(donor);

    //get project
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts_table accounts(get_self(), donor.value);
    auto& acct = accounts.get(proj.requested.symbol.code().raw(), "account not found");

    //find donation
    donations_table donations(get_self(), get_self().value);
    auto by_donor = donations.get_index<name("bydonor")>();
    auto& don = by_donor.get(project_name.value, "donation not found");

    //validate
    check(proj.status == FUNDING, "project has already been funded");

    //remove donation from project
    projects.modify(proj, same_payer, [&](auto& row) {
        row.received -= don.total;
        row.donations -= 1;
    });

    //debit donation amount back to account balance
    accounts.modify(acct, same_payer, [&](auto& row) {
        row.balance += don.total;
    });

    //delete donation
    donations.erase(don);
}



//======================== admin actions ========================

ACTION grassroots::addcategory(name new_category) {
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

ACTION grassroots::rmvcategory(name category) {
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

    return (cat != categories.end());
}



//========== reactions ==========

void grassroots::catch_transfer(name from, name to, asset quantity, string memo) {

    //TODO: parse string for donation info

    //get first receiver of action
    name rec = get_first_receiver();
    check(rec == name("eosio.token"), "not eosio.token transfer");

    //check for account
    accounts_table accounts(get_self(), to.value);
    auto acct = accounts.find(quantity.symbol.code().raw());

    //check if account has a balance
    if (acct != accounts.end()) {

        //update balance
        accounts.modify(acct, same_payer, [&](auto& row) {
            row.balance += quantity;
        });
    } else if (acct == accounts.end() && memo == "open account") {

        //emplace new account, ram paid by contract
        accounts.emplace(get_self(), [&](auto& row) {
            row.balance = quantity;
        });
    }
}



//========== migration actions ==========

ACTION grassroots::rmvaccount(name account_name) {
    accounts_table accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not found");
    accounts.erase(acc);
}

ACTION grassroots::rmvproject(name project_name) {
    projects_table projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");
    projects.erase(proj);
}

ACTION grassroots::rmvdonation(uint64_t donation_id) {
    donations_table donations(get_self(), get_self().value);
    auto& don = donations.get(donation_id, "donation not found");
    donations.erase(don);
}

// ACTION grassroots::addfeatured(name project_name, uint32_t added_seconds) {
//     featured_table featured(get_self(), get_self().value);
//     auto f_itr = featured.find(project_name.value);

//     if (f_itr == featured.end()) {
//         featured.emplace(get_self(), [&](auto& row) {
//             row.project_name = project_name;
//             row.featured_until = now() + added_seconds;
//         });
//     } else {
//         featured.modify(f_itr, same_payer, [&](auto& row) {
//             row.featured_until = now() + added_seconds;
//         });
//     }
// }



//========== pre/post dispatcher ==========

// extern "C"
// {
//     void apply(uint64_t receiver, uint64_t code, uint64_t action)
//     {
// 
//         if (code == receiver)
//         {
//             switch (action)
//             {
//                 EOSIO_DISPATCH_HELPER(grassroots, 
//                     (newproject)(updateproj)(openfunding)(cancelproj)(deleteproj) //project
//                     (registeracct)(withdraw)(deleteacct) //account
//                     (donate)(undonate) //donation
//                     (addcategory)(rmvcategory) //admin
//                     (rmvaccount)(rmvproject)(rmvdonation)(addfeatured)); //migration
//             }
//         }  else if (code == name("eosio.token").value && action == name("transfer").value) {
//             execute_action<grassroots>(eosio::name(receiver), eosio::name(code), &grassroots::catch_transfer);
//         }
//     }
// }

// extern "C" bool pre_dispatch(name self, name original_receiver, name action) {
//    print_f("pre_dispatch : % % %\n", self, original_receiver, action);
//    name nm;
//    read_action_data((char*)&nm, sizeof(nm));
//    if (nm == "quit"_n) {
//       return false;
//    }
//    return true;
// }

// extern "C" void post_dispatch(name self, name original_receiver, name action) {
//    print_f("post_dispatch : % % %\n", self, original_receiver, action);
//    std::set<name> valid_actions = {"test1"_n, "test2"_n, "test4"_n, "test5"_n};
//    check(valid_actions.count(action) == 0, "valid action should have dispatched");
//    check(self == "eosio"_n, "should only be eosio for action failures");
// }   
