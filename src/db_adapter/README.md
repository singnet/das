# Adapter

## Usage

```bash
make run-adapter OPTIONS="config.json"

# Generates metta files in the ./knowledege_base directory
make run-adapter OPTIONS="config.json true"
```

## Example of config.json

```json
See the config/example.json file in the root of this repository.
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