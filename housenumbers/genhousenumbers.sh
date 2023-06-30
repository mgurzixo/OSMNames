#!/bin/sh

# usage: genhousenumbers.sh outname countryname...

## country_geoname.tsv format:
#  1 name
#  2 alternative_names
#  3 osm_type
#  4 osm_id
#  5 class
#  6 type
#  7 lon
#  8 lat
#  9 place_rank
# 10 importance
# 11 street
# 12 city
# 13 county
# 14 state
# 15 country
# 16 country_code
# 17 display_name
# 18 west
# 19 south
# 20 east
# 21 north
# 22 wikidata
# 23 wikipedia
# 24 housenumbers

## country_housenumbers.tsv format
# 1 osm_id
# 2 street_id
# 3 street
# 4 housenumber
# 5 lon
# 6 lat

outname=$1
echo outname: ${outname}
shift 1
countries=$*
echo countries: $countries
echo Generating ${outname}_geo.tsv and ${outname}_hn.tsv ...
rm -f ${outname}_geo.tsv
rm -f ${outname}_hn.tsv
# remove first line (columns names)
for i in ${countries}; do
    echo Processing ../data/export/${i}_geonames.tsv
    tail -n +2 ../data/export/${i}_geonames.tsv >>${outname}_geo.tsv
    echo Processing ../data/export/${i}_housenumbers.tsv
    tail -n +2 ../data/export/${i}_housenumbers.tsv >>${outname}_hn.tsv
done
echo Sorting ${outname}_hn.tsv by street_id ...
sort -n -t "$(printf "\t")" -k 2,2 -k 4,4 <${outname}_hn.tsv >${outname}_hn1.tsv
echo Generating ${outname}Index and ${outname}Data
./gennumbers ${outname} <${outname}_hn1.tsv

echo Sorting ${outname}_geo.tsv by osm_id ...
sort -n -t "$(printf "\t")" -k 4,4 <${outname}_geo.tsv >${outname}_geo1.tsv
echo Generating $(outname)_search.tsv
./mergestreets ${outname}
echo done.
