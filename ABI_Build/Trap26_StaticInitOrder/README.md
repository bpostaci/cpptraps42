# Trap 26 — Static Initialization Order

**Rule:** do not make dynamic initialization in one translation unit depend on dynamic initialization in another translation unit.

**This trap is not plain undefined behavior.** Cross-translation-unit dynamic initialization order is unspecified; sanitizer silence is expected because the defect is a startup ordering contract.

## The rule

Objects with static storage duration are initialized before `main`, but the relative order of dynamic initialization across different translation units is unspecified. If one global object's initializer reads another dynamically initialized global from a different translation unit, the program depends on link order or other implementation details.

The usual fix is construct-on-first-use: put the object behind a function-local static. Since C++11, initialization of a function-local static is ordered by the first call and is thread-safe.

## In this code

The trap is split across four files.

| File | Contribution |
|---|---|
| `state.hpp` | declares `Configuration`, `global_configuration`, `copied_during_static_initialization`, and `safe_configuration()` |
| `config.cpp` | defines `Configuration::Configuration()` as `value(42)`, defines `global_configuration`, and implements `safe_configuration()` with a function-local static |
| `dependent.cpp` | defines `copied_during_static_initialization = global_configuration.value` during static initialization |
| `main.cpp` | safe target prints `safe_configuration().value`; unsafe target prints `copied_during_static_initialization` |

- **Safe target** (`Trap26_StaticInitOrder`) — uses construct-on-first-use.
- **Unsafe target** (`Trap26_StaticInitOrder_unsafe`, `RUN_UNSAFE_EXAMPLE`) — reads the cross-TU copy made during startup.

## Why it fails

The order between `global_configuration` in `config.cpp` and `copied_during_static_initialization` in `dependent.cpp` is unspecified. A build may initialize `global_configuration` first and appear correct, or may evaluate the dependent initializer before the constructor has produced the intended value. Reading a not-yet-constructed object is the defect the pattern invites.

## Correct direction

```cpp
const Configuration& safe_configuration() {
    static const Configuration value;
    return value;
}

int main() {
    use(safe_configuration().value);
}
```

Prefer `constinit` when constant initialization is possible; otherwise use function-local statics to make the dependency explicit.

## Detection

| Tool | Result |
|---|---|
| Constructor breakpoints / startup trace | yes — shows which dynamic initializer ran first in this build |
| Link-order experiments | useful — can expose the dependency by changing the result |
| Sanitizers | no — unspecified initialization order is not an ASan/UBSan/TSan pattern |
| Compiler/linker | generally no — both translation units are individually well formed |

## Next

- [`debug_analysis.md`](debug_analysis.md) — CDB/WinDbg session that observes the startup initializer order.
- [`../../TRAP_GUIDE.md`](../../TRAP_GUIDE.md) — row 26.
