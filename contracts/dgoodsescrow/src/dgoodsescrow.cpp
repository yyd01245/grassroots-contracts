/**
 * 
 * 
 * @author Craig Branscom
 * @contract dgoodsescrow
 * @copyright defined in LICENSE.txt
 */

#include "../include/dgoodsescrow.hpp"

dgoodsescrow::dgoodsescrow(name self, name code, datastream<const char*> ds) : 
    contract(self, code, ds), 
    _symbolinfo(get_self(), get_self().value) {}

dgoodsescrow::~dgoodsescrow() {}

