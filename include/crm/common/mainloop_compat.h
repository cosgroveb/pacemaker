/*
 * Copyright 2026 the Pacemaker project contributors
 *
 * The version control history for this file may have further details.
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#ifndef PCMK__CRM_COMMON_MAINLOOP_COMPAT__H
#define PCMK__CRM_COMMON_MAINLOOP_COMPAT__H

#include <crm/common/mainloop.h>    // mainloop_*

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file
 * \brief Deprecated Pacemaker main event loop API
 * \ingroup core
 * \deprecated Do not include this header directly. The APIs in this header
 *             will be removed in a future release.
 */

//! \deprecated Do not use
qb_ipcs_service_t *mainloop_add_ipc_server(const char *name, enum qb_ipc_type type,
                                           struct qb_ipcs_service_handlers *callbacks);

//! \deprecated Do not use
qb_ipcs_service_t *mainloop_add_ipc_server_with_prio(const char *name,
                                                    enum qb_ipc_type type,
                                                    struct qb_ipcs_service_handlers *callbacks,
                                                    enum qb_loop_priority prio);

#ifdef __cplusplus
}
#endif

#endif
