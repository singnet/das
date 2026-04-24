# Adapter

## Usage

```bash
make run-adapter OPTIONS="config.json"

# Generates metta files in the ./knowledege_base directory
make run-adapter OPTIONS="config.json true"
```

## Example of config.json

```json
{
  "schema_version": "1.0",
  "atomdb": {
    "type": "morkdb",
    "mongodb": {
      "endpoint": "localhost:40021",
      "username": "admin",
      "password": "admin",
      "cluster": false,
      "cluster_secret_key": "None",
      "nodes": [
        {
          "context": "default",
          "ip": "localhost",
          "username": "username"
        }
      ]
    },
    "morkdb": {
      "endpoint": "localhost:40022"
    }
  },
  "adapter": {
    "type": "postgres",
    "host": "chado.flybase.org",
    "port": 5432,
    "username": "flybase",
    "password": "",
    "database": "flybase",
    "context_mapping": {
      "queries_sql": "./simple_test.sql"
    }
  }
}
```

## simple_test.sql

```sql
SELECT
f.feature_id as public_feature__feature_id,
f.name as public_feature__name,
f.uniquename as public_feature__uniquename
FROM
feature as f WHERE f.feature_id<=1000;
```