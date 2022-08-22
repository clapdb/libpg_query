#pragma once

#include <fcntl.h>
#include <unistd.h>

#include "arena/arena.hpp"
#include "pg_query.h"
#include "pg_query_ptr.h"

// NOLINTNEXTLINE
struct PgQueryFBEParseResult
{
    pg_query::ParseResult parse_tree;
    char* stderr_buffer;
    PgQueryError* error;
};

pg_query::ParseResult pg_query_nodes_to_fbe(stdb::memory::Arena& arena, const void* obj);

// NOLINTNEXTLINE
PgQueryFBEParseResult pg_query_parse_fbe(stdb::memory::Arena& arena, const char* input);

// NOLINTNEXTLINE
void pg_query_free_fbe_parse_result(PgQueryFBEParseResult& result);
