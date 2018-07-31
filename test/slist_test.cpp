#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include "kii_http.h"

TEST_CASE( "slist append (1)" ) {
  kii_slist* list = NULL;
  kii_slist* appended = kii_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
}