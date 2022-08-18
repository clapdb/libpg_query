#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse_tests.c"
#include "pg_query_parse_fbe.hpp"

int main() {
    int ret_code = 0;

    for (size_t i = 0; i < testsLength; i += 2) {
        PgQueryFBEParseResult result = pg_query_parse_fbe(tests[i]);

        if (result.error != nullptr) {
            ret_code = -1;
            printf("%s\n", result.error->message);
        } else {
            printf(".");
        }

        pg_query_free_fbe_parse_result(result);
    }

    printf("\n");

    pg_query_exit();

    return ret_code;
}
