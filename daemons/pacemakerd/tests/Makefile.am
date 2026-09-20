#
# Copyright 2026 the Pacemaker project contributors
#
# The version control history for this file may have further details.
#
# This source code is licensed under the GNU General Public License version 2
# or later (GPLv2+) WITHOUT ANY WARRANTY.
#

include $(top_srcdir)/mk/common.mk
include $(top_srcdir)/mk/tap.mk
include $(top_srcdir)/mk/unittest.mk

WRAPPED += kill
WRAPPED += pcmk__daemon_user
WRAPPED += pcmk__env_option
WRAPPED += pcmk__ipc_is_authentic_process_active
WRAPPED += pcmk__panic
WRAPPED += pcmk__pid_active

LDADD += $(top_builddir)/lib/cluster/libcrmcluster.la

check_PROGRAMS = check_next_subdaemon_test

TESTS = $(check_PROGRAMS)
