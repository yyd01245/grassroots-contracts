/**
 * 
 * 
 * @author Craig Branscom
 * @copyright defined in LICENSE.txt
 */

#include <grassroots.hpp>

// grassroots::grassroots(name self, name code, datastream<const char*> ds) {
// }

// grassroots::~grassroots() {
// }

void grassroots::deposit() {

}

void grassroots::withdraw() {

}

void grassroots::catch_transfer(name from, asset amount) {

}

extern "C" {
    void apply(uint64_t self, uint64_t code, uint64_t action) {

        size_t size = action_data_size();
        constexpr size_t max_stack_buffer_size = 512;
        void* buffer = nullptr;
        if( size > 0 ) {
            buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);
            read_action_data(buffer, size);
        }
        datastream<const char*> ds((char*)buffer, size);

        if (code == self && action == name("deposit").value) {
            execute_action(name(self), name(code), &grassroots::deposit);
        } else if (code == self && action == name("withdraw").value) {
            execute_action(name(self), name(code), &grassroots::withdraw);
        } else if (code == name("eosio.token").value && action == name("transfer").value) {

            struct transfer_args {
                name from;
                name to;
                asset quantity;
                string memo;
            };
            
            grassroots grassroots(name(self), name(code), ds);
            auto args = unpack_action_data<transfer_args>();
            //eosio_assert(args.to == name("gograssroots"), "");
            grassroots.catch_transfer(args.from, args.quantity);
        }
    } //end apply
}; //end dispatcher
