# CPU history (`/proc/cpu`)

`/proc/cpu` is a read-only **fixed-width text table** of up to 30 completed one-second CPU accounting samples, newest first. It is not CSV.

```sh
cat /proc/cpu
watch 1 cat /proc/cpu
```

```text
       |                CPU Usage (%)                 |                  CPU Time (ms)                  
Time   |    Idle    Busy  ( Console    Task    Net )  |      Idle     Busy  ( Console     Task     Net )
-------+----------------------------------------------+-------------------------------------------------
   26s |    93.7     6.3  (     2.3     0.0     4.0)  |     936.9     63.1  (    22.8      0.0     40.3)
   25s |    93.9     6.1  (     2.1     0.0     3.9)  |     939.3     60.7  (    21.4      0.0     39.4)
```

Values are illustrative. `Time` is seconds since boot. Each row describes a **completed one-second interval**; before the first interval, only the headings are returned.

| Field | Meaning |
|---|---|
| `Idle` | Remainder after subtracting accounted work from one second; not a directly measured sleep duration |
| `Busy` | Sum of `Console`, `Task`, and `Net`, capped at one second |
| `Console` | Elapsed time inside idle-task `os_console_poll()`; not simply transfer time to `/dev/console` |
| `Task` | Non-idle task time sampled at 1 ms granularity by SysTick |
| `Net` | Elapsed time inside idle-task `net_poll()`; not total Telnet communication time |

The left `CPU Usage (%)` group shows percentages to one decimal place; the right `CPU Time (ms)` group shows milliseconds to one decimal place. The kernel keeps microsecond accounting values (apart from the 1 ms task sampling) and uses integer-only conversion for display. Rounding can make component totals differ slightly from `Busy`. Interrupt time, measurement overhead, and networking outside the idle path are not separately accounted for. **This is approximate load accounting, not a precise CPU profiler.**

Reading does not erase history. The ordering is fixed when opened, and split `read()` calls are supported. Readers stalled for 30 seconds or more may lose rows overwritten in the ring. On a non-W Pico, `Net` reflects the overhead of the empty `net_poll()` call, not Wi-Fi work.

On Pico W a 250 µs repeating timer and idle-task `WFI` reduce busy spinning. Networking remains on Core 0 in the idle task, not in the timer callback. Only Core 0 is included in the table.
