
for i in $(cat $1); do
	ufw deny from $i
done