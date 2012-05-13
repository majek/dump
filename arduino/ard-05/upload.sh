
tail log.csv |tr -s "\n"|tail -n 1|sed 's|^\(.*\),\(.*\),\(.*\),\(.*\)$|0, \1\n1, \2\n2, \3\n|' > data.csv

cat data.csv

KEY=
curl --request PUT \
     --data-binary @data.csv \
     --header "X-PachubeApiKey: $KEY" \
     http://api.pachube.com/v2/feeds/53618.csv


