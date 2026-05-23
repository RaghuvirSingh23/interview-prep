# Bash & Python Scripting for DevOps

Deep dive into shell and Python automation for NVIDIA compiler team interview. Covers production-grade scripting patterns for build infrastructure.

---

## Table of Contents

1. [Bash Scripting](#1-bash-scripting)
2. [Python for DevOps](#2-python-for-devops)
3. [Bash vs Python](#3-bash-vs-python)
4. [Hands-On Exercises](#4-hands-on-exercises)
5. [Interview Questions](#5-interview-questions)

---

## 1. Bash Scripting

### 1.1 Basics: Shebang, Variables, Quoting

```bash
#!/usr/bin/env bash
# env finds bash on PATH — more portable than /bin/bash

NAME="build"
VERSION=1.2.3          # no spaces around =
readonly BUILD_ID="abc" # cannot be reassigned
declare -i COUNT=0     # integer arithmetic mode
```

**Quoting is critical:**

| Style | Behavior |
|-------|----------|
| `$var` (unquoted) | Word splitting + glob expansion (dangerous) |
| `"$var"` | Variable expansion, no word splitting (safe) |
| `'$var'` | Literal string, nothing expanded |

```bash
MSG="hello world"
echo $MSG       # works here, but unsafe in general
echo "$MSG"     # always prefer this

FILES="*.log"
echo $FILES     # expands to matching files! (usually wrong)
echo "$FILES"   # literal *.log
```

**Interview tip:** Explain why `"$var"` is safer than `$var` — prevents word splitting on spaces and accidental glob expansion.

### 1.2 Conditionals

```bash
# [[ ]] is Bash-specific, safer, supports patterns and regex
if [[ -f "build.log" ]]; then
    echo "log exists"
elif [[ -d "out" ]]; then
    echo "out is directory"
fi

# Numeric comparison
if [[ $COUNT -gt 0 ]]; then echo "positive"; fi

# String comparison
if [[ "$BRANCH" == "main" ]]; then echo "mainline"; fi

# Glob pattern matching (only in [[ ]])
if [[ "$FILE" == *.c ]]; then echo "C source"; fi

# Regex
if [[ "$TAG" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then echo "semver"; fi
```

**`[` (POSIX) vs `[[` (Bash):** `[[` handles empty vars safely, supports `&&`/`||` inside, and does pattern matching. Use `[` only when strict POSIX `sh` compatibility is required.

### 1.3 Loops

```bash
# for over words
for f in *.o; do
    [[ -e "$f" ]] || continue   # handle no-match case
    echo "$f"
done

# Brace expansion
for i in {1..5}; do echo "$i"; done

# C-style for
for ((i=0; i<10; i++)); do echo "$i"; done

# while (reading lines safely)
while IFS= read -r line; do
    echo "$line"
done < build.log

# until (wait for condition)
until [[ -f "done.marker" ]]; do sleep 1; done
```

### 1.4 Functions

```bash
log_info() {
    printf '[%s] INFO: %s\n' "$(date -Iseconds)" "$*"
}

process_file() {
    local path="$1"                  # local scope
    local -r basename="${path##*/}"  # read-only local
    echo "$basename"
}

# Return values: only 0-255 integers. Use stdout for data.
get_cpu_count() {
    nproc
}
cpus=$(get_cpu_count)
```

### 1.5 String Manipulation (Parameter Expansion)

No external tools needed:

```bash
s="hello-world-build"

echo "${#s}"                  # 17 (length)
echo "${s:6}"                 # world-build (substring from offset 6)
echo "${s:6:5}"               # world (substring with length)

echo "${s#hello-}"            # world-build (remove shortest prefix)
echo "${s##*-}"               # build (remove longest prefix)
echo "${s%-build}"            # hello-world (remove shortest suffix)
echo "${s%%-*}"               # hello (remove longest suffix)

echo "${s/world/earth}"       # hello-earth-build (first replacement)
echo "${s//-/_}"              # hello_world_build (all replacements)

echo "${UNDEFINED:-default}"  # use default if unset
: "${CACHE_DIR:=/tmp/cache}"  # set if unset

echo "${s^^}"                 # HELLO-WORLD-BUILD (uppercase, Bash 4+)
```

### 1.6 Arrays

```bash
# Indexed array
files=(main.c utils.c)
files+=("plugin.c")
echo "${files[0]}"        # main.c
echo "${files[@]}"        # all elements
echo "${#files[@]}"       # count: 3

for f in "${files[@]}"; do echo "$f"; done

# Associative array (Bash 4+)
declare -A artifacts=(
    [linux-x64]="app.tar.gz"
    [linux-arm64]="app-arm.tar.gz"
)
echo "${artifacts[linux-x64]}"
for key in "${!artifacts[@]}"; do
    echo "$key -> ${artifacts[$key]}"
done
```

### 1.7 Command Substitution

```bash
now=$(date -u +%Y%m%d%H%M%S)           # preferred: nests cleanly
path=$(dirname "$(readlink -f "$0")")   # nested example

now=`date -u +%Y%m%d%H%M%S`            # legacy backtick (avoid)
```

Always use `$()`.

### 1.8 Exit Codes and Error Handling

```bash
# 0 = success, non-zero = failure
# $? = last exit code
# ${PIPESTATUS[@]} = array of exit codes for each pipe stage

# Strict mode (use in every CI script)
#!/usr/bin/env bash
set -euo pipefail
# -e: exit on first failing command
# -u: treat unset variables as error
# -o pipefail: pipeline fails if ANY stage fails
```

**Caveats with `set -e`:** Does NOT trigger in `if cmd;`, `while cmd;`, `cmd || true`, `! cmd`. For critical steps:

```bash
compile || { echo "compile failed"; exit 1; }
```

**`trap` for cleanup:**

```bash
cleanup() {
    rm -rf /tmp/build.$$
}
trap cleanup EXIT
trap 'echo interrupted; exit 130' INT TERM
```

### 1.9 Redirection

```bash
command > file          # stdout overwrite
command >> file         # stdout append
command 2> err.log      # stderr to file
command &> all.log      # stdout+stderr (Bash)
command > out 2>&1      # POSIX merge stderr into stdout
command > /dev/null 2>&1  # discard all

# Here-document
cat <<EOF
BUILD_ID=$BUILD_ID
EOF

# Here-document without expansion
cat <<'EOF'
Literal $BUILD_ID
EOF

# Here-string
grep pattern <<<"$VAR"
```

### 1.10 Pipes and Process Substitution

```bash
# Process substitution: treats command output as a file
diff <(sort a.txt) <(sort b.txt)

# Feed find output without subshell pitfalls
while read -r x; do echo "$x"; done < <(find . -name '*.c')
```

### 1.11 File Tests

```bash
[[ -e path ]]   # exists
[[ -f path ]]   # regular file
[[ -d path ]]   # directory
[[ -r path ]]   # readable
[[ -w path ]]   # writable
[[ -x path ]]   # executable
[[ -s path ]]   # exists and size > 0
[[ f1 -nt f2 ]] # f1 newer than f2
```

### 1.12 Text Processing: grep, sed, awk

```bash
# grep
grep -E 'error|warning' build.log        # extended regex
grep -RIn --include='*.c' 'TODO' src/     # recursive, line numbers, skip binary
grep -c error build.log                   # count matching lines

# sed
sed -n '10,20p' file                      # print lines 10-20
sed 's/foo/bar/g' file                    # substitute
sed -i.bak 's/^VERSION=.*/VERSION=2.0/' config.mk  # in-place with backup

# awk
awk '{print $1, $NF}' build.log          # first and last field
awk -F: '$1 ~ /error/ {c++} END {print c}' log  # count errors
awk '/START/,/END/' trace.txt             # print range between markers
```

### 1.13 find and xargs

```bash
# Safe deletion (handles spaces in filenames)
find . -name '*.o' -print0 | xargs -0 rm -f

# Parallel execution
find tests -name 'test_*.sh' -print0 | xargs -0 -P 8 -I {} bash {}

# Never parse ls output. Use find or globs.
```

### 1.14 Subshells, Background, wait

```bash
( cd /tmp && make )     # subshell: cd doesn't affect parent
                        # inherits copy of env; export in subshell doesn't leak

long_task &             # background
pid=$!
other_work
wait "$pid" || exit 1   # wait for background task
```

### 1.15 Build Script Patterns

**Parsing command-line args with `getopts`:**

```bash
usage() { echo "Usage: $0 [-v] [-o outdir] target"; }

verbose=0
outdir=""
while getopts ":vo:" opt; do
    case "$opt" in
        v) verbose=1 ;;
        o) outdir="$OPTARG" ;;
        *) usage; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
target="${1:-all}"
```

**Logging function:**

```bash
LOG_LEVEL="${LOG_LEVEL:-INFO}"
log() {
    local level="$1"; shift
    printf '[%s] %s: %s\n' "$(date -Iseconds)" "$level" "$*"
}
log INFO "Starting build"
log ERROR "Failed" >&2
```

**Retry with backoff:**

```bash
retry() {
    local n=0 max=5 delay=2
    until "$@"; do
        ((n++))
        [[ $n -ge $max ]] && return 1
        sleep "$delay"
    done
}
retry curl -fsS "$URL"
```

**Lock file (prevent concurrent builds):**

```bash
exec 200>/var/lock/mybuild.lock
flock 200 || { echo "Another build running"; exit 1; }
```

**Parallel execution with status tracking:**

```bash
pids=()
for t in "${targets[@]}"; do
    ( build_one "$t" ) &
    pids+=($!)
done
status=0
for p in "${pids[@]}"; do
    wait "$p" || status=1
done
exit "$status"
```

---

## 2. Python for DevOps

### 2.1 Files and Directories

```python
from pathlib import Path

root = Path("build/out")
root.mkdir(parents=True, exist_ok=True)

for py in Path("src").rglob("*.py"):
    text = py.read_text(encoding="utf-8")

if (root / "artifact.zip").exists():
    size = (root / "artifact.zip").stat().st_size
```

```python
import shutil

shutil.copy2("src/a.txt", "dst/a.txt")       # preserves metadata
shutil.copytree("templates", "out/templates", dirs_exist_ok=True)
shutil.rmtree("tmp_build", ignore_errors=False)
shutil.which("cmake")                         # find executable on PATH
```

```python
import os
os.environ.get("BUILD_NUMBER", "0")
```

### 2.2 subprocess

**`run` (most common):**

```python
import subprocess

result = subprocess.run(
    ["cmake", "--build", "build", "--target", "install"],
    check=True,          # raise CalledProcessError on non-zero exit
    capture_output=True, # capture stdout and stderr
    text=True,           # decode as string
)
print(result.stdout)
```

**`Popen` (streaming output):**

```python
p = subprocess.Popen(
    ["make", "-j8"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,  # merge stderr into stdout
    text=True,
)
for line in p.stdout:
    print(line, end="")
code = p.wait()
```

**`check_output` (simple capture):**

```python
sha = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
```

**Best practices:**
- Prefer list args over `shell=True` (avoids injection)
- Pass custom env: `env={**os.environ, "CC": "clang"}`
- Always set `timeout` for CI scripts

### 2.3 JSON and YAML

```python
import json
from pathlib import Path

data = json.loads(Path("manifest.json").read_text())
Path("out.json").write_text(json.dumps(data, indent=2))
```

```python
import yaml  # pip install pyyaml

cfg = yaml.safe_load(Path("ci.yaml").read_text())
# ALWAYS use safe_load, never yaml.load on untrusted input
```

### 2.4 Regular Expressions

```python
import re

log = Path("build.log").read_text(errors="replace")
errors = re.findall(r"error:\s*(.+)", log, flags=re.IGNORECASE | re.MULTILINE)

m = re.search(r"BUILD (\d+) finished in ([\d.]+)s", log)
if m:
    build_id, seconds = m.groups()
```

### 2.5 argparse (CLI tools)

```python
import argparse
from pathlib import Path

def main():
    p = argparse.ArgumentParser(description="Upload build artifacts")
    p.add_argument("path", type=Path, help="File or directory to upload")
    p.add_argument("-n", "--name", required=True)
    p.add_argument("--dry-run", action="store_true")
    args = p.parse_args()
    if args.dry_run:
        print(f"Would upload {args.path} as {args.name}")

if __name__ == "__main__":
    main()
```

### 2.6 Logging

```python
import logging

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
log = logging.getLogger("build")

log.info("Starting build")
try:
    1 / 0
except ZeroDivisionError:
    log.exception("unexpected")  # includes traceback
```

For libraries: `logging.getLogger(__name__)` and let the application configure handlers.

### 2.7 HTTP with requests

```python
import requests

# GET
r = requests.get("https://api.example.com/v1/status", timeout=30)
r.raise_for_status()
data = r.json()

# POST with file upload
r = requests.post(
    "https://api.example.com/v1/artifacts",
    headers={"Authorization": f"Bearer {token}"},
    files={"file": open("out.zip", "rb")},
    data={"repo": "driver", "build": "12345"},
    timeout=60,
)
```

Always set `timeout`. Handle `requests.exceptions.RequestException`.

### 2.8 Working with APIs

**Jenkins API (trigger build):**

```python
import requests
from requests.auth import HTTPBasicAuth

s = requests.Session()
s.auth = HTTPBasicAuth("user", "token")
base = "https://jenkins.example.com"

# Get CSRF crumb (required for POST)
crumb = s.get(f"{base}/crumbIssuer/api/json").json()
headers = {crumb["crumbRequestField"]: crumb["crumb"]}

# Trigger parameterized build
r = s.post(
    f"{base}/job/compiler-build/buildWithParameters",
    params={"BRANCH": "main"},
    headers=headers,
    timeout=60,
)
r.raise_for_status()
```

**Artifactory API (upload):**

```python
url = f"{artifactory_url}/artifactory/{repo}/{path_in_repo}"
r = requests.put(
    url,
    data=open(local_path, "rb"),
    headers={"Authorization": f"Bearer {token}"},
    timeout=300,
)
r.raise_for_status()
```

### 2.9 Jinja2 (Templating)

```python
from jinja2 import Environment, FileSystemLoader, StrictUndefined

env = Environment(
    loader=FileSystemLoader("templates"),
    undefined=StrictUndefined,  # catch typos in template vars
    trim_blocks=True,
    lstrip_blocks=True,
)
tpl = env.get_template("CMakeLists.txt.j2")
output = tpl.render(PROJECT="mylib", SOURCES=["a.c", "b.c"])
Path("CMakeLists.txt").write_text(output)
```

### 2.10 Python Packaging

**Modern (`pyproject.toml`):**

```toml
[build-system]
requires = ["setuptools>=61"]
build-backend = "setuptools.build_meta"

[project]
name = "ci-tools"
version = "0.1.0"
dependencies = ["requests", "pyyaml", "jinja2"]

[project.scripts]
ci-upload = "ci_tools.upload:main"
```

### 2.11 Common DevOps Script Patterns

**Build log parser:**

```python
def parse_build_log(path: Path) -> dict:
    text = path.read_text(errors="replace")
    return {
        "errors": len(re.findall(r"\berror\b", text, re.I)),
        "warnings": len(re.findall(r"\bwarning\b", text, re.I)),
        "failed_targets": re.findall(r"^\*\*\* \[([^\]]+)\] Error", text, re.M),
    }
```

**Upload with retry:**

```python
def upload_with_retry(session, url, path, max_attempts=3):
    for attempt in range(1, max_attempts + 1):
        try:
            r = session.put(url, data=path.read_bytes(), timeout=300)
            r.raise_for_status()
            return
        except requests.RequestException as e:
            if attempt == max_attempts:
                raise
            log.warning("Upload failed: %s, retrying...", e)
```

**JUnit XML aggregator:**

```python
import xml.etree.ElementTree as ET

def aggregate_junit(paths: list[Path]) -> tuple[int, int, int]:
    total = failures = errors = 0
    for p in paths:
        for suite in ET.parse(p).getroot().iter("testsuite"):
            total += int(suite.attrib.get("tests", 0))
            failures += int(suite.attrib.get("failures", 0))
            errors += int(suite.attrib.get("errors", 0))
    return total, failures, errors
```

---

## 3. Bash vs Python

| Use Bash when | Use Python when |
|---------------|-----------------|
| Gluing CLI tools (make, cmake, docker) with pipes | Parsing complex formats (JSON, YAML, XML) |
| Script < ~100 lines, mostly subprocess calls | Need structured types, classes, unit tests |
| Tight shell integration (`source`, env vars) | REST APIs, retries, auth, structured logging |
| Maximum portability on minimal Unix images | Cross-platform (Windows agents) |
| Inline CI steps | Shared libraries reused across pipelines |

**Hybrid pattern:** Bash entrypoint that `exec`s Python for complex logic. Or Python using `subprocess` for native CLI tools.

---

## 4. Hands-On Exercises

### Exercise 1: Safe Build Script (Bash)

Write a script that:
1. Accepts `-c` flag to clean `build/` if it exists
2. Runs `cmake -S . -B build && cmake --build build -j$(nproc)`
3. Uses `set -euo pipefail`
4. Logs each phase with timestamps
5. Traps EXIT for cleanup

### Exercise 2: Error Grep (Bash)

Parse `build.log` and print unique file paths that appear after `error:` using grep + sort + cut.

### Exercise 3: Ninja Wrapper (Python)

`argparse` CLI: `-C builddir`, optional `-t target`. Passes through to `ninja`, streams output, exits with ninja's return code.

### Exercise 4: Jenkins Build Trigger + Poll (Python)

Using `requests`:
1. Trigger a parameterized build (handle CSRF crumb)
2. Poll queue → build API every 10s until result is SUCCESS/FAILURE
3. Print final build URL

### Exercise 5: CMake Template Generator (Python)

Jinja2 template that generates `add_library()` from a list of sources and sets `target_compile_features(... cxx_std_17)`.

---

## 5. Interview Questions

**Q1: What is `#!/usr/bin/env bash` and why not `/bin/bash`?**
`env bash` finds Bash on PATH — more portable across systems (macOS, containers, NixOS). `/bin/bash` hardcodes a path that may not exist.

**Q2: Why is `set -e` not enough for robust scripts?**
Doesn't exit in `if cmd;`, `cmd || true`, `! cmd`. Combine with `pipefail`, explicit error checks, and `trap` for cleanup.

**Q3: `[[` vs `[`?**
`[[` is Bash-specific: handles empty vars safely, supports patterns/regex, allows `&&`/`||` inside. Use `[` for strict POSIX sh compatibility only.

**Q4: How to handle filenames with spaces?**
Quote variables (`"$f"`), use `IFS= read -r`, `find -print0 | xargs -0`. Never parse `ls` output.

**Q5: What does `2>&1` mean?**
Redirect fd 2 (stderr) to wherever fd 1 (stdout) currently points. Order matters: `>file 2>&1` sends both to file.

**Q6: Write a retry function in Bash.**
Loop with counter, sleep between attempts, return last exit code or 0 on success. (See §1.15)

**Q7: What is process substitution?**
`<(cmd)` creates a named pipe/fd that command output can be read from. Example: `diff <(sort a) <(sort b)` without temp files.

**Q8: How does `flock` prevent concurrent builds?**
Advisory lock on a file descriptor. Second process blocks or fails depending on flags, preventing two writers from corrupting shared output.

**Q9: Why avoid `shell=True` in Python subprocess?**
Injection risk if any argument contains user input. Harder to quote correctly. Use argument lists instead.

**Q10: `subprocess.run` vs `Popen`?**
`run` is high-level: waits, optional check, captures output. `Popen` gives control over streaming, background, custom I/O.

**Q11: How to stream build output to log AND console?**
`Popen` with `stdout=PIPE`, read lines, write to both file and stdout. Or shell `make 2>&1 | tee build.log`.

**Q12: Why `yaml.safe_load`?**
`yaml.load` can instantiate arbitrary Python objects (code execution risk). `safe_load` restricts to plain data types.

**Q13: How to authenticate REST APIs in CI?**
Short-lived tokens from OIDC/Vault. `Authorization: Bearer` header. TLS always. Mask secrets in logs. Never commit tokens to repo.

**Q14: What is a Jenkins crumb?**
CSRF token. Jenkins requires it for POST requests. Fetch from `crumbIssuer/api/json` and send as header.

**Q15: Write a script that finds stale .o files (source newer than object).**

```bash
#!/usr/bin/env bash
set -euo pipefail
while IFS= read -r -d '' cfile; do
    ofile="${cfile%.c}.o"
    [[ -f "$ofile" ]] || continue
    [[ "$cfile" -nt "$ofile" ]] && echo "$ofile"
done < <(find . -name '*.c' -print0)
```

**Q16: Parallel compilation in Python?**

```python
from concurrent.futures import ProcessPoolExecutor, as_completed
import subprocess
from pathlib import Path

def compile_one(src: Path) -> str:
    obj = src.with_suffix(".o")
    subprocess.run(["gcc", "-c", str(src), "-o", str(obj)], check=True)
    return str(obj)

with ProcessPoolExecutor() as pool:
    futures = [pool.submit(compile_one, s) for s in Path(".").rglob("*.c")]
    for f in as_completed(futures):
        print("Built", f.result())
```

Use `ProcessPoolExecutor` for CPU-bound work (compilation), not `ThreadPoolExecutor`.

**Q17: How to make a Bash script debuggable in CI?**
`set -x` for trace output. `printf` with timestamps. Save logs as artifacts. Use `bash -x script.sh` for one-off debugging.

**Q18: `exec` in Bash vs Python?**
Bash `exec cmd` replaces the shell process (same PID) — useful for handing off to Python. Python `os.exec*` replaces current process; no return.

**Q19: What can go wrong uploading large binaries to Artifactory?**
Timeouts, reverse proxy body size limits, checksum mismatch on retry, wrong repo path, token scope. Mitigate with retries, backoff, and verifying HTTP 201/204.

**Q20: `$()` vs backticks?**
Same semantics (capture stdout), but `$()` nests cleanly and is more readable. Always prefer `$()`.
