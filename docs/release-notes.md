# Release Notes

## DAS Version 0.1.1

* das-toolbox 0.2.21
* hyperon-das-atomdb 0.6.13
* hyperon-das 0.7.14
* das-serverless-functions 1.12.12

### Changelog

#### das-toolbox 0.2.21

```
[#47] Add support to ":" in symbol names
[#51] Add minor features to das-cli and update documentation
    das-cli --version
    das-cli update-version [--version] (defaulted to newest version)
    Rename das-cli server to das-cli db
    das-cli db restart
    das-cli faas restart
    Remove parameter --path in das-cli metta load and das-cli metta validate and get the input file as a required parameter.
    Rename das-cli metta validate to das-cli metta check.
    das-cli server start should sleep for a couple of seconds after finishing the startup of DB containers 
    Show progress bar printed by the metta parser binaries (db_loader and syntax_check) when executing das-cli metta load and das-cli metta validate
    Change message showing default version of the running function to show the actual version number.
    Add a das-cli logs das to follow the DAS log das.log
    Add das-cli jupyter-notebook start start a jupyter-notebook server running with all required dependencies to use hyperon-das.
    Adjust runtime messages for das-cli example local and das-cli example faas. Both show python commands
    das-cli faas update-version [--version]
    das-cli faas --version
    das-cli python-library version: show currently installed and newest available versions of both, hyperon-das and hyperon-das-atomdb
    das-cli python-library update. Update hyperon-das to the newest available version. As a consequence, hyperon-das-atomdb should be updated to the proper required version as well.
    das-cli python-library set --hyperon-das 0.4.0 --hyperon-das-atomdb 0.8.2 Allow setting specific versions for both libraries
    das-cli python-library list by default, list all major/minor versions of hyperon-das and hyperon-das-atomdb. There should have optional parameters --show-patches and --library <xxx>
    Add a new command to see release notes of specific version of specific package or lib. das-cli release-notes.
[#62] Remove example python files for local/remote DAS in das-cli examples
[#59] Fix das-cli --version output message
[#43] Improve DAS CLI Manual
[#69] Minor das-cli fixes
    Updated das-cli metta check/load output to only display db_load and syntax_check outputs.
    Modified das-cli faas update-version output to provide both old and new version information and advise callers to use das-cli faas restart to update a running faas server and also warns if no newer version is available.
    Applied changes to the das-cli update-version command output to provide both old and new version information and also warns if no newer version is available.
    Removed references to the example files distributed_atom_space_local.py and distributed_atom_space_remote.py, as well as the files themselves.
    Reviewed "Segmentation fault (core dumped)" error
    Added support for non-Ubuntu-based distributions to utilize the update-version command. Note that this command was developed and primarily tested on Ubuntu distributions.
[#73] das-cli python-library version is raising an error
[#76] Put version number in openfaas docker image name
[#75] Enable the configuration of a Redis cluster instead of maintaining only a standalone instance
```

#### hyperon-das-atomdb 0.6.13

```
[#127] Create bulk_insert() in Adapters
[#132] Fix bug in create_field_index() method
[das-query-engine#223] Update log messages
[#129] Create a new adapter called PostgresLobeDB
[das-query-engine#214] Add retrieve_all_atoms method
[#124] Changed count_atoms() to return more accurate numbers
[das-query-engine#197] Changed get_all_links() to return a tuple
[#142] Changed add_link() and add_node() to work with get_atom returns
[das-query-engine#114] Changed commit() to receive buffer as a kwargs parameter
[#63] Changed MongoFieldNames to FieldNames and placed it generally for all adapters
[das#45] Round 1 - Initial refactoring of RAM Only DAS
[#46] Add support for MongoDB indexes
[#153] Refactoring create index
```

#### hyperon-das 0.7.14

```
[#201] Implement fetch() in the DAS API
[#223] Updates in docstrings and logging messages
[#210] Improve message error when connecting to the server
[#214] Improve fetch() method to optionally fetch all the atoms
[#241] Fix tests
[#213] Add TraverseEngine to API documentation
[#114] Persist new atoms in remote server
[#216] Changed design for custom filters in TraverseEngine
[#229] Improve error handler
[#218] Make a DAS server read-only
[#223] Updated kwargs documentation
[#268] Adding performance tests in MeTTa
[#268] enhancing perf tests script, added results comparison
```

#### das-serverless-functions 1.12.12

```
[#100] Add fetch. New Action
[#113] Add create_context command in query-engine function
```
## DAS Version 0.1.0

* hyperon-das: 0.7.0
* hyperon-das-atomdb: 0.6.0
* FaaS functions: 1.12.0
* MeTTa Parser: 0.3.0
* Toolbox das-cli: 0.2.0

### Changelog

#### hyperon-das 0.7.0

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

#### hyperon-das-atomdb 0.6.0

```
[#112] Fix return of the functions get_matched_links(), get_incoming_links(), get_matched_type_template(), get_matched_type() from set to list
[#114] Add create_field_index() to RedisMongoDB adapter
[#120] Refactor Collections in RedisMongoDB adapter
[#118] Create a new set in Redis to save custom index filters
```

#### FaaS functions 1.12.0

```
[#57] Enable Running Integration Tests Locally with AtomDB Source Code and Local Query Engine
[#62] Create integration tests for get_incoming_links
[#64] Shutdown containers before run tests
[#90] OpenFaas is not serializing/deserializing query answers
[#97] Add custom_query() and update create_field_index()
```

#### MeTTa Parser 0.3.0

```
[#36] Add support for comments and escaped characters in literals and symbol names
[#39] Add support for empty expressions
[das-query-engine#192] Add support to complex type definitions in DAS MeTTa parser.
[das-atom-db#120] Refactor collection in MongoDB
[#43] Bugfix - BSON data being corrupted after implementation of complext typedef expressions
```

#### Toolbox das-cli 0.2.0

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
