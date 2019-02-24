/**
 * Grassroots is a crowd-funding development platform for EOSIO software.
 * 
 * @author Craig Branscom
 * @contract grassroots
 * @copyright defined in LICENSE.txt
 */

#include "../include/grassroots.hpp"

grassroots::grassroots(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

grassroots::~grassroots() {}

void grassroots::newaccount(name new_account_name) {
    //authenticate
    require_auth(new_account_name);

    //check account doesn't already exist
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.find(new_account_name.value);
    check(acc == accounts.end(), "account already exists");

    //emplace new account, ram paid by user
    accounts.emplace(new_account_name, [&](auto& row) {
        row.account_name = new_account_name;
        row.balance = asset(0, TLOS_SYM);
        row.dividends = asset(0, ROOT_SYM);
    });
}

void grassroots::contribute(name project_name, name tier_name, name contributor, string memo) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(contributor.value, "account not found");

    //authenticate
    require_auth(contributor);
    check(acc.account_name == contributor, "can't contribute from someone else's account");

    //validate
    check(is_tier_in_project(tier_name, proj.tiers), "tier doesn't exist in project");
    check(now() > proj.begin_time && now() < proj.end_time, "cannot contribute outside campaign window");

    uint128_t proj_name = static_cast<uint128_t>(project_name.value);
    uint128_t acc_name = static_cast<uint128_t>(contributor.value);
    uint128_t contrib_key = (proj_name << 64) | acc_name;
    
    //search for existing contribution
    contributions contributions(get_self(), get_self().value);
    auto by_contribs = contributions.get_index<name("bycontrib")>();
    auto contrib = by_contribs.find(contrib_key);

    if (contrib == by_contribs.end()) { //new contribution

        //get tier index
        vector<tier> new_tiers_after_contrib = proj.tiers;
        int idx = get_tier_idx(tier_name, new_tiers_after_contrib);
        asset price = new_tiers_after_contrib[idx].price;
        new_tiers_after_contrib[idx].remaining -= 1;

        //validate
        check(idx != -1, "tier index not found");
        check(new_tiers_after_contrib[idx].remaining > 0, "no contributions left at this tier");
        check(acc.balance >= price, "insufficient balance");

        //charge contribution price
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.balance -= price;
        });

        //update project
        projects.modify(proj, same_payer, [&](auto& row) {
            row.tiers = new_tiers_after_contrib;
            row.received += price;
        });

        //emplace contribution, ram paid by user
        contributions.emplace(contributor, [&](auto& row) {
            row.contrib_id = contributions.available_primary_key();
            row.project_name = project_name;
            row.contributor = contributor;
            row.tier_name = tier_name;
        });

    } else { //contribution for project already exists
        //TODO: implement upgrading/downgrading contribution tier
        check(false, "contribution adjustment in developement...");
    }

}

void grassroots::uncontribute(name project_name, name contributor, name tier_name) {
    //find project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value);

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(contributor.value, "account not found");

    uint128_t proj_name = static_cast<uint128_t>(project_name.value);
    uint128_t acc_name = static_cast<uint128_t>(contributor.value);
    uint128_t contrib_key = (proj_name << 64) | acc_name;
    
    //search for existing contribution
    contributions contributions(get_self(), get_self().value);
    auto by_contribs = contributions.get_index<name("bycontrib")>();
    auto contrib = by_contribs.find(contrib_key);
    
    //validate
    check(contrib != by_contribs.end(), "account hasn't contributed to this project");
    check(acc.account_name == contributor, "cannot remove someone else's contribution");
    check(now() > proj.begin_time && now() <= proj.end_time && proj.received < proj.requested, 
        "cannot uncontribute after project is funded");
    
    //get tier price
    vector<tier> new_tiers = proj.tiers;
    int idx = get_tier_idx(contrib->tier_name, new_tiers);
    check(idx != -1, "tier not found");
    
    //increment remaining contributions
    new_tiers[idx].remaining += 1;
    
    //update account
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance += new_tiers[idx].price;
    });

    //update project
    projects.modify(proj, same_payer, [&](auto& row) {
        row.tiers = new_tiers;
        row.received -= new_tiers[idx].price;
    });

    //remove contribution
    by_contribs.erase(contrib);
    
}

void grassroots::donate(name project_name, name donor, asset amount, string memo) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(donor.value, "account not found");

    //authenticate
    require_auth(donor);
    check(acc.account_name == donor, "cannot donate from someone else's account");

    //validate
    check(amount > asset(0, TLOS_SYM), "must donate a positive amount");
    check(acc.balance >= amount, "insufficient balance");

    //add donation to project
    projects.modify(proj, same_payer, [&](auto& row) {
        row.received += amount;
    });

    //subtract donation from balance
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= amount;
    });
}

void grassroots::rmvaccount(name account_name) {
    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not found");

    //authenticate
    require_auth(account_name);
    check(acc.account_name == account_name, "cannot withdraw from another user's account");
    check(acc.balance == asset(0, TLOS_SYM), "cannot remove acount with balance");
    check(acc.dividends == asset(0, ROOT_SYM), "cannot remove account with dividends");

    //remove account
    accounts.erase(acc);
}

void grassroots::withdraw(name account_name, asset amount) {
    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(account_name.value, "account not found");

    //authenticate
    require_auth(account_name);
    check(acc.account_name == account_name, "cannot withdraw from another user's account");

    //validate
    check(acc.balance >= amount, "insufficient balance");
    check(amount > asset(0, TLOS_SYM), "must withdraw a positive amount");

    //update balances
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= amount;
    });

    //inline trx requires grassrootsio@active to have grassrootsio@eosio.code
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
		get_self(), //from
		acc.account_name, //to
		amount, //quantity
        std::string("grassroots withdrawal") //memo
	)).send();

}

void grassroots::newproject(name project_name, name category, name creator,
        string title, string description, string info_link, asset requested) {

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not found");

    //get projects
    projects projects(get_self(), get_self().value);
    auto proj = projects.find(project_name.value);

    //authenticate
    require_auth(creator);
    check(acc.account_name == creator, "can't make a project with someone else's account");

    //validate
    check(proj == projects.end(), "project name already taken");
    check(is_valid_category(category), "invalid category");
    check(title != "", "title cannot be blank");
    check(description != "", "description cannot be blank");
    check(info_link != "", "info_link cannot be blank");
    check(requested > asset(0, TLOS_SYM), "must request positive amount of TLOS");
    check(acc.balance >= PROJECT_FEE, "insufficient balance to cover project fee");

    vector<tier> blank_tiers;

    //emplace new project, ram paid by user
    projects.emplace(creator, [&](auto& row) {
        row.project_name = project_name;
        row.category = category;
        row.creator = creator;
        row.title = title;
        row.description = description;
        row.info_link = info_link;
        row.tiers = blank_tiers;
        row.requested = requested;
        row.received = asset(0, TLOS_SYM);
        row.begin_time = 0;
        row.end_time = 0;
        row.project_status = SETUP;
        row.last_edit = now();
    });

    //charge fee
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.balance -= PROJECT_FEE;
    });
}

void grassroots::addtier(name project_name, name creator, 
    name tier_name, asset price, string description, uint16_t contributions) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(proj.creator == creator, "cannot add tiers to another account's project");

    //validate
    check(price > asset(0, TLOS_SYM), "price must be greater than 0");
    check(description != "", "description cannot be blank");
    check(!is_tier_in_project(tier_name, proj.tiers), "tier with same name exists in project");
    check(contributions > 0, "must have more than 0 contributions");
    check(proj.begin_time == 0 && proj.end_time == 0, "cannot edit project after readying");

    //make new tier
    tier new_tier = {
        tier_name, //tier_name
        price, //price
        description, //description
        contributions //remaining
    };

    vector<tier> new_tiers = proj.tiers;
    new_tiers.emplace_back(new_tier);

    //update tiers
    projects.modify(proj, same_payer, [&](auto& row) {
        row.tiers = new_tiers;
        row.last_edit = now();
    });

}

void grassroots::updateinfo(name project_name, name creator,
        string title, string description, string info_link, asset requested) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(proj.creator == creator, "cannot add tiers to another account's project");

    //validate
    check(title != "", "title cannot be blank");
    check(description != "", "description cannot be blank");
    check(info_link != "", "info_link cannot be blank");
    check(requested >= asset(0, TLOS_SYM), "must request a positive amount");
    check(proj.begin_time == 0 && proj.end_time == 0, "cannot edit project after readying");

    //update project info
    projects.modify(proj, same_payer, [&](auto& row) {
        row.title = title;
        row.description = description;
        row.info_link = info_link;
        row.requested = requested;
        row.last_edit = now();
    });
}

void grassroots::readyproject(name project_name, name creator, uint8_t length_in_days) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can ready the project");

    //validate
    check(length_in_days >= 1 && length_in_days <= 180, "project length must be between 1 and 180 days");
    check(proj.tiers.size() >= 1, "project must have at least 1 tier");

    //update project times
    projects.modify(proj, same_payer, [&](auto& row) {
        row.begin_time = now();
        row.end_time = now() + uint32_t(length_in_days * 86400);
        row.project_status = OPEN;
        row.last_edit = now();
    });
}

void grassroots::closeproject(name project_name, name creator) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto& acc = accounts.get(creator.value, "account not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can close the project");

    //validate
    check(now() > proj.end_time, "can't close project until past end time");

    //determine new status
    uint8_t new_status;
    if (proj.received >= proj.requested) {
        new_status = FUNDED;

        //update account balance
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.balance += proj.received;
        });

    } else {
        new_status = FAILED;
    }

    //update project status
    projects.modify(proj, same_payer, [&](auto& row) {
        row.project_status = new_status;
        row.last_edit = now();
    });
}

void grassroots::rmvproject(name project_name, name creator) {
    //get project
    projects projects(get_self(), get_self().value);
    auto& proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(creator == proj.creator, "only project creator can remove the project");
    //check(proj.project_status == SETUP, "cannot remove project after setup");

    //remove project
    projects.erase(proj);
}

//TODO: make this function more elegant
bool grassroots::is_valid_category(name category) {
    
    if (category == name("games") || 
        category == name("apps") || 
        category == name("research") || 
        category == name("tools") || 
        category == name("media") || 
        category == name("outreach") || 
        category == name("products")) {
        return true;
    }

    return false;
}

bool grassroots::is_tier_in_project(name tier_name, vector<tier> tiers) {
    for (tier t : tiers) {
        if (t.tier_name == tier_name) {
            return true;
        }
    }

    return false;
}

int grassroots::get_tier_idx(name tier_name, vector<tier> tiers) {
    for (int i = 0; i < tiers.size(); i++) {
        if (tiers[i].tier_name == tier_name) {
            return i;
        }
    }

    return -1;
}

//BUG: only catch transfers TO grassrootsio
void grassroots::catch_transfer(name from, asset amount) {
    //check for account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.find(from.value);

    if (acc == accounts.end()) { //no account found
        //emplace new account, ram paid by contract
        accounts.emplace(get_self(), [&](auto& row) {
            row.account_name = from;
            row.balance = amount;
            row.dividends = asset(0, ROOT_SYM);
        });
    } else { //account found
        //update balance
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.balance += amount;
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
                EOSIO_DISPATCH_HELPER(grassroots, (newaccount)(contribute)(uncontribute)(donate)(rmvaccount)(withdraw)
                    (newproject)(addtier)(updateinfo)(readyproject)(closeproject)(rmvproject));
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
                grassroots.catch_transfer(args.from, args.quantity);
            }
            //execute_action<grassroots>(eosio::name(receiver), eosio::name(code), &grassroots::catch_transfer(args.from, args.quantity));
        }
    }
}
