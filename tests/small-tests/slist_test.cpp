#include <string.h>
#include "catch.hpp"
#include "kch.h"

TEST_CASE( "slist append (1)" ) {
  kch_slist* list = NULL;
  kch_slist* appended = kch_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  REQUIRE ( appended->next == NULL );
  kch_slist_free_all(appended);
}

TEST_CASE( "slist append (2)" ) {
  kch_slist* list = NULL;
  kch_slist* appended = kch_slist_append(list, "aaaaa", 5);
  REQUIRE( appended != NULL );
  REQUIRE( strlen(appended->data) == 5 );
  REQUIRE( strncmp(appended->data, "aaaaa", 5) == 0 );
  appended = kch_slist_append(appended, "bbbb", 4);
  kch_slist* next = appended->next;
  REQUIRE ( next != NULL );
  REQUIRE( strlen(next->data) == 4 );
  REQUIRE( strncmp(next->data, "bbbb", 4) == 0 );
  REQUIRE( next->next == NULL );
  kch_slist_free_all(appended);
}