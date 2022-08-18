#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "fbe/pg_query_ptr.h"
#include "pg_query.h"

// NOLINTNEXTLINE
struct PgQueryFBEParseResult
{
    pg_query::ParseResult parse_tree;
    char* stderr_buffer;
    PgQueryError* error;
};

pg_query::ParseResult pg_query_nodes_to_fbe(const void* obj);

// NOLINTNEXTLINE
PgQueryFBEParseResult pg_query_parse_fbe(const char* input);

// NOLINTNEXTLINE
void pg_query_free_fbe_parse_result(PgQueryFBEParseResult& result);
