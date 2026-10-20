Installing Cluster Software
---------------------------

.. index:: installation

Most major Linux distributions have pacemaker packages in their standard
package repositories, or the software can be built from source code. See
`How to Install <https://projects.clusterlabs.org/w/cluster_administration/how_to_install/>`_
on the ClusterLabs wiki for details.

Locked IPC Memory
#################

When built with a supporting libqb, Pacemaker requires locked shared-memory IPC
rings for local daemon connections. Locking prevents these pages from being
swapped out while the server retains the connection. Builds with older libqb
versions log a warning and use unlocked rings.

For daemons without ``CAP_IPC_LOCK``, the inherited ``RLIMIT_MEMLOCK`` soft limit
controls admission. An insufficient limit rejects new connections and can prevent
cluster startup. Check each running daemon's limits in ``/proc/PID/limits`` and
locked bytes (``VmLck``) in ``/proc/PID/status``, including daemons running as the
cluster user. Size a finite budget for peak simultaneous connections, allowing
room for temporary supervision probes and monitoring clients. Both CIB endpoints
share one process budget, as do the remote executor's local proxy services.

The packaged systemd units set an 8 MiB limit per process. Adjust the soft and hard
limits through a ``LimitMEMLOCK`` drop-in for ``pacemaker.service`` or
``pacemaker_remote.service`` as needed for the deployment's connection capacity.
For other launchers, set both limits before starting Pacemaker and before dropping
privileges, for example with the shell's ``ulimit -l`` command. Child daemons
inherit these limits. Apply changes during a planned service restart and verify
the resulting limits and connection capacity.
