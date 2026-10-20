/*
 * Copyright 2026 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU General Public License version 2
 * or later (GPLv2+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <errno.h>
#include <stdint.h>

#include <qb/qbipcs.h>
#include <crm/common/mainloop.h>
#include <crm/common/unittest_internal.h>

#include "crmcommon_private.h"

static struct qb_ipcs_service_handlers callbacks = { 0 };
static qb_ipcs_service_t *created = NULL;

qb_ipcs_service_t *__real_qb_ipcs_create(
    const char *name, int32_t service_id, enum qb_ipc_type type,
    struct qb_ipcs_service_handlers *handlers);
qb_ipcs_service_t *__wrap_qb_ipcs_create(
    const char *name, int32_t service_id, enum qb_ipc_type type,
    struct qb_ipcs_service_handlers *handlers);

#ifdef HAVE_QB_IPCS_CREATE_2
qb_ipcs_service_t *__real_qb_ipcs_create_2(
    const char *name, int32_t service_id, enum qb_ipc_type type,
    struct qb_ipcs_service_handlers *handlers, uint32_t flags);
qb_ipcs_service_t *__wrap_qb_ipcs_create_2(
    const char *name, int32_t service_id, enum qb_ipc_type type,
    struct qb_ipcs_service_handlers *handlers, uint32_t flags);
#endif

void __real_qb_ipcs_request_rate_limit(qb_ipcs_service_t *server,
                                      enum qb_ipcs_rate_limit rate);
void __wrap_qb_ipcs_request_rate_limit(qb_ipcs_service_t *server,
                                      enum qb_ipcs_rate_limit rate);
int32_t __wrap_qb_ipcs_run(qb_ipcs_service_t *server);

qb_ipcs_service_t *
__wrap_qb_ipcs_create(const char *name, int32_t service_id,
                      enum qb_ipc_type type,
                      struct qb_ipcs_service_handlers *handlers)
{
    function_called();
    assert_string_equal(name, "pcmk-ipc-test");
    assert_int_equal(service_id, 0);
    assert_int_equal(type, QB_IPC_SHM);
    assert_ptr_equal(handlers, &callbacks);
    if (!mock()) {
        errno = ENOMEM;
        return NULL;
    }
    created = __real_qb_ipcs_create(name, service_id, type, handlers);
    assert_non_null(created);
    return created;
}

#ifdef HAVE_QB_IPCS_CREATE_2
qb_ipcs_service_t *
__wrap_qb_ipcs_create_2(const char *name, int32_t service_id,
                        enum qb_ipc_type type,
                        struct qb_ipcs_service_handlers *handlers,
                        uint32_t flags)
{
    function_called();
    assert_string_equal(name, "pcmk-ipc-test");
    assert_int_equal(service_id, 0);
    assert_int_equal(type, QB_IPC_SHM);
    assert_ptr_equal(handlers, &callbacks);
    check_expected_uint(flags);
    if (!mock()) {
        errno = ENOMEM;
        return NULL;
    }
    created = __real_qb_ipcs_create_2(name, service_id, type, handlers, flags);
    assert_non_null(created);
    return created;
}
#endif

void
__wrap_qb_ipcs_request_rate_limit(qb_ipcs_service_t *server,
                                  enum qb_ipcs_rate_limit rate)
{
    function_called();
    assert_ptr_equal(server, created);
    check_expected_int(rate);
    __real_qb_ipcs_request_rate_limit(server, rate);
}

int32_t
__wrap_qb_ipcs_run(qb_ipcs_service_t *server)
{
    function_called();
    assert_ptr_equal(server, created);
    // Exercise real construction and setup without creating a listening socket.
    return 0;
}

static void
expect_constructor(uint32_t flags, bool succeeds)
{
#ifdef HAVE_QB_IPCS_CREATE_2
    expect_function_call(__wrap_qb_ipcs_create_2);
    expect_uint_value(__wrap_qb_ipcs_create_2, flags, flags);
    will_return(__wrap_qb_ipcs_create_2, succeeds);
#else
    assert_int_equal(flags, 0);
    expect_function_call(__wrap_qb_ipcs_create);
    will_return(__wrap_qb_ipcs_create, succeeds);
#endif
}

static uint32_t
locking_flags(void)
{
#ifdef HAVE_QB_IPCS_CREATE_2
    return QB_IPCS_REQUIRE_LOCKED_MAPPINGS;
#else
    return 0U;
#endif
}

static void
private_flags_and_priority(void **state)
{
    qb_ipcs_service_t *server = NULL;

    expect_constructor(locking_flags(), true);
    expect_function_call(__wrap_qb_ipcs_request_rate_limit);
    expect_int_value(__wrap_qb_ipcs_request_rate_limit, rate, QB_IPCS_RATE_FAST);
    expect_function_call(__wrap_qb_ipcs_run);
    server = pcmk__add_mainloop_ipc_server("pcmk-ipc-test", &callbacks,
                                         QB_LOOP_HIGH, locking_flags());
    assert_ptr_equal(server, created);
    qb_ipcs_unref(server);
    created = NULL;
}

static void
public_default_flags(void **state)
{
    qb_ipcs_service_t *server = NULL;

    expect_constructor(0U, true);
    expect_function_call(__wrap_qb_ipcs_run);
    server = mainloop_add_ipc_server("pcmk-ipc-test", QB_IPC_NATIVE, &callbacks);
    assert_ptr_equal(server, created);
    qb_ipcs_unref(server);
    created = NULL;
}

static void
public_priority_flags(void **state)
{
    qb_ipcs_service_t *server = NULL;

    expect_constructor(0U, true);
    expect_function_call(__wrap_qb_ipcs_request_rate_limit);
    expect_int_value(__wrap_qb_ipcs_request_rate_limit, rate, QB_IPCS_RATE_FAST);
    expect_function_call(__wrap_qb_ipcs_run);
    server = mainloop_add_ipc_server_with_prio("pcmk-ipc-test", QB_IPC_NATIVE,
                                              &callbacks, QB_LOOP_HIGH);
    assert_ptr_equal(server, created);
    qb_ipcs_unref(server);
    created = NULL;
}

static void
constructor_failure(void **state)
{
    expect_constructor(locking_flags(), false);
    assert_null(pcmk__add_mainloop_ipc_server("pcmk-ipc-test", &callbacks,
                                            QB_LOOP_HIGH, locking_flags()));
}

PCMK__UNIT_TEST(NULL, NULL,
               cmocka_unit_test(private_flags_and_priority),
               cmocka_unit_test(public_default_flags),
               cmocka_unit_test(public_priority_flags),
               cmocka_unit_test(constructor_failure))
