
if [ "$1" = "grassroots"]
    contract=grassroots
    account=grassrootsio
else
    echo "need contract"
    exit 0
fi

if [ "$2" = "production" ]
then
    url=http://api.tlos.goodblock.io
elif [ "$2" = "test" ]
then
    url=https://api-test.tlos.goodblock.io/
elif [ "$2" = "local" ]
then
    url=http://127.0.0.1:8888
else
    echo "need stage"
    exit 0
fi

#eos v1.7.0
cleos -u $url set contract $account ./ $contract.wasm $contract.abi -p $account
#test wallet: PW5JzkDgprrUzjATioEKUdnVH99byHnamU4igex2Pps6Agx62JYht
#Pub: EOS8XCqRAx44FAzFtMy688jDtrybGHuktnpm8kAHn1XPDYqHJHbZ2
#Priv: 
#testing: cleos -u https://api-test.tlos.goodblock.io/ set contract grassrootsio ./ grassroots.wasm grassroots.abi -p grassrootsio