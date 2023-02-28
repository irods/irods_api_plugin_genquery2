#ifndef IRODS_API_PLUGIN_GENQUERY2_COMMON_H
#define IRODS_API_PLUGIN_GENQUERY2_COMMON_H

// The API plugin number that identifies the API endpoint implemented in this project template.
// User-defined API plugin numbers start at 1,000,000.
static const int IRODS_APN_GENQUERY2 = 1'000'001;

typedef struct genquery2_input
{
    char* query_string;
    int length;
} genquery2_input_t;

typedef struct genquery2_output
{
    char* query_string;
    int length;
} genquery2_output_t;

#endif // IRODS_API_PLUGIN_GENQUERY2_COMMON_H
