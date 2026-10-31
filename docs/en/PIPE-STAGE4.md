# Pipe stage 4: scheduler integration

The pipe read/write SVC now suspends a task on empty/full buffers (unless O_NONBLOCK is set). The wake_pipes routine runs after PendSV has saved the interrupted task's context and completes pending reads/writes by updating the saved r0. EOF occurs when the last writer closes and the buffer drains. A writer with no readers gets OS_EIO.

This is an intermediate implementation: shell `cmd1 | cmd2` is not yet supported. No ARM firmware build or on-device test has been performed. Static source checks are not functional tests.
