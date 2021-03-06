#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "src/utils.h"
#include "src/tables.h"
#include "src/translate_utils.h"
#include "src/translate.h"

const char* TMP_FILE = "test_output.txt";
const char* OUR_TEST_FILE = "best_test_file_evar.txt";
const int BUF_SIZE = 1024;

/****************************************
 *  Helper functions 
 ****************************************/

int do_nothing() {
    return 0;
}

int init_log_file() {
    set_log_file(TMP_FILE);
    return 0;
}

int check_lines_equal(char **arr, int num) {
    char buf[BUF_SIZE];

    FILE *f = fopen(TMP_FILE, "r");
    if (!f) {
        CU_FAIL("Could not open temporary file");
        return 0;
    }
    for (int i = 0; i < num; i++) {
        if (!fgets(buf, BUF_SIZE, f)) {
            CU_FAIL("Reached end of file");
            return 0;
        }
        CU_ASSERT(!strncmp(buf, arr[i], strlen(arr[i])));
    }
    fclose(f);
    return 0;
}

/****************************************
 *  Test cases for translate_utils.c 
 ****************************************/

void test_translate_reg() {
    CU_ASSERT_EQUAL(translate_reg("$0"), 0);
    CU_ASSERT_EQUAL(translate_reg("$at"), 1);
    CU_ASSERT_EQUAL(translate_reg("$v0"), 2);
    CU_ASSERT_EQUAL(translate_reg("$a0"), 4);
    CU_ASSERT_EQUAL(translate_reg("$a1"), 5);
    CU_ASSERT_EQUAL(translate_reg("$a2"), 6);
    CU_ASSERT_EQUAL(translate_reg("$a3"), 7);
    CU_ASSERT_EQUAL(translate_reg("$t0"), 8);
    CU_ASSERT_EQUAL(translate_reg("$t1"), 9);
    CU_ASSERT_EQUAL(translate_reg("$t2"), 10);
    CU_ASSERT_EQUAL(translate_reg("$t3"), 11);
    CU_ASSERT_EQUAL(translate_reg("$s0"), 16);
    CU_ASSERT_EQUAL(translate_reg("$s1"), 17);
    CU_ASSERT_EQUAL(translate_reg("$3"), -1);
    CU_ASSERT_EQUAL(translate_reg("asdf"), -1);
    CU_ASSERT_EQUAL(translate_reg("hey there"), -1);
}

void test_translate_num() {
    long int output;

    CU_ASSERT_EQUAL(translate_num(&output, "35", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, 35);
    CU_ASSERT_EQUAL(translate_num(&output, "145634236", 0, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 145634236);
    CU_ASSERT_EQUAL(translate_num(&output, "0xC0FFEE", -9000000000, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 12648430);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 72), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 71), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 72, 150), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 73, 150), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "35x", -100, 100), -1);
}

/****************************************
 *  Test cases for tables.c 
 ****************************************/

void test_table_1() {
    int retval;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    retval = add_to_table(tbl, "abc", 8);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "efg", 12);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 24); 
    CU_ASSERT_EQUAL(retval, -1); 
    retval = add_to_table(tbl, "bob", 14); 
    CU_ASSERT_EQUAL(retval, -1); 

    retval = get_addr_for_symbol(tbl, "abc");
    CU_ASSERT_EQUAL(retval, 8); 
    retval = get_addr_for_symbol(tbl, "q45");
    CU_ASSERT_EQUAL(retval, 16); 
    retval = get_addr_for_symbol(tbl, "ef");
    CU_ASSERT_EQUAL(retval, -1);
    
    free_table(tbl);

    char* arr[] = { "Error: name 'q45' already exists in table.",
                    "Error: address is not a multiple of 4." };
    check_lines_equal(arr, 2);

    SymbolTable* tbl2 = create_table(SYMTBL_NON_UNIQUE);
    CU_ASSERT_PTR_NOT_NULL(tbl2);

    retval = add_to_table(tbl2, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl2, "q45", 24); 
    CU_ASSERT_EQUAL(retval, 0);

    free_table(tbl2);

}

void test_table_2() {
    int retval, max = 100;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    char buf[10];
    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = add_to_table(tbl, buf, 4 * i);
        CU_ASSERT_EQUAL(retval, 0);
    }

    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = get_addr_for_symbol(tbl, buf);
        CU_ASSERT_EQUAL(retval, 4 * i);
    }

    free_table(tbl);
}

/****************************************
 *  Add your test cases here
 ****************************************/

void test_addu() {
    // set_log_file(OUR_TEST_FILE);
    FILE *file = fopen(TMP_FILE, "r+");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);
    
    char* name = "addu";
    char* args[3];
    args[0] = "$s0";
    args[1] = "$0";
    args[2] = "$0";
    size_t num_args = 3;
    uint32_t addr = 0;
    
    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_sll() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "sll";
    char* args[3];
    args[0] = "$v0";
    args[1] = "$s1";
    args[2] = "3";
    size_t num_args = 3;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_addiu() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "addiu";
    char* args[3];
    args[0] = "$a0";
    args[1] = "$t0";
    args[2] = "-14";
    size_t num_args = 3;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_ori() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "ori";
    char* args[3];
    args[0] = "$s2";
    args[1] = "$t1";
    args[2] = "4";
    size_t num_args = 3;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_slt() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "slt";
    char* args[3];
    args[0] = "$a0";
    args[1] = "$a1";
    args[2] = "$a2";
    size_t num_args = 3;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_lui() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "lui";
    char* args[2];
    args[0] = "$a3";
    args[1] = "17";
    size_t num_args = 2;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_sw() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "sw";
    char* args[3];
    args[0] = "$s3";
    args[1] = "5";
    args[2] = "$at";
    size_t num_args = 3;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_jr() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "jr";
    char* args[1];
    args[0] = "$sp";
    size_t num_args = 1;
    uint32_t addr = 0;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_bne() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);
    add_to_table(symtbl, "label", 0x00000000);

    int label_addr = get_addr_for_symbol(symtbl, "label");
    CU_ASSERT_EQUAL(label_addr, 0);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "bne";
    char* args[3];
    args[0] = "$t3";
    args[1] = "$t0";
    args[2] = "label";
    size_t num_args = 3;
    uint32_t addr = 0x00000004;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_jal() {
    FILE *file = fopen(TMP_FILE, "a");

    SymbolTable* symtbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(symtbl);
    add_to_table(symtbl, "label", 0x00000004);
    int label_addr = get_addr_for_symbol(symtbl, "label");
    CU_ASSERT_EQUAL(label_addr, 4);

    SymbolTable* reltbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(reltbl);

    char* name = "jal";
    char* args[1];
    args[0] = "label";
    size_t num_args = 1;
    uint32_t addr = 0x0000000c;

    int result = translate_inst(file, name, args, num_args, addr, symtbl, reltbl);
    CU_ASSERT_EQUAL(result, 0);

    fclose(file);
}

void test_li() {
    FILE *file = fopen(TMP_FILE, "a");

    char* name = "li";
    char* args[2];
    args[0] = "$v0";
    args[1] = "26";
    size_t num_args = 2;

    int result = write_pass_one(file, name, args, num_args);
    CU_ASSERT_EQUAL(result, 1);

    fclose(file);
}



int main(int argc, char** argv) {
    CU_pSuite pSuite1 = NULL, pSuite2 = NULL, pSuite3 = NULL, pSuite4 = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Suite 1 */
    pSuite1 = CU_add_suite("Testing translate_utils.c", NULL, NULL);
    if (!pSuite1) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_reg", test_translate_reg)) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_num", test_translate_num)) {
        goto exit;
    }

    /* Suite 2 */
    pSuite2 = CU_add_suite("Testing tables.c", init_log_file, NULL);
    if (!pSuite2) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_1", test_table_1)) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_2", test_table_2)) {
        goto exit;
    }

    /* Suite 3 */
    pSuite3 = CU_add_suite("Testing translate.c", NULL, NULL);
    if (!pSuite3) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_addu", test_addu)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_sll", test_sll)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_addiu", test_addiu)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_ori", test_ori)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_slt", test_slt)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_lui", test_lui)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_sw", test_sw)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_jr", test_jr)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_bne", test_bne)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_jal", test_jal)) {
        goto exit;
    }

    /* Suite 4*/
    pSuite4 = CU_add_suite("Testing pseudoexpansions", NULL, NULL);
    if (!pSuite4) {
        goto exit;
    }
    if (!CU_add_test(pSuite4, "test_li", test_li)) {
        goto exit;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

exit:
    CU_cleanup_registry();
    return CU_get_error();;
}
