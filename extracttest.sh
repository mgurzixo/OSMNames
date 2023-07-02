#!/bin/sh
export PBF_FILE=${1}.osm.pbf
# export SKIP_VACUUM=True
echo TEST_FILE: ${PBF_FILE}
time docker-compose run --rm osmnames
