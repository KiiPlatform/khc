#include <string.h>
#include "catch.hpp"
#include "khc.h"

TEST_CASE( "slist append (1)" ) {
  khc_slist* list = NULL;
  khc_slist* appended = khc_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  REQUIRE ( appended->next == NULL );
  khc_slist_free_all(appended);
}

TEST_CASE( "slist append (2)" ) {
  khc_slist* list = NULL;
  khc_slist* appended = khc_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  appended = khc_slist_append(appended, "bbbb", 4);
  khc_slist* next = appended->next;
  REQUIRE ( next != NULL );
  REQUIRE( strlen(next->data) == 4 );
  REQUIRE( strncmp(next->data, "bbbb", 4) == 0 );
  REQUIRE( next->next == NULL );
  khc_slist_free_all(appended);
}