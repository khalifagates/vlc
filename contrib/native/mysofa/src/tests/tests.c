#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <string.h>
#include <stdio.h>
#include "tests.h"

int main() {

    int err;
    
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite Tools", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	/* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
	if ((NULL == CU_add_test(pSuite, "test of coordinates", test_coordinates))
		|| (NULL == CU_add_test(pSuite, "test of nsearch", test_nsearch))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_check", test_check))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_lookup", test_lookup))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_neighbors", test_neighbors))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_interpolate", test_interpolate))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_resample", test_resample))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_loudness", test_loudness))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_minphase", test_minphase))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_cache", test_cache))
		|| (NULL == CU_add_test(pSuite, "test of mysofa_easy", test_easy))
			) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	err = CU_get_number_of_failures();
	CU_cleanup_registry();
	return err;
}
