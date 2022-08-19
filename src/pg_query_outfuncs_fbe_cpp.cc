
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>

#include "arena/arena.hpp"
#include "fbe/pg_query_ptr.h"
#include "pg_query_parse_fbe.hpp"

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

extern "C"
{
#include <ctype.h>

#include "src/postgres/include/c.h"
#include "src/postgres/include/port.h"
#include "src/postgres/include/postgres.h"
}

extern "C"
{
#include <ctype.h>

#include "pg_query_outfuncs.h"
#include "src/postgres/include/access/relation.h"
#include "src/postgres/include/nodes/parsenodes.h"
#include "src/postgres/include/nodes/plannodes.h"
#include "src/postgres/include/nodes/value.h"
#include "src/postgres/include/utils/datum.h"
}

#define OUT_TYPE(typename, typename_c) pg_query::typename*

#define OUT_NODE(typename, typename_c, typename_underscore, typename_underscore_upcase, typename_cast, fldname) \
    {                                                                                                           \
        pg_query::typename* fldname = arena.Create<pg_query::typename>();                                       \
        out = fldname;                                                                                          \
        _out##typename_c(arena, fldname, (const typename_cast*)obj);                                            \
    }

#define WRITE_INT_FIELD(outname, outname_json, fldname) out->outname = node->fldname;
#define WRITE_UINT_FIELD(outname, outname_json, fldname) out->outname = node->fldname;
#define WRITE_LONG_FIELD(outname, outname_json, fldname) out->outname = node->fldname;
#define WRITE_FLOAT_FIELD(outname, outname_json, fldname) out->outname = node->fldname;
#define WRITE_BOOL_FIELD(outname, outname_json, fldname) out->outname = node->fldname;

#define WRITE_CHAR_FIELD(outname, outname_json, fldname) out->outname = node->fldname;

#define WRITE_STRING_FIELD(outname, outname_json, fldname) \
    if (node->fldname != NULL) {                           \
        out->outname = std::string(node->fldname);         \
    }

#define WRITE_ENUM_FIELD(typename, outname, outname_json, fldname) \
    out->outname = ((pg_query::typename)_enumToInt##typename(node->fldname));

#define WRITE_LIST_FIELD(outname, outname_json, fldname)      \
    if (node->fldname != NULL) {                              \
        const ListCell* lc;                                   \
        foreach (lc, node->fldname) {                         \
            out->outname.push_back(pg_query::Node());         \
            _outNode(arena, out->outname.back(), lfirst(lc)); \
        }                                                     \
    }

#define WRITE_BITMAPSET_FIELD(outname, outname_json, fldname)  // FIXME

#define WRITE_NODE_FIELD(outname, outname_json, fldname) \
    { _outNode(arena, out->outname, &node->fldname); }

#define WRITE_NODE_PTR_FIELD(outname, outname_json, fldname) \
    if (node->fldname != NULL) {                             \
        _outNode(arena, out->outname, node->fldname);        \
    }

#define WRITE_SPECIFIC_NODE_FIELD(typename, typename_underscore, outname, outname_json, fldname) \
    { _out##typename(arena, &out->outname, &node->fldname); }

#define WRITE_SPECIFIC_NODE_PTR_FIELD(typename, typename_underscore, outname, outname_json, fldname) \
    if (node->fldname != NULL) {                                                                     \
        out->outname = arena.Create<pg_query::typename>();                                           \
        _out##typename(arena, out->outname, node->fldname);                                          \
    }

static void _outNode(::stdb::memory::Arena& arena, pg_query::Node& out, const void* obj);

static void _outList(::stdb::memory::Arena& arena, pg_query::List* out_node, const List* node) {
    const ListCell* lc;

    foreach (lc, node) {
        out_node->items.emplace_back();
        _outNode(arena, out_node->items.back(), lfirst(lc));
    }
}

static void _outIntList(::stdb::memory::Arena& arena, pg_query::IntList* out_node, const List* node) {
    const ListCell* lc;

    foreach (lc, node) {
        out_node->items.emplace_back();
        _outNode(arena, out_node->items.back(), lfirst(lc));
    }
}

static void _outOidList(::stdb::memory::Arena& arena, pg_query::OidList* out_node, const List* node) {
    const ListCell* lc;

    foreach (lc, node) {
        out_node->items.emplace_back();
        _outNode(arena, out_node->items.back(), lfirst(lc));
    }
}

// TODO: Add Bitmapset

static void _outInteger(::stdb::memory::Arena& arena, pg_query::Integer* out_node, const Value* node) {
    out_node->ival = (node->val.ival);
}

static void _outFloat(::stdb::memory::Arena& arena, pg_query::Float* out_node, const Value* node) {
    out_node->str = (node->val.str);
}

static void _outString(::stdb::memory::Arena& arena, pg_query::String* out_node, const Value* node) {
    out_node->str = (node->val.str);
}

static void _outBitString(::stdb::memory::Arena& arena, pg_query::BitString* out_node, const Value* node) {
    out_node->str = (node->val.str);
}

static void _outNull(::stdb::memory::Arena& arena, pg_query::Null* out_node, const Value* node) {
    // Null has no fields
}

#include "pg_query_enum_defs.c"
#include "pg_query_outfuncs_defs.cc"

static void _outNode(::stdb::memory::Arena& arena, pg_query::Node& out, const void* obj) {
    if (obj == NULL) return;  // Keep out as NULL

    switch (nodeTag(obj)) {
#include "pg_query_outfuncs_conds.c"

        default:
            printf("could not dump unrecognized node type: %d", (int)nodeTag(obj));
            elog(WARNING, "could not dump unrecognized node type: %d", (int)nodeTag(obj));

            return;
    }
}

pg_query::ParseResult pg_query_nodes_to_fbe(::stdb::memory::Arena& arena, const void* obj) {
    const ListCell* lc;
    pg_query::ParseResult parse_result;
    if (obj == nullptr) {
        return parse_result;
    }

    parse_result.version = PG_VERSION_NUM;
    foreach (lc, (List*)obj) {
        parse_result.stmts.emplace_back();
        const uint64_t size = parse_result.stmts.size();
        _outRawStmt(arena, &(parse_result.stmts[size - 1]), (const RawStmt*)lfirst(lc));
    }

    return parse_result;
}
