#ifndef IRODS_API_PLUGIN_GENQUERY2_COMMON_H
#define IRODS_API_PLUGIN_GENQUERY2_COMMON_H

// The API plugin number that identifies the API endpoint implemented in this project template.
// User-defined API plugin numbers start at 1,000,000.
static const int IRODS_APN_GENQUERY2 = 1'000'001;

typedef struct genquery2_input
{
    char* query_string;
    char* zone;
    int sql_only;
} genquery2_input_t;

#define GenQuery2_Input_PI "str *query_string; str *zone; int sql_only;"

#endif // IRODS_API_PLUGIN_GENQUERY2_COMMON_H
