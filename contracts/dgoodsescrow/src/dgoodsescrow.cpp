/**
 * 
 * 
 * @author Craig Branscom
 * @contract dgoodsescrow
 * @copyright defined in LICENSE.txt
 */

#include "../include/dgoodsescrow.hpp"

dgoodsescrow::dgoodsescrow(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

dgoodsescrow::~dgoodsescrow() {}

dgoodsescrow::create(name issuer, name category, name token_name, bool fungible, bool burnable,
    bool transferable, int64_t max_supply) {
    //
    
    if (!environment.exists()) {
        vector<uint64_t> new_totals = {0,0,0};

        env_struct = env{
            self, //publisher
            new_totals, //totals
            now(), //time_now
            0 //last_ballot_id
        };

        environment.set(env_struct, self);
    } else {
        env_struct = environment.get();
        env_struct.time_now = now();
    }

}