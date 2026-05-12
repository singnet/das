# Adapter

## Usage

```bash
make run-adapter OPTIONS="config/das.json"
```

## Example of config.json

```json
See the config/das.json file in the root of this repository.
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