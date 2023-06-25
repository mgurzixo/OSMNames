import os

from osmnames.database.functions import exec_sql, exec_sql_from_file, vacuum_database
from osmnames.prepare_data.set_names import set_names
from osmnames.prepare_data.prepare_housenumbers import prepare_housenumbers
from osmnames.prepare_data.create_hierarchy import create_hierarchy
from osmnames import consistency_check
from osmnames.helpers import run_in_parallel
from osmnames import logger
import time

log = logger.setup(__name__)


def prepare_data():
    configure_for_preparation()
    set_country_codes()
    set_polygon_types()
    set_place_ranks()
    merge_linked_nodes()
    set_names()
    delete_unusable_entries()
    follow_wikipedia_redirects()
    create_hierarchy()
    merge_corresponding_linestrings()
    prepare_housenumbers()


def configure_for_preparation():
    drop_unused_indexes()
    create_custom_columns()
    set_tables_unlogged()
    create_additional_indexes()  # MG
    create_helper_functions()


def drop_unused_indexes():
    for index in ["osm_linestring_osm_id_idx", "osm_housenumber_osm_id_idx"]:
        exec_sql("DROP INDEX IF EXISTS {}".format(index))


def create_custom_columns():
    exec_sql_from_file("create_custom_columns.sql",
                       cwd=os.path.dirname(__file__))


def set_tables_unlogged():
    exec_sql_from_file("set_tables_unlogged.sql",
                       cwd=os.path.dirname(__file__), parallelize=True)


def create_additional_indexes():
    # exec_sql_from_file("create_additional_indexes.sql",
    #                    cwd=os.path.dirname(__file__))
    # run_in_parallel(
    #     create_idx_osm_housenumber_normalized_street,
    #     create_idx_osm_housenumber_street_id,
    #     # create_idx_osm_relation_member_role
    # )
    log.info("start create_additional_indexes()")
    exec_sql("CREATE INDEX IF NOT EXISTS idx_osm_housenumber_normalized_street ON osm_housenumber USING hash(normalized_street);")
    log.info("DONE create_idx_osm_housenumber_normalized_street")
    exec_sql(
        "CREATE INDEX IF NOT EXISTS idx_osm_housenumber_street_id ON osm_housenumber USING hash(street_id);")
    log.info("DONE create_idx_osm_housenumber_street_id")
    exec_sql(
        "CREATE INDEX IF NOT EXISTS idx_osm_relation_member_role ON osm_relation_member USING hash(role);")
    log.info("DONE create_idx_osm_relation_member_role")


# def create_idx_osm_housenumber_normalized_street():
#     log.info("start create_idx_osm_housenumber_normalized_street")
#     start = time.time()
#     exec_sql("CREATE INDEX IF NOT EXISTS idx_osm_housenumber_normalized_street ON osm_housenumber USING hash(normalized_street);")
#     end = time.time()
#     log.info("finished create_idx_osm_housenumber_normalized_street (took {}s)".round(
#         end-start, 1))


# def create_idx_osm_housenumber_street_id():
#     log.info("start create_idx_osm_housenumber_street_id")
#     start = time.time()
#     exec_sql(
#         "CREATE INDEX IF NOT EXISTS idx_osm_housenumber_street_id ON osm_housenumber USING hash(street_id);")
#     end = time.time()
#     log.info("finished create_idx_osm_housenumber_street_id (took {}s)".round(
#         end-start, 1))


# def create_idx_osm_relation_member_role():
#     log.info("start create_idx_osm_relation_member_role")
#     start = time.time()
#     exec_sql(
#         "CREATE INDEX IF NOT EXISTS idx_osm_relation_member_role ON osm_relation_member USING hash(role);")
#     end = time.time()
#     log.info("finished create_idx_osm_relation_member_role (took {}s)".round(
#         end-start, 1))


def create_helper_functions():
    exec_sql_from_file("create_helper_functions.sql",
                       cwd=os.path.dirname(__file__), parallelize=True)


def merge_linked_nodes():
    exec_sql_from_file(
        "merge_linked_nodes/merge_nodes_linked_by_relation.sql", cwd=os.path.dirname(__file__))
    vacuum_database()


def delete_unusable_entries():
    exec_sql_from_file("delete_unusable_entries.sql",
                       cwd=os.path.dirname(__file__), parallelize=True)
    vacuum_database()


def set_place_ranks():
    exec_sql_from_file("set_place_ranks.sql",
                       cwd=os.path.dirname(__file__), parallelize=True)
    vacuum_database()


def set_country_codes():
    exec_sql_from_file("set_country_codes.sql", cwd=os.path.dirname(__file__))
    vacuum_database()
    consistency_check.missing_country_codes()


def set_polygon_types():
    exec_sql_from_file("set_polygon_types.sql", cwd=os.path.dirname(__file__))
    vacuum_database()


def merge_corresponding_linestrings():
    exec_sql_from_file("merge_corresponding_linestrings.sql",
                       cwd=os.path.dirname(__file__), parallelize=True)
    vacuum_database()


def follow_wikipedia_redirects():
    exec_sql_from_file("follow_wikipedia_redirects.sql",
                       cwd=os.path.dirname(__file__))
    vacuum_database()
