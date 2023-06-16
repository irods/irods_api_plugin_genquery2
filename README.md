# iRODS GenQuery2 Parser

An **experimental** re-implementation of the iRODS GenQuery parser.

This project exists as a means for allowing the iRODS community to test the implementation and provide feedback so that the iRODS Consortium can produce a GenQuery parser that is easy to understand, maintain, and enhance all while providing a syntax that mirrors standard SQL as much as possible.

Once stable, the code will be merged into the iRODS server making it available with future releases of iRODS.

**If you use this project, please consider providing feedback through issues and/or discussions. We want to hear from you :-)**

## Features

- Enforces the iRODS permission model
- Logical AND, OR, and NOT
- Grouping via parentheses
- SQL CAST
- SQL GROUP BY
- SQL aggregate functions (e.g. count, sum, avg, etc)
- Per-column sorting via ORDER BY [ASC|DESC]
- SQL FETCH FIRST N ROWS ONLY (LIMIT offered as an alias)
- Metadata queries involving different iRODS entities (i.e. data objects, collections, users, and resources)
- Operators: =, !=, <, <=, >, >=, LIKE, BETWEEN, IS [NOT] NULL
- SQL keywords are case-insensitive
- Federation is supported

## Limitations (for now)

- Groups are not fully supported
- Cannot resolve tickets to data objects and collections using a single query
- Integer values must be treated as strings, except when used for OFFSET, LIMIT, FETCH FIRST _N_ ROWS ONLY

## Building

This project provides an API Plugin, a Rule Engine Plugin, and an iCommand upon successfully compiling it.

This project only compiles against iRODS 4.3.0 and depends on the following:
- irods-dev(el)
- irods-runtime
- irods-externals-boost
- irods-externals-fmt
- irods-externals-json
- irods-externals-nanodbc
- irods-externals-spdlog
- openssl development libraries
- flex 2.6.1
- bison 3.0.4

You may need to install rpm-build for Almalinux 8.

The steps for building the package are:
```bash
mkdir build
cd build
cmake /path/to/git/repo
ninja package # Or, make -j package
```

If you're compiling this project against a later version of iRODS 4.3, then you'll need to disable 4.3.0 compatibility. To do that, run the following:
```bash
cmake -DIRODS_ENABLE_430_COMPATIBILITY=NO /path/to/git/repo
```

This project can also be compiled for iRODS 4.2.11 and iRODS 4.2.12. To do that, run the following:
```bash
cmake -DIRODS_ENABLE_42X_COMPATIBILITY=YES -DIRODS_ENABLE_430_COMPATIBILITY=NO /path/to/git/repo
```

So far, this implementation has only run on Ubuntu 20.04 and Almalinux 8.

## Usage

### API Plugin

The following interface is not stable and may change over time.

- API Number: 1000001
- Input:
    - `query_string`: A GenQuery2 string
    - `zone`: The name of the zone to execute the query in. If null, the query is executed in the local zone.
    - `sql_only`: An integer which instructs the API plugin to return SQL without executing it
- Output: A JSON string (i.e. an array of array of strings)

### Microservices

In order to use the microservices, you'll need to enable the Rule Engine Plugin.

In your server_config.json file, insert the following inside of the rule_engines stanza:
```json
{
    "instance_name": "irods_rule_engine-genquery2-instance",
    "plugin_name": "irods_rule_engine-genquery2",
    "plugin_specific_configuration": {}
}
```

With that in place, you now have access to several microservices. Below is an example demonstrating their use via the iRODS Rule Language.

Let the following rule exist in `/etc/irods/core.re`.
```bash
genquery2_test_rule()
{
    # Execute a query. The results are stored in the Rule Engine Plugin.
    genquery2_execute(*handle, "select COLL_NAME, DATA_NAME order by DATA_NAME desc limit 1");

    # Iterate over the resutls.
    while (errorcode(genquery2_next_row(*handle)) == 0) {
        genquery2_column(*handle, '0', *coll_name); # Copy the COLL_NAME into *coll_name.
        genquery2_column(*handle, '1', *data_name); # Copy the DATA_NAME into *data_name.
        writeLine("stdout", "logical path => [*coll_name/*data_name]");
    }

    # Free any resources used. This is handled for you when the agent is shut down as well.
    genquery2_destroy(*handle);
}
```

We can run the rule using the following:
```bash
irule -r irods_rule_engine_plugin-irods_rule_language-instance genquery2_test_rule '*handle=%*coll_name=%*data_name=' ruleExecOut
```

### iCommand

`iquery` is a very simple iCommand which allows users to run GenQuery2 queries from the command line.

This tool is not designed as a replacement for `iquest`. It was designed for testing so it lacks several features.

#### Usage

```
iquery <query_string>
```

The command returns a JSON string on success.

If there's an error, it is printed to the terminal as is. Error information is NOT guaranteed to return as a JSON string.

See the help text for additional options and information.
```bash
iquery -h
```

#### Examples

NOTE: You can pipe the results to `jq` (if installed) so that they are easy to read.

```bash
# List all data objects and collections the user has access to.
iquery "select COLL_NAME, DATA_NAME"

# List all replicas the user has access to.
iquery "select no distinct DATA_ID, COLL_NAME, DATA_NAME, DATA_REPL_NUM"

# List all data objects which exist in the following resource hierarchies.
iquery "select COLL_NAME, DATA_NAME where DATA_RESC_HIER in ('demoResc', 'pt;repl;ufs0', 'otherResc')"

# List all data objects along with the resource name which satisfy the mixed metadata query.
iquery "select COLL_NAME, DATA_NAME, RESC_NAME where META_COLL_ATTR_NAME = 'a1' and (META_DATA_ATTR_NAME = 'a2' or META_RESC_ATTR_VALUE not like 'v1%')"

# List all data objects and collections the user has access to in "otherZone".
iquery -z otherZone "select COLL_NAME, DATA_NAME"

# Show the SQL that would be executed. The "pg_format" SQL formatter is only used for demonstration purposes.
iquery --sql-only "select COLL_NAME, DATA_NAME where RESC_NAME = 'demoResc'" | pg_format -
SELECT DISTINCT
    t0.coll_name,
    t1.data_name
FROM
    R_COLL_MAIN t0
    INNER JOIN R_DATA_MAIN t1 ON t0.coll_id = t1.coll_id
    INNER JOIN R_RESC_MAIN t2 ON t1.resc_id = t2.resc_id
    INNER JOIN R_OBJT_ACCESS pdoa ON t1.data_id = pdoa.object_id
    INNER JOIN R_TOKN_MAIN pdt ON pdoa.access_type_id = pdt.token_id
    INNER JOIN R_USER_MAIN pdu ON pdoa.user_id = pdu.user_id
    INNER JOIN R_OBJT_ACCESS pcoa ON t0.coll_id = pcoa.object_id
    INNER JOIN R_TOKN_MAIN pct ON pcoa.access_type_id = pct.token_id
    INNER JOIN R_USER_MAIN pcu ON pcoa.user_id = pcu.user_id
WHERE
    t2.resc_name = ?
    AND pdu.user_name = ?
    AND pcu.user_name = ?
    AND pdoa.access_type_id >= 1050
    AND pcoa.access_type_id >= 1050 FETCH FIRST 16 ROWS ONLY
```

## Logging

You can instruct the parser to show additional information as it processes messages by adding the following line to the log_level stanza in server_config.json. For example:
```javascript
{
    "log_level": {
        // ... Other Log Categories ...

        "genquery2": "trace",

        // ... Other Log Categories ...
    }
}
```

## Available Columns

NOTE: GenQuery2 column names are subject to change as the implementation is improved.

| GenQuery2 Column | Database Table | Database Column |
|---|---|---|
| ZONE_ID | R_ZONE_MAIN | zone_id |
| ZONE_NAME | R_ZONE_MAIN | zone_name |
| ZONE_TYPE | R_ZONE_MAIN | zone_type_name |
| ZONE_CONNECTION | R_ZONE_MAIN | zone_conn_string |
| ZONE_COMMENT | R_ZONE_MAIN | r_comment |
| ZONE_CREATE_TIME | R_ZONE_MAIN | create_ts |
| ZONE_MODIFY_TIME | R_ZONE_MAIN | modify_ts |
| USER_ID | R_USER_MAIN | user_id |
| USER_NAME | R_USER_MAIN | user_name |
| USER_TYPE | R_USER_MAIN | user_type_name |
| USER_ZONE | R_USER_MAIN | zone_name |
| USER_INFO | R_USER_MAIN | user_info |
| USER_COMMENT | R_USER_MAIN | r_comment |
| USER_CREATE_TIME | R_USER_MAIN | create_ts |
| USER_MODIFY_TIME | R_USER_MAIN | modify_ts |
| USER_AUTH_ID | R_USER_AUTH | user_id |
| USER_DN | R_USER_AUTH | user_auth_name |
| USER_DN_INVALID | R_USER_MAIN | r_comment |
| RESC_ID | R_RESC_MAIN | resc_id |
| RESC_NAME | R_RESC_MAIN | resc_name |
| RESC_ZONE_NAME | R_RESC_MAIN | zone_name |
| RESC_TYPE_NAME | R_RESC_MAIN | resc_type_name |
| RESC_CLASS_NAME | R_RESC_MAIN | resc_class_name |
| RESC_HOSTNAME | R_RESC_MAIN | resc_net |
| RESC_VAULT_PATH | R_RESC_MAIN | resc_def_path  |
| RESC_FREE_SPACE | R_RESC_MAIN | free_space |
| RESC_FREE_SPACE_TIME | R_RESC_MAIN | free_space_ts |
| RESC_INFO | R_RESC_MAIN | resc_info |
| RESC_COMMENT | R_RESC_MAIN | r_comment |
| RESC_STATUS | R_RESC_MAIN | resc_status |
| RESC_CREATE_TIME | R_RESC_MAIN | create_ts |
| RESC_MODIFY_TIME | R_RESC_MAIN | modify_ts  |
| RESC_CHILDREN | R_RESC_MAIN | resc_children  |
| RESC_CONTEXT | R_RESC_MAIN | resc_context  |
| RESC_PARENT | R_RESC_MAIN | resc_parent  |
| RESC_PARENT_CONTEXT | R_RESC_MAIN | resc_parent_context |
| DATA_ID | R_DATA_MAIN | data_id |
| DATA_COLL_ID | R_DATA_MAIN | coll_id |
| DATA_NAME | R_DATA_MAIN | data_name |
| DATA_REPL_NUM | R_DATA_MAIN | data_repl_num |
| DATA_VERSION | R_DATA_MAIN | data_version |
| DATA_TYPE_NAME | R_DATA_MAIN | data_type_name |
| DATA_SIZE | R_DATA_MAIN | data_size |
| DATA_PATH | R_DATA_MAIN | data_path |
| DATA_REPL_STATUS | R_DATA_MAIN | data_is_dirty |
| DATA_STATUS | R_DATA_MAIN | data_status |
| DATA_CHECKSUM | R_DATA_MAIN | data_checksum |
| DATA_EXPIRY | R_DATA_MAIN | data_expiry_ts |
| DATA_MAP_ID | R_DATA_MAIN | data_map_id |
| DATA_COMMENTS | R_DATA_MAIN | r_comment |
| DATA_CREATE_TIME | R_DATA_MAIN | create_ts |
| DATA_MODIFY_TIME | R_DATA_MAIN | modify_ts |
| DATA_MODE | R_DATA_MAIN | data_mode |
| DATA_RESC_ID | R_DATA_MAIN | resc_id |
| DATA_RESC_HIER | N/A | hier _(derived)_ |
| COLL_ID | R_COLL_MAIN | coll_id |
| COLL_NAME | R_COLL_MAIN | coll_name |
| COLL_PARENT_NAME | R_COLL_MAIN | parent_coll_name |
| COLL_MAP_ID | R_COLL_MAIN | coll_map_id |
| COLL_INHERITANCE | R_COLL_MAIN | coll_inheritance |
| COLL_COMMENTS | R_COLL_MAIN | r_comment |
| COLL_CREATE_TIME | R_COLL_MAIN | create_ts |
| COLL_MODIFY_TIME | R_COLL_MAIN | modify_ts |
| COLL_TYPE | R_COLL_MAIN | coll_type |
| COLL_INFO1 | R_COLL_MAIN | coll_info1 |
| COLL_INFO2 | R_COLL_MAIN | coll_info2 |
| META_DATA_ATTR_NAME | R_META_MAIN | meta_attr_name |
| META_DATA_ATTR_VALUE | R_META_MAIN | meta_attr_value |
| META_DATA_ATTR_UNITS | R_META_MAIN | meta_attr_unit |
| META_DATA_ATTR_ID | R_META_MAIN | meta_id |
| META_DATA_CREATE_TIME | R_META_MAIN | create_ts |
| META_DATA_MODIFY_TIME | R_META_MAIN | modify_ts |
| META_COLL_ATTR_NAME | R_META_MAIN | meta_attr_name |
| META_COLL_ATTR_VALUE | R_META_MAIN | meta_attr_value |
| META_COLL_ATTR_UNITS | R_META_MAIN | meta_attr_unit |
| META_COLL_ATTR_ID | R_META_MAIN | meta_id |
| META_COLL_CREATE_TIME | R_META_MAIN | create_ts |
| META_COLL_MODIFY_TIME | R_META_MAIN | modify_ts |
| META_RESC_ATTR_NAME | R_META_MAIN | meta_attr_name |
| META_RESC_ATTR_VALUE | R_META_MAIN | meta_attr_value |
| META_RESC_ATTR_UNITS | R_META_MAIN | meta_attr_unit |
| META_RESC_ATTR_ID | R_META_MAIN | meta_id |
| META_RESC_CREATE_TIME | R_META_MAIN | create_ts |
| META_RESC_MODIFY_TIME | R_META_MAIN | modify_ts |
| META_USER_ATTR_NAME | R_META_MAIN | meta_attr_name |
| META_USER_ATTR_VALUE | R_META_MAIN | meta_attr_value |
| META_USER_ATTR_UNITS | R_META_MAIN | meta_attr_unit |
| META_USER_ATTR_ID | R_META_MAIN | meta_id |
| META_USER_CREATE_TIME | R_META_MAIN | create_ts |
| META_USER_MODIFY_TIME | R_META_MAIN | modify_ts |
| GROUP_ID | R_USER_GROUP | group_user_id |
| GROUP_MEMBER_ID | R_USER_GROUP | user_id |
| DELAY_RULE_ID | R_RULE_EXEC | rule_exec_id |
| DELAY_RULE_NAME | R_RULE_EXEC | rule_name |
| DELAY_RULE_REI_FILE_PATH | R_RULE_EXEC | rei_file_path |
| DELAY_RULE_USER_NAME | R_RULE_EXEC | user_name |
| DELAY_RULE_EXE_ADDRESS | R_RULE_EXEC | exe_address |
| DELAY_RULE_EXE_TIME | R_RULE_EXEC | exe_time |
| DELAY_RULE_EXE_FREQUENCY | R_RULE_EXEC | exe_frequency |
| DELAY_RULE_PRIORITY | R_RULE_EXEC | priority |
| DELAY_RULE_ESTIMATED_EXE_TIME | R_RULE_EXEC | estimated_exe_time |
| DELAY_RULE_NOTIFICATION_ADDR | R_RULE_EXEC | notification_addr |
| DELAY_RULE_LAST_EXE_TIME | R_RULE_EXEC | last_exe_time |
| DELAY_RULE_STATUS | R_RULE_EXEC | exe_status |
| DATA_ACCESS_PERM_ID | R_OBJT_ACCESS | access_type_id |
| DATA_ACCESS_PERM_NAME | R_TOKN_MAIN | token_name |
| DATA_ACCESS_USER_ID | R_OBJT_ACCESS | user_id |
| DATA_ACCESS_USER_NAME | R_USER_MAIN | user_name |
| COLL_ACCESS_PERM_ID | R_OBJT_ACCESS | access_type_id |
| COLL_ACCESS_PERM_NAME | R_TOKN_MAIN | token_name |
| COLL_ACCESS_USER_ID | R_OBJT_ACCESS | user_id |
| COLL_ACCESS_USER_NAME | R_USER_NAME | user_name |
| TICKET_ID | R_TICKET_MAIN | ticket_id |
| TICKET_STRING | R_TICKET_MAIN | ticket_string |
| TICKET_TYPE | R_TICKET_MAIN | ticket_type |
| TICKET_USER_ID | R_TICKET_MAIN | user_id |
| TICKET_OBJECT_ID | R_TICKET_MAIN | object_id |
| TICKET_OBJECT_TYPE | R_TICKET_MAIN | object_type |
| TICKET_USES_LIMIT | R_TICKET_MAIN | uses_limit |
| TICKET_USES_COUNT | R_TICKET_MAIN | uses_count |
| TICKET_WRITE_FILE_LIMIT | R_TICKET_MAIN | write_file_limit |
| TICKET_WRITE_FILE_COUNT | R_TICKET_MAIN | write_file_count |
| TICKET_WRITE_BYTE_LIMIT | R_TICKET_MAIN | write_byte_limit |
| TICKET_WRITE_BYTE_COUNT | R_TICKET_MAIN | write_byte_count |
| TICKET_EXPIRY_TIME | R_TICKET_MAIN | ticket_expiry_ts |
| TICKET_CREATE_TIME | R_TICKET_MAIN | create_time |
| TICKET_MODIFY_TIME | R_TICKET_MAIN | modify_time |
| TICKET_ALLOWED_HOST | R_TICKET_ALLOWED_HOSTS | host |
| TICKET_ALLOWED_HOST_TICKET_ID | R_TICKET_ALLOWED_HOSTS | ticket_id |
| TICKET_ALLOWED_USER_NAME | R_TICKET_ALLOWED_USERS | user_name |
| TICKET_ALLOWED_USER_TICKET_ID | R_TICKET_ALLOWED_USERS | ticket_id |
| TICKET_ALLOWED_GROUP_NAME | R_TICKET_ALLOWED_GROUPS | group_name |
| TICKET_ALLOWED_GROUP_TICKET_ID | R_TICKET_ALLOWED_GROUPS | ticket_id |

## Questions and Answers

### Can I use the GenQuery2 microservices with the Python Rule Engine Plugin (PREP)?

Yes. The important thing to remember is that the GenQuery2 microservices can update input arguments. This behavior is fully supported by the iRODS Rule Language, but not the PREP. Changes to input arguments are only visible via the object returned from the microservice.

Below is an example demonstrating how to use the GenQuery2 microservices with the PREP. Pay close attention to the objects returned from each microservice call.

```python
def list_all_data_objects(rule_args, callback, rei):
    # Notice the first input argument. In the iRODS Rule Language, the microservice
    # would write the handle to it. Here, it is only a placeholder.
    result = callback.genquery2_execute('', 'select COLL_NAME, DATA_NAME')

    # We can retrieve the handle by reading the "arguments" list like so. The
    # "arguments" list is zero-indexed and each index maps to an argument in the call
    # previously made.
    handle = result['arguments'][0]

    while True:
        try:
            callback.genquery2_next_row(handle)
        except:
            break

        result = callback.genquery2_column(handle, '0', '')
        coll_name = result['arguments'][2]

        result = callback.genquery2_column(handle, '1', '')
        data_name = result['arguments'][2]

        callback.writeLine('stdout', 'logical path => [{}/{}]'.format(coll_name, data_name))
                                                                                            
    callback.genquery2_destroy(handle)
```
