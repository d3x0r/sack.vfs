rule=$(nft -a list table filter |grep $1 | sed  's/.*handle \(\n*\)/\1/g')
[ ! -z $rule ] &&  nft delete rule filter INPUT handle $rule 
