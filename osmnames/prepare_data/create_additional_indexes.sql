-- MG
create function print(text) returns void as $$ begin raise notice '%',
$1;
end $$ language plpgsql;
select print('Starting');
CREATE INDEX IF NOT EXISTS idx_osm_housenumber_normalized_street ON osm_housenumber USING hash(normalized_street);
select print('DONE idx_osm_housenumber_normalized_street');
CREATE INDEX IF NOT EXISTS idx_osm_housenumber_street_id ON osm_housenumber USING hash(street_id);
select print('DONE idx_osm_housenumber_street_id');
CREATE INDEX IF NOT EXISTS idx_osm_relation_member_role ON osm_relation_member USING hash(role);
--&
select print('DONE idx_osm_relation_member_role');