# Release Notes

## DAS Version 0.1.0

* hyperon-query-engine: 0.7.0
* hyperon-das-atomdb: 0.6.0
* FaaS functions: 1.12.0
* MeTTa Parser: 0.3.0
* Toolbox: 0.2.0

### Changelog

#### hyperon-query-engine: 0.7.0

```
[#180] Fix in the test_metta_api.py integration test
[#136] Implement methods in the DAS API to create indexes in the database
[#BUGFIX] Fix Mock in unit tests
[#90] OpenFaas is not serializing/deserializing query answers
[#190] Implement custom_query() method in DAS API
[#184] Fix bug that prevented DAS from answering nested queries properly
[#202] Fix tests after adding complex typedef expressions
[BUGFIX] Fix tests to compare dicts using only commom keys
```

#### hyperon-das-atomdb: 0.6.0

```
[#112] Fix return of the functions get_matched_links(), get_incoming_links(), get_matched_type_template(), get_matched_type() from set to list
[#114] Add create_field_index() to RedisMongoDB adapter
[#120] Refactor Collections in RedisMongoDB adapter
[#118] Create a new set in Redis to save custom index filters
```

#### FaaS functions: 1.12.0

```
[#57] Enable Running Integration Tests Locally with AtomDB Source Code and Local Query Engine
[#62] Create integration tests for get_incoming_links
[#64] Shutdown containers before run tests
[#90] OpenFaas is not serializing/deserializing query answers
[#97] Add custom_query() and update create_field_index()
```

#### MeTTa Parser: 0.3.0

```
[#36] Add support for comments and escaped characters in literals and symbol names
[#39] Add support for empty expressions
[das-query-engine#192] Add support to complex type definitions in DAS MeTTa parser.
[das-atom-db#120] Refactor collection in MongoDB
[#43] Bugfix - BSON data being corrupted after implementation of complext typedef expressions
```

#### Toolbox: 0.2.0

```
[#44] das-cli faas start should have optional parameters
[#45] Fix DAS-CLI metta loader container 
[#41] Developing a DAS CLI Manual Using the 'man' Command
[#30] Publish debian package to a package manager
[#32] Optimizing user feedback and experience for scripts
[#33] Remove poc loader
[#28] Generate Debian package
[#27] When you type 'server stop,' only shut down the server
[das-metta-parser/#36] Add support for comments and escaped characters in literals and symbol names
Change MeTTa parser version to 0.2.4
```
