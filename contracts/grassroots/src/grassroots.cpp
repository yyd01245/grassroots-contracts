/**
 * 
 * 
 * @author Craig Branscom
 * @contract grassroots
 * @copyright defined in LICENSE.txt
 */

#include <grassroots.hpp>

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
        row.unspent_balance = asset(0, TLOS_SYM);
        row.spent_balance = asset(0, TLOS_SYM);
    });
}

void grassroots::pledge(name project_name, name tier_name, name pledger) {
    //get project
    projects projects(get_self(), get_self().value);
    auto proj = projects.get(project_name.value, "project not found");

    //get account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.get(pledger.value, "account not found");

    //authenticate
    require_auth(pledger);
    check(acc.account_name == pledger, "can't pledge from someone else's account");

    //validate
    check(is_tier_in_project(tier_name, proj.tiers), "tier doesn't exist in project");

    uint128_t proj_name = static_cast<uint128_t>(project_name.value);
    uint128_t acc_name = static_cast<uint128_t>(pledger.value);
    uint128_t pledge_key = (proj_name << 64) | acc_name;
    
    //search for existing pledge
    pledges pledges(get_self(), get_self().value);
    auto by_pledges = pledges.get_index<name("bypledge")>();
    auto pl = by_pledges.find(pledge_key);

    if (pl == by_pledges.end()) { //new pledge

        //get tier index
        vector<tier> new_tiers_after_pledge = proj.tiers;
        int idx = get_tier_idx(tier_name, new_tiers_after_pledge);

        //validate
        check(idx != -1, "tier index not found");
        check(new_tiers_after_pledge[idx].pledges_left > 0, "no pledges left at this tier");
        check(acc.unspent_balance >= price, "insufficient unspent balance");
        
        asset price = new_tiers_after_pledge[idx].price;
        new_tiers_after_pledge[idx].pledges_left -= 1;

        //charge pledge price
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.unspent_balance -= price;
            row.spent_balance += price;
        });

        //update tier
        projects.modify(acc, same_payer, [&](auto& row) {
            row.tiers = new_tiers_after_pledge;
        });

        //emplace pledge, ram paid by user
        pledges.emplace(pledger, [&](auto& row) {
            row.pledge_id = pledges.available_primary_key();
            row.project_name = project_name;
            row.pledger = pledger;
            row.amount = price;
            row.timestamp = now();
        });

    } else { //pledge for project already exists
        //TODO: implement upgrading/downgrading pledge tier
        check(false, "pledge adjustment in developement...");
    }

}

void grassroots::unpledge(name project_name, asset amount) {
    //TODO: implement
}

void grassroots::withdraw(name account_name, asset amount) {
    //get account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.get(account_name.value, "account not found");

    //authenticate
    require_auth(account_name);
    check(acc.account_name == account_name, "cannot withdraw from another user's account");

    //validate
    check(acc.unspent_balance >= amount, "insufficient unspent balance");
    check(amount > asset(0, TLOS_SYM), "must withdraw a positive amount");

    //update balances
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.unspent_balance -= amount;
    });

    //TODO: inline transfer to account_name

}


void grassroots::newproject(name project_name, name category, name creator,
        string title, string description, string info_link, asset requested) {

    //get account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.get(creator.value, "account not found");

    //get projects
    projects projects(get_self(), get_self().value);
    auto proj = projects.find(project_name.value);

    //authenticate
    require_recipient(creator);
    check(acc.account_name == creator, "can't make a project with someone else's account");

    //validate
    check(proj == projects.end(), "project name already taken");
    check(is_valid_category(category), "invalid category");
    check(title != "", "title cannot be blank");
    check(description != "", "description cannot be blank");
    check(info_link != "", "info_link cannot be blank");
    check(requested > asset(0, TLOS_SYM), "must request positive amount of TLOS");

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
        row.last_edit = now();
    });

    //charge fee
    accounts.modify(acc, same_payer, [&](auto& row) {
        row.unspent_balance -= PROJECT_FEE;
    });
}

void grassroots::addtier(name project_name, name creator, 
    name tier_name, asset price, string description, uint8_t pledges) {
    //get project
    projects projects(get_self(), get_slef().value);
    auto proj = projects.get(project_name.value, "project not found");

    //authenticate
    require_auth(creator);
    check(proj.creator == creator, "cannot add tiers to another account's project");

    //validate
    check(price > asset(0, TLOS_SYM), "price must be greater than 0");
    check(description != "", "description cannot be blank");
    check(!is_tier_in_project(tier_name, proj.tiers), "tier with same name exists in project");

    //make new tier
    tier new_tier = {
        tier_name,
        price,
        description,
        0 //pledges_left
    }

    //emplace in order by price
    vector<tiers> new_tiers = emplace_tier_in_order(new_tier, proj.tiers);

    //update tiers
    projects.modify(proj, same_payer, [&](auto& row) {
        row.tiers = new_tiers;
        row.last_edit = now();
    });

}

void grassroots::updateinfo(name project_name, name creator,
        string title, string description, string info_link, asset requested) {
    //TODO: implement
}

void grassroots::readyproject(name project_name, name creator, uint8_t length_in_days) {
    //get project
    projects projects(get_self(), get_self().value);
    auto proj = projects.get(project_name.value, "project not found");

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
        row.last_edit = now();
    });
}

void grassroots::rmvproject(name project_name, name creator) {
    //TODO: implement
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

bool grassroots::is_tier_in_project(tier_name, vector<tier> tiers) {
    for (tier t : tiers) {
        if (t.tier_name == tier_name) {
            return true;
        }
    }

    return false;
}

vector<tier> grassroots::emplace_tier_in_order(tier new_tier, vector<tier> tiers) {
    //TODO: implement
    return tiers.emplace_back(new_tier);
}

int grassroots::get_tier_idx(name tier_name, vector<tier> tiers) {
    for (int i = 0; i < tiers.size(); i++) {
        if (tiers[i].tier_name == tier_name) {
            return i;
        }
    }

    return -1;
}

void grassroots::catch_transfer(name from, asset amount) {
    //check for account
    accounts accounts(get_self(), get_self().value);
    auto acc = accounts.find(from.value);

    if (acc == accounts.end()) { //no account found
        //emplace new account, ram paid by contract
        accounts.emplace(get_self(), [&](auto& row) {
            row.account_name = from;
            row.unspent_balance = amount;
            row.spent_balance = asset(0, TLOS_SYM);
        });
    } else { //account found
        //update unspent balance
        accounts.modify(acc, same_payer, [&](auto& row) {
            row.unspent_balace += amount;
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
                EOSIO_DISPATCH_HELPER(grassroots, (newaccount)(pledge)(unpledge)(withdraw)
                    (newproject)(addtier)(updateinfo)(readyproject)(rmvproject));
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
            grassroots.catch_transfer(args.from, args.quantity);
            //execute_action<grassroots>(eosio::name(receiver), eosio::name(code), &grassroots::catch_transfer(args.from, args.quantity));
        }
    }
}
