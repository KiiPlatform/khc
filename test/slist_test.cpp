#define CATCH_CONFIG_MAIN

#include <string.h>
#include "catch.hpp"
#include "kii_http.h"

TEST_CASE( "slist append (1)" ) {
  kii_slist* list = NULL;
  kii_slist* appended = kii_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  REQUIRE ( appended->next == NULL );
  kii_slist_free_all(appended);
}

TEST_CASE( "slist append (2)" ) {
  kii_slist* list = NULL;
  kii_slist* appended = kii_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  appended = kii_slist_append(appended, "bbbb", 4);
  kii_slist* next = appended->next;
  REQUIRE ( next != NULL );
  REQUIRE( strlen(next->data) == 4 );
  REQUIRE( strncmp(next->data, "bbbb", 4) == 0 );
  REQUIRE( next->next == NULL );
  kii_slist_free_all(appended);
}