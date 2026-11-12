# Ownership regressions

`engine_harness.mm` links the real `4DPlugin.cpp`, the plugin classes and the unchanged bundled `4D Plugin API` with ASan/UBSan. It also replaces the 4D engine (`Call4D`) with a synthetic one that counts live engine strings and aborts on anything it does not model.

No folder is watched. The harness feeds a synthetic, non-existent path straight into the FSEvents callback `gotEvent`, then runs `listenerLoopExecuteMethod` directly.

```sh
tests/ownership/run.sh . fixed                # every test must pass

B=$(mktemp -d); git archive <base-sha> | tar -x -C "$B"
tests/ownership/run.sh "$B" baseline          # named defects must fail, controls pass
```

| Test | Checks |
|---|---|
| `defect_byid_param_owner_leak` | `PA_ExecuteMethodByID` parameters leave no engine strings behind |
| `defect_1007_param_owner_leak` | `EXECUTE METHOD` (1007) parameters leave no engine strings behind |
| `defect_byid_result_not_cleared` | a text result from the method is released |
| `guard_byid_error_result_untouched` | the result is not touched when the engine reports an error |
| `control_*` | arguments reach the method intact; `FW GET WATCH PATHS` array ownership |

Not covered:
- The Windows `ReadDirectoryChangesW` thread, which reaches the same `listenerLoopExecuteMethod`.
- The `CALLBACK_IN_NEW_PROCESS` branch, which is dormant (`0`).
- A real 4D runtime.
