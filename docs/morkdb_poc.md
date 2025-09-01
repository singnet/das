# AtomDB with MORKDB only

## Setup

### 1 - Clone PoC branch:
```
git clone -b artur-morkdb-poc https://github.com/singnet/das.git
cd das

# Or (if das repo is alredy cloned)
git pull && git checkout artur-morkdb-poc
```

### 2 - Build
```
make build-all
```

### 3 - Start MORKDB server
```
make run-mork-server

# Importl (load) animals.metta to MORKDB
curl https://raw.githubusercontent.com/singnet/das-toolbox/refs/heads/master/das-cli/src/examples/data/animals.metta -o /tmp/animals.metta

make mork-loader FILE=/tmp/animals.metta

# Download small.metta from GDrive and load it too
make mork-loader FILE=/tmp/small.metta

# Loader logs must be something like:
>>>
Serving /tmp at http://172.18.0.1:9000/
172.18.0.7 - - [01/Sep/2025 16:11:01] "GET /file.metta HTTP/1.1" 200 -
Done!
Static server stop.
```

### 4 - Run AttentionBroker and QueryAgent:

Be sure to set the necessary ENV VARs in `.env`:
```
DAS_ATTENTION_BROKER_ADDRESS=0.0.0.0
DAS_ATTENTION_BROKER_PORT=37007
DAS_MONGODB_NAME=das
DAS_MONGODB_HOSTNAME=0.0.0.0
DAS_MONGODB_PORT=28000
DAS_REDIS_HOSTNAME=0.0.0.0
DAS_REDIS_PORT=29000
DAS_MONGODB_USERNAME=dbadmin
DAS_MONGODB_PASSWORD=dassecret
DAS_MORK_HOSTNAME=0.0.0.0
DAS_MORK_PORT=8000
```

Start AB:
```
make run-attention-broker
```

Start QA:
```
make run-query-agent
```

Be sure that QueryAgent is connected to the MORKDB server by checking its logs for:
```
2025-09-01 18:36:54 | [INFO] | Connected to MORK at 0.0.0.0:8000
```

### 5 - Use Query client

Use the `query` binary to send queries to the QA:
```
# Simple query on animals.metta
./bin/query 0.0.0.0:8080 0.0.0.0:35700 42000:43000 0 50 '(Similarity "human" $S)'

# Simple query on small.metta
./bin/query 0.0.0.0:8080 0.0.0.0:35700 42000:43000 0 50 '(EVALUATION (PREDICATE (public.cvterm.cv_id (CONCEPT (public.cv $CV)))) (CONCEPT (public.cvterm "88010")))'

# Complex query on small.metta
./bin/query 0.0.0.0:8080 0.0.0.0:35700 42000:43000 0 1000 '(EVALUATION (PREDICATE (public.cvterm.cv_id (CONCEPT (public.cv $CV)))) (CONCEPT (public.cvterm $CV_TERM)))'
```
