#include "pg_query_parse_fbe.hpp"

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include <arena/arena.hpp>

#include "pg_query_ptr.h"

extern "C"
{
#include "pg_query.h"
#include "pg_query_internal.h"
#include "pg_query_outfuncs.h"
#include "postgres/include/port.h"
#include "postgres/include/utils/palloc.h"
}
// NOLINTNEXTLINE
PgQueryFBEParseResult pg_query_parse_fbe(stdb::memory::Arena& arena, const char* input) {
    MemoryContext ctx = nullptr;
    PgQueryInternalParsetreeAndError parsetree_and_error;
    PgQueryFBEParseResult result = {};

    ctx = pg_query_enter_memory_context();

    parsetree_and_error = pg_query_raw_parse(input);

    // These are all malloc-ed and will survive exiting the memory context, the caller is responsible to free them now
    result.stderr_buffer = parsetree_and_error.stderr_buffer;
    result.error = parsetree_and_error.error;
    result.parse_tree = pg_query_nodes_to_fbe(arena, parsetree_and_error.tree);

    pg_query_exit_memory_context(ctx);

    return result;
}

// NOLINTNEXTLINE
void pg_query_free_fbe_parse_result(PgQueryFBEParseResult& result) {
    if (result.error != nullptr) {
        pg_query_free_error(result.error);
    }

    free(result.stderr_buffer);
}
