# Testing Adaguc-server

Adaguc contains two types of tests:

- Tests specifically for adaguc-server
- Tests specifically for the Python wrapper/EDR functionality

During the tests, adaguc makes use of a PostgreSQL backend.


## Using postgres

To run the tests against Postgres, the following needs to happen:

- The adaguc XML configuration files used by tests all require a `<DataBase parameters="{ADAGUC_DB}"/>` section. This gets handled by `starttests_psql.sh`
- PostgreSQL must be reachable on port `5432`.

### Via Docker

To do this through Docker, follow these steps:
1. Edit `docker-compose.yml`
2. Comment out all servers that are not `adaguc-db`
3. Add the `ports` section to the `adaguc-db` service to expose port 5432:
```yaml
  adaguc-db:
    (...)
    ports:
      - 5432:5432
```
4. Start postgres while inside the `./Docker` directory:
```bash
docker compose up -d
```
5. Edit `./Dockerfile`, to enable the `runtests_psql.sh`:
```Dockerfile
RUN bash runtests_psql.sh
```
6. Trigger a new build:
```bash
docker build -t adaguc-server --progress plain  --add-host=host.docker.internal:host-gateway .
```

All tests should now run against postgres via the docker build.

### Without Docker

To run the tests outside of Docker, make sure Postgres is reachable on port 5432, and then execute the test script directly:
```bash
docker compose -f Docker/docker-compose-test.yml up -Vd
./runtests_psql.sh
```

> **⚠️ Note**: some tests might fail due to platform architecture differences between amd64 and arm64.