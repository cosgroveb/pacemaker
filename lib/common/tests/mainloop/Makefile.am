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

WRAPPED += qb_ipcs_create
WRAPPED += qb_ipcs_create_2
WRAPPED += qb_ipcs_request_rate_limit
WRAPPED += qb_ipcs_run

check_PROGRAMS = pcmk__add_mainloop_ipc_server_test
# Compile the tested code here so link-time wrappers intercept its libqb calls.
pcmk__add_mainloop_ipc_server_test_SOURCES = pcmk__add_mainloop_ipc_server_test.c
pcmk__add_mainloop_ipc_server_test_SOURCES += $(top_srcdir)/lib/common/mainloop.c
TESTS = $(check_PROGRAMS)
