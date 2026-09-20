/*
 * Copyright 2026 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU General Public License version 2
 * or later (GPLv2+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <crm/common/unittest_internal.h>

// Exercise the static callback and child state without exporting daemon internals.
#include "../pcmkd_subdaemons.c"

static const char *fail_fast = NULL;
static pcmkd_child_t saved_children[PCMK__NELEM(pcmk_children)];
static pcmkd_child_t *target = &pcmk_children[PCMK_CHILD_CONTROLD];

int __wrap_pcmk__daemon_user(uid_t *uid, gid_t *gid);
const char *__wrap_pcmk__env_option(const char *option);
int __wrap_pcmk__ipc_is_authentic_process_active(const char *name, uid_t refuid,
                                                gid_t refgid, pid_t *gotpid);
int __wrap_pcmk__pid_active(pid_t pid, const char *daemon);
int __wrap_kill(pid_t pid, int sig);
void __wrap_pcmk__panic(const char *reason);

int
__wrap_pcmk__daemon_user(uid_t *uid, gid_t *gid)
{
    *uid = 0;
    *gid = 0;
    return pcmk_rc_ok;
}

const char *
__wrap_pcmk__env_option(const char *option)
{
    assert_string_equal(option, PCMK__ENV_FAIL_FAST);
    return fail_fast;
}

int
__wrap_pcmk__ipc_is_authentic_process_active(const char *name, uid_t refuid,
                                            gid_t refgid, pid_t *gotpid)
{
    check_expected(name);
    *gotpid = mock_type(pid_t);
    return mock_type(int);
}

int
__wrap_pcmk__pid_active(pid_t pid, const char *daemon)
{
    assert_int_equal(pid, target->pid);
    assert_string_equal(daemon, pcmk__server_name(target->server));
    return pcmk_rc_ok;
}

int
__wrap_kill(pid_t pid, int sig)
{
    function_called();
    assert_int_equal(pid, target->pid);
    assert_int_equal(sig, SIGKILL);
    // Signal delivery succeeds, but no child-exit callback follows.
    return 0;
}

void
__wrap_pcmk__panic(const char *reason)
{
    function_called();
    assert_string_equal(reason, "Subdaemon is unresponsive");
    assert_int_equal(target->pid, 1000 + PCMK_CHILD_CONTROLD);
    assert_true(pcmk__is_set(target->flags, child_shutting_down));
}

#if SUPPORT_COROSYNC
bool
pcmkd_corosync_connected(void)
{
    fail_msg("Unexpected cluster connection check");
    return false;
}

void
pcmkd_shutdown_corosync(void)
{
    fail_msg("Unexpected Corosync shutdown");
}
#endif

static int
setup_group(void **state)
{
    memcpy(saved_children, pcmk_children, sizeof(pcmk_children));
    return 0;
}

static int
setup(void **state)
{
    memcpy(pcmk_children, saved_children, sizeof(pcmk_children));
    for (int i = 0; i < PCMK__NELEM(pcmk_children); i++) {
        pcmk_children[i].pid = 1000 + i;
    }
    shutdown_trigger = NULL;
    fail_fast = NULL;
    return 0;
}

static void
check_round(bool responsive)
{
    // Complete a round so the callback's static index returns to its start.
    for (int i = 0; i < PCMK__NELEM(pcmk_children); i++) {
        pcmkd_child_t *child = &pcmk_children[i];
        bool healthy = (child != target) || responsive;

        expect_string(__wrap_pcmk__ipc_is_authentic_process_active, name,
                      pcmk__server_ipc_name(child->server));
        will_return(__wrap_pcmk__ipc_is_authentic_process_active,
                    healthy? child->pid : 0);
        will_return(__wrap_pcmk__ipc_is_authentic_process_active,
                    healthy? pcmk_rc_ok : pcmk_rc_ipc_unresponsive);
        assert_int_equal(check_next_subdaemon(NULL), G_SOURCE_CONTINUE);
    }
    assert_int_equal(target->pid, 1000 + PCMK_CHILD_CONTROLD);
}

static void
panic_before_child_exit(void **state)
{
    fail_fast = "yes";
    for (int i = 1; i < PCMK_PROCESS_CHECK_RETRIES; i++) {
        check_round(false);
        assert_int_equal(target->check_count, i);
    }

    expect_function_call(__wrap_kill);
    expect_function_call(__wrap_pcmk__panic);
    check_round(false);
}

static void
fail_fast_unset(void **state)
{
    target->check_count = PCMK_PROCESS_CHECK_RETRIES - 1;
    expect_function_call(__wrap_kill);
    check_round(false);
    assert_int_equal(target->check_count, 0);
}

static void
fail_fast_disabled(void **state)
{
    fail_fast = "no";
    fail_fast_unset(state);
}

static void
controlled_shutdown(void **state)
{
    fail_fast = "yes";
    // Only pointer presence is tested; no shutdown callback should run.
    shutdown_trigger = (crm_trigger_t *) state;
    target->check_count = PCMK_PROCESS_CHECK_RETRIES - 1;
    expect_function_call(__wrap_kill);
    check_round(false);
}

static void
respawn_disabled(void **state)
{
    fail_fast = "yes";
    target->flags &= ~child_respawn;
    target->check_count = PCMK_PROCESS_CHECK_RETRIES - 1;
    expect_function_call(__wrap_kill);
    check_round(false);
}

static void
child_already_shutting_down(void **state)
{
    fail_fast = "yes";
    target->flags |= child_shutting_down;
    target->check_count = PCMK_PROCESS_CHECK_RETRIES - 1;
    check_round(false);
}

static void
responsive_child_resets_retries(void **state)
{
    fail_fast = "yes";
    target->check_count = PCMK_PROCESS_CHECK_RETRIES - 1;
    check_round(true);
    assert_int_equal(target->check_count, 0);
}

PCMK__UNIT_TEST(setup_group, NULL,
    cmocka_unit_test_setup(panic_before_child_exit, setup),
    cmocka_unit_test_setup(fail_fast_unset, setup),
    cmocka_unit_test_setup(fail_fast_disabled, setup),
    cmocka_unit_test_setup(controlled_shutdown, setup),
    cmocka_unit_test_setup(respawn_disabled, setup),
    cmocka_unit_test_setup(child_already_shutting_down, setup),
    cmocka_unit_test_setup(responsive_child_resets_retries, setup))
