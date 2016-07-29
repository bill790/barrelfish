/**
 * \file
 * \brief remote retype test
 */

/*
 * Copyright (c) 2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <barrelfish/barrelfish.h>
#include <if/test_defs.h>

#include "test.h"

__attribute__((unused))
static void debug_capref(const char *prefix, struct capref cap)
{
    char buf[128];
    debug_print_capref(buf, 128, cap);
    printf("%s capref = %s\n", prefix, buf);
}

enum server_op {
    SERVER_OP_CREATE_COPY,
    // Create a BASE_PAGE_SIZEd Frame cap of last page in RAM
    SERVER_OP_RETYPE_1,
    // Exit server
    SERVER_OP_EXIT,
};

enum client_op {
    // do a retype on client while server holds copies of cap but no
    // descendants
    CLIENT_OP_RETYPE_1,
    // Do retype (overlapping and non-overlapping) on client while server
    // holds copies of cap and Frame of last page in region
    CLIENT_OP_RETYPE_2,
};

//{{{1 Server-side cap operations

struct server_state {
    struct capref ram;
    struct capref copy;
    struct capref frame1;
};

void init_server(struct test_binding *b)
{
    b->st = malloc(sizeof(struct server_state));
    assert(b->st);
    if (!b->st) {
        USER_PANIC("state malloc() in server");
    }
}

void server_do_test(struct test_binding *b, uint32_t test, struct capref cap)
{
    errval_t err;
    struct server_state *st = b->st;

    switch(test) {
        case SERVER_OP_CREATE_COPY:
            // First call: store RAM cap in our state
            st->ram = cap;
            printf("server: creating a copy\n");
            err = slot_alloc(&st->copy);
            PANIC_IF_ERR(err, "slot alloc for copy in server");

            err = cap_copy(st->copy, cap);
            PANIC_IF_ERR(err, "copy in server");

            err = test_basic__tx(b, NOP_CONT, CLIENT_OP_RETYPE_1);
            PANIC_IF_ERR(err, "server: triggering 1st retype on client");
            break;

        case SERVER_OP_RETYPE_1:
            printf("server: retype #1\n");
            err = slot_alloc(&st->frame1);
            PANIC_IF_ERR(err, "slot alloc for retype in server");

            err = cap_retype(st->frame1, st->copy, RAM_SIZE - BASE_PAGE_SIZE,
                             ObjType_Frame, BASE_PAGE_SIZE, 1);
            PANIC_IF_ERR(err, "retype last page in server");

            printf("server: triggering 2nd set of retypes in client\n");
            err = test_basic__tx(b, NOP_CONT, CLIENT_OP_RETYPE_2);
            PANIC_IF_ERR(err, "server: triggering 2nd set of retypes on client");
            break;

        case SERVER_OP_EXIT:
            printf("server: exit\n");
            exit(0);

        default:
            USER_PANIC("server: Unknown test %"PRIu32"\n", test);
    }
}

//{{{1 Client-side cap operations

struct client_state {
    struct capref ram;
};

void init_client(struct test_binding *b)
{
    errval_t err;

    // Allocate client state struct
    b->st = malloc(sizeof(struct client_state));
    assert(b->st);
    if (!b->st) {
        USER_PANIC("state malloc() in client");
    }
    struct client_state *cst = b->st;

    // Allocate RAM cap
    err = ram_alloc(&cst->ram, RAM_BITS);
    PANIC_IF_ERR(err, "in client: ram_alloc");

    // Trigger Server
    err = test_caps__tx(b, NOP_CONT, SERVER_OP_CREATE_COPY, cst->ram, NULL_CAP);
    PANIC_IF_ERR(err, "in client: sending cap to server");
}

static void client_test_retype(struct capref src, gensize_t offset,
                               gensize_t size, size_t count, errval_t expected_err)
{
    errval_t err;
    struct capref result;
    err = slot_alloc(&result);
    PANIC_IF_ERR(err, "in client: slot_alloc for retype result");

    // Tests come here
    err = cap_retype(result, src, offset, ObjType_Frame, size, count);
    if (err != expected_err) {
        printf("distops_retype: retype(offset=%"PRIuGENSIZE", size=%"PRIuGENSIZE
                     ", count=%zu) to Frame returned %s, expected %s\n",
                     offset, size, count, err_getcode(err), err_getcode(expected_err));
    }
    if (err_is_ok(err)) {
        // Cap delete only necessary if retype successful
        err = cap_delete(result);
        PANIC_IF_ERR(err, "cap_delete in client_test_retype");
    }
    err = slot_free(result);
    PANIC_IF_ERR(err, "slot_free in client_test_retype");
}

void client_do_test(struct test_binding *b, uint32_t test, struct capref cap)
{
    errval_t err;
    struct client_state *st = b->st;
    assert(st);

    switch(test) {
        // Do retype while copies exist on other core
        case CLIENT_OP_RETYPE_1:
            printf("client: retype #1\n");
            printf("   retype first page: should succeed\n");
            client_test_retype(st->ram, 0, BASE_PAGE_SIZE, 1, SYS_ERR_OK);
            err = test_basic__tx(b, NOP_CONT, SERVER_OP_RETYPE_1);
            PANIC_IF_ERR(err, "client: trigger first retype on server");
            break;

        case CLIENT_OP_RETYPE_2:
            printf("client: retype #2\n");
            printf("   retype last page: should fail\n");
            client_test_retype(st->ram, RAM_SIZE - BASE_PAGE_SIZE, BASE_PAGE_SIZE, 1,
                    SYS_ERR_REVOKE_FIRST);

            printf("   retype first page: should succeed\n");
            client_test_retype(st->ram, 0, BASE_PAGE_SIZE, 1, SYS_ERR_OK);

            printf("   retype 2nd last page: should succeed\n");
            client_test_retype(st->ram, RAM_SIZE - 2*BASE_PAGE_SIZE,
                               BASE_PAGE_SIZE, 1, SYS_ERR_OK);

            err = test_basic__tx(b, NOP_CONT, SERVER_OP_EXIT);
            PANIC_IF_ERR(err, "client: trigger exit on server");
            printf("distops_retype: client done\n");
            exit(0);

        default:
            USER_PANIC("client: Unknown test %"PRIu32"\n", test);
    }
}
