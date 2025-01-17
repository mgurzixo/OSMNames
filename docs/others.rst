======
Others
======

Performance
***********

The following tips can help to improve the performance for processing large PBF
files with OSMNames.


Database Configuration
----------------------

For better performance, the database needs to be configured according to the
resources of the host system, the process runs on. A custom configuration can be
added to ``data/postgres_config.sh`` which is mounted into the postgres
container through ``docker_compose.yml``. The script is executed when restarting
the database container.

Here is an example for the content of the script:

.. code-block:: bash

  #!/bin/bash
  set -o errexit
  set -o pipefail
  set -o nounset

  function alter_system() {
      echo "Altering System parameters"
      PGUSER="$POSTGRES_USER" psql --dbname="$POSTGRES_DB" <<-EOSQL
      alter system set max_connections = '40';
      alter system set shared_buffers = '16GB';
      alter system set effective_cache_size = '48GB';
      alter system set maintenance_work_mem = '2GB';
      alter system set checkpoint_completion_target = '0.9';
      alter system set wal_buffers = '16MB';
      alter system set default_statistics_target = '500';
      alter system set random_page_cost = '1.1';
      alter system set effective_io_concurrency = '200';
      alter system set work_mem = '13107kB';
      alter system set min_wal_size = '4GB';
      alter system set max_wal_size = '16GB';
      alter system set max_worker_processes = '32';
      alter system set max_parallel_workers_per_gather = '16';
      alter system set max_parallel_workers = '32';
      alter system set max_parallel_maintenance_workers = '4';

      alter system set autovacuum_work_mem = '4GB';
      alter system set checkpoint_timeout = '20min';
      alter system set datestyle = 'iso, mdy';
      alter system set default_text_search_config = 'pg_catalog.english';
      alter system set dynamic_shared_memory_type = 'posix';
      alter system set fsync = 'off';
      alter system set lc_messages = 'en_US.utf8';
      alter system set lc_monetary = 'en_US.utf8';
      alter system set lc_numeric = 'en_US.utf8';
      alter system set lc_time = 'en_US.utf8';
      alter system set listen_addresses = '*';
      alter system set log_checkpoints = 'on';
      alter system set log_temp_files = '1MB';
      alter system set log_timezone = 'UTC';
      alter system set synchronous_commit = 'off';
      alter system set temp_buffers = '120MB';
      alter system set timezone = 'UTC';
      alter system set track_counts = 'on';
      alter system set log_statement = 'all';
  EOSQL
  }

  alter_system


Determining the best configuration for a host is not easy. A good starting
point for that is `PgTune <https://pgtune.leopard.in.ua/>`_.

Parallelization is limited by CPU count and PostgreSQL's ``max_connections``
setting. As a rule of thumb ``max_connections = cpu_count * 3`` or greater is
required to (try to) utilize all CPUs.

tmpfs
-----

To improve the performance of OSMNames the database can be hold in the RAM
while processing. The easiest way to do this, is by adding following line to
the `docker-compose.yml` file:

.. code-block:: yaml

    ...
    postgres:
      ...
      tmpfs: /var/lib/postgresql/data:size=300G

This only makes sense if the necessary amount of RAM is available. Additionally
keep in mind that the data will be lost when restarting the docker container.
