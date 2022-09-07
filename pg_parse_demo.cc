// Welcome to the easiest way to parse an SQL query :-)
// Compile the file like this: ninja pg_parse_demo
// usage: echo "select a from t" | ./pg_parse_demo

#include <pg_query.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

auto main() -> int {
    std::string sql;
    std::getline(std::cin, sql);

    PgQueryParseResult result = pg_query_parse(sql.c_str());
    if (result.error != nullptr) {
        printf("error: %s at %d\n", result.error->message, result.error->cursorpos);
    } else {
        printf("%s\n", result.parse_tree);
    }

    pg_query_free_parse_result(result);
    pg_query_exit();
    return 0;
}
