EXT=enp3s0

for i in $(cat $1); do
	nft add rule ip filter INPUT iifname "$EXT" ip saddr $i/24 counter drop
done