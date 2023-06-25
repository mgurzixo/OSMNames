#!/bin/sh
rm --f bid
for i in luxembourg_geonames.tsv paris_geonames.tsv belgium_geonames.tsv france_geonames.tsv planet-latest_geonames.tsv; do
    tail -n +2 $i >>bid
done
cp header.tsv data.tsv
sort -u -t "$(printf "\t")" -k 4,4 bid | sort -n -r -t "$(printf "\t")" -k 10,10 >>data.tsv
rm -f bid
cp data.tsv ../../../zksearch/data/input
# rm -fr ../../../OSMNames/zksearch/data/index
