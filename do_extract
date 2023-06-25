#!/bin/sh
for i in $(./comfile pbf_file_url); do
    export PBF_FILE=$i
    time docker-compose run --rm osmnames
done
