## Timer execution

**NotoJS** can execute notebooks automatically on a minute-based schedule. Timers are configured in the server configuration file and run only in the main server process.

### Configuration

Add a `[timers]` section to the **NotoJS** `INI` configuration:

```ini
[timers]
5m = cleanup.notojs
15m = refresh.notojs
60m = hourly-report.notojs
```

Each entry maps an interval to a top-level notebook file in the configured workspace.

The key must use `Nm` format:

- `N` is the interval in minutes.
- `m` is the required suffix.
- `N` must divide `60` evenly.

Valid examples include:

```ini
1m = every-minute.notojs
5m = every-five-minutes.notojs
10m = every-ten-minutes.notojs
15m = quarterly.notojs
30m = twice-hourly.notojs
60m = hourly.notojs
```

Invalid intervals, such as `7m`, throw a configuration error because they do not divide the hour evenly.

Notebook names must:

- end with `.notojs`
- be top-level workspace notebook names
- not contain directory separators such as `/`

For example, `cleanup.notojs` is valid, while `jobs/cleanup.notojs` is rejected.

### Schedule behavior

Timers run on wall-clock minute boundaries. For an interval `Nm`, **NotoJS** schedules the notebook at minute values `0, N, 2N, ...` within each hour.

For example:

| Interval | Runs at minutes |
| --- | --- |
| `5m` | `00`, `05`, `10`, ..., `55` |
| `15m` | `00`, `15`, `30`, `45` |
| `30m` | `00`, `30` |
| `60m` | `00` |

If the server starts between scheduled boundaries, the first run happens at the next matching minute.

### Execution environment

When a timer fires, **NotoJS** executes every code cell in the scheduled notebook in order. 

The timer execution defines global `input` as an object containing the scheduled time:

```javascript!noplay
new Storage('timers').setItem('last_run', input);
// { hour: 14, minute: 30 }
```

Use this to make one notebook handle multiple schedules or branch based on the current run time:

```javascript!noplay
if(input.minute === 0) {
  // ...
} else {
  // ...
}
```

Timer runs are silent: notebook output is discarded. Errors are recorded in `sys:errorlog` database.

### Compilation and updates

Scheduled notebooks are cached as compiled bytecode. **NotoJS** recompiles a notebook when its stored version changes.

When a notebook is saved through the workspace API or editor, the timer system is notified and reloads the updated version for future runs. If a scheduled notebook is removed or renamed, its cached version is cleared and the timer stops executing it until a valid notebook with that configured name exists again.

Compilation and runtime errors are recorded through **NotoJS** error storage for the notebook name. If a cell fails, execution of the remaining cells in that timer run stops.

### Lifecycle

Timers start when the main server starts:

```sh
notojs /path/to/notojs.ini
```

They stop when the server receives `SIGINT` or `SIGTERM` and shuts down its plugin, timer, and window services.

No timer thread is created when the `[timers]` section is empty or missing.

###### See also
- `doc('topic:config')`
- `doc('topic:lmdb')`
