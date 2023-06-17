ALTER TABLE osm_linestring SET UNLOGGED; --&
ALTER TABLE osm_point SET UNLOGGED; --&
ALTER TABLE osm_polygon SET UNLOGGED; --&
ALTER TABLE osm_housenumber SET UNLOGGED; --&

-- MG
CREATE INDEX IF NOT EXISTS idx_osm_housenumber_normalized_street ON osm_housenumber USING hash(normalized_street);
CREATE INDEX IF NOT EXISTS idx_osm_housenumber_street_id ON osm_housenumber USING hash(street_id);