# Resume Deep Dive — Cisco Points

This doc breaks down each Cisco resume bullet so you can explain any part of it
fluently in an interview. Each section covers: what it means, why it matters,
how you'd describe it, and the follow-up questions you'll get.

---

## Point 1: GMake → Bazel Migration

> "Migrated the build system from GMake to Bazel, handling dependency graphs
> and toolchain configurations across a large scale C/C++ codebase."

### Why This Migration Happens

GMake problems at scale:

```
Problem                        │ What goes wrong
───────────────────────────────┼──────────────────────────────────────────
Recursive Make                 │ Each sub-directory gets its own make
                               │ process. The global dependency graph is
                               │ fragmented — parallel builds can race or
                               │ miss dependencies.
───────────────────────────────┼──────────────────────────────────────────
No caching                     │ CI rebuilds everything from scratch on
                               │ every run. A 100k LOC C++ project might
                               │ take 30+ minutes per pipeline.
───────────────────────────────┼──────────────────────────────────────────
Implicit dependencies          │ Developers forget to list a header dep.
                               │ Build works on their machine (because the
                               │ .o is still there) but breaks on clean CI.
───────────────────────────────┼──────────────────────────────────────────
No hermetic builds             │ Make calls whatever g++ is on PATH. Two
                               │ machines with different GCC versions
                               │ produce different (or broken) binaries.
───────────────────────────────┼──────────────────────────────────────────
Makefile spaghetti             │ Over the years, Makefiles accumulate
                               │ hacks — conditional includes, shell
                               │ commands, eval tricks. Nobody wants to
                               │ touch them.
```

What Bazel solves:

```
Bazel advantage                │ How
───────────────────────────────┼──────────────────────────────────────────
Correct incremental builds     │ Every target declares ALL its deps
                               │ explicitly. Bazel sandboxes each action
                               │ so it can't accidentally read files it
                               │ didn't declare.
───────────────────────────────┼──────────────────────────────────────────
Remote caching                 │ Build outputs are content-addressed. If
                               │ the inputs didn't change, fetch the
                               │ cached .o from a shared cache instead of
                               │ recompiling. CI goes from 30 min to 3 min.
───────────────────────────────┼──────────────────────────────────────────
Hermetic toolchains            │ Bazel downloads the exact compiler
                               │ version specified in the WORKSPACE/MODULE.
                               │ Every machine uses the same toolchain.
───────────────────────────────┼──────────────────────────────────────────
Remote execution               │ Farm out build actions to a cluster of
                               │ build machines. Massive parallelism.
───────────────────────────────┼──────────────────────────────────────────
Language-agnostic              │ Same build system for C++, Python,
                               │ protobuf, Go — everything in one graph.
```

### How the Migration Works Step by Step

A real large-scale migration is NOT "rewrite all Makefiles in one PR". It's
incremental. Here's the phased approach:

#### Phase 0: Audit the existing build

Before touching anything, understand what you have:

```bash
# How many Makefiles?
find . -name "Makefile" -o -name "*.mk" | wc -l

# How many source files?
find . -name "*.cpp" -o -name "*.c" -o -name "*.h" | wc -l

# What are the top-level targets?
grep -r "^[a-zA-Z_]*:" Makefile | head -30

# What toolchains are used?
grep -r "CXX\|CC\|AR\|LD" Makefile | sort -u
```

Map out the dependency graph of libraries and binaries. Draw it on paper.

#### Phase 1: Get Bazel to build alongside Make

You DON'T remove Makefiles yet. Both build systems coexist.

```
project/
├── Makefile              ← still works, still used by CI
├── MODULE.bazel          ← NEW: Bazel module definition
├── .bazelrc              ← NEW: Bazel config flags
├── toolchain/
│   └── BUILD             ← NEW: hermetic toolchain definition
├── src/
│   ├── Makefile           ← still there
│   ├── BUILD              ← NEW: Bazel build rules
│   ├── core/
│   │   ├── Makefile
│   │   ├── BUILD          ← NEW
│   │   ├── engine.cpp
│   │   └── engine.h
│   └── utils/
│       ├── Makefile
│       ├── BUILD          ← NEW
│       ├── logger.cpp
│       └── logger.h
└── tests/
    ├── Makefile
    ├── BUILD              ← NEW
    └── test_engine.cpp
```

Start with leaf libraries (things with no internal dependencies) and work
inward.

#### Phase 2: Migrate leaf libraries first

Take a simple GMake target:

**BEFORE — Makefile (src/utils/Makefile):**

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -fPIC

OBJ = logger.o config.o
LIB = libutils.a

$(LIB): $(OBJ)
	ar rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I../../include -c $< -o $@

clean:
	rm -f $(OBJ) $(LIB)
```

**AFTER — BUILD (src/utils/BUILD):**

```python
cc_library(
    name = "utils",
    srcs = ["logger.cpp", "config.cpp"],
    hdrs = ["logger.h", "config.h"],
    visibility = ["//visibility:public"],
)
```

That's it. A few lines of declarative code replaces 12 lines of imperative
Makefile. Key differences:

- `srcs` = source files (what gets compiled)
- `hdrs` = headers (what consumers can #include)
- `visibility` = who is allowed to depend on this target
- No flags — those come from the toolchain, configured once globally
- No manual `ar rcs` — Bazel knows how to build a cc_library

Verify:

```bash
# Build just this library
bazel build //src/utils:utils

# Check it produces the same thing
file bazel-bin/src/utils/libutils.a
```

#### Phase 3: Migrate targets that depend on migrated libraries

Now migrate something that depends on utils:

**BEFORE — Makefile (src/core/Makefile):**

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall

OBJ = engine.o
LIB = libcore.a

$(LIB): $(OBJ)
	ar rcs $@ $^

engine.o: engine.cpp engine.h ../utils/logger.h
	$(CXX) $(CXXFLAGS) -I../utils -c $< -o $@
```

**AFTER — BUILD (src/core/BUILD):**

```python
cc_library(
    name = "core",
    srcs = ["engine.cpp"],
    hdrs = ["engine.h"],
    deps = ["//src/utils:utils"],     # ← dependency on the migrated library
    visibility = ["//visibility:public"],
)
```

Notice: no `-I../utils`. Bazel figures out include paths from the `deps`
declaration. When `core` depends on `utils`, it automatically gets access to
the `hdrs` listed in `utils`.

#### Phase 4: Migrate binaries and tests

**BEFORE — Makefile (tests/Makefile):**

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I../src/core -I../src/utils

test_engine: test_engine.o ../src/core/libcore.a ../src/utils/libutils.a
	$(CXX) -o $@ $^

test_engine.o: test_engine.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: test_engine
	./test_engine
```

**AFTER — BUILD (tests/BUILD):**

```python
cc_test(
    name = "test_engine",
    srcs = ["test_engine.cpp"],
    deps = [
        "//src/core:core",
        "//src/utils:utils",
    ],
)
```

Run:

```bash
bazel test //tests:test_engine
```

Bazel will compile only what's needed, run the test, cache the result, and
skip it next time if nothing changed.

#### Phase 5: Toolchain configuration

This is the "toolchain configurations" part of your resume bullet. In Make,
the compiler is whatever `g++` or `$(CXX)` points to on the machine. In Bazel,
you define it explicitly:

**.bazelrc:**

```
build --cxxopt=-std=c++17
build --cxxopt=-Wall
build --cxxopt=-Wextra

# Use a specific GCC toolchain (not whatever's on PATH)
build --crosstool_top=//toolchain:gcc_suite
```

**toolchain/BUILD:**

```python
cc_toolchain_suite(
    name = "gcc_suite",
    toolchains = {
        "k8": ":gcc_12_toolchain",      # x86_64 Linux
        "aarch64": ":gcc_12_aarch64",    # ARM cross-compile
    },
)

cc_toolchain(
    name = "gcc_12_toolchain",
    all_files = ":gcc_12_all",
    compiler_files = ":gcc_12_compiler",
    linker_files = ":gcc_12_linker",
    # ... paths to the actual compiler binaries
)
```

This means every developer and every CI machine uses the exact same GCC 12,
regardless of what's installed on the system.

#### Phase 6: Switch CI to Bazel, remove Makefiles

Once all targets build with Bazel and pass tests:

1. Update Jenkins/CI to call `bazel build //...` and `bazel test //...`
2. Enable remote caching (point to a shared cache server)
3. Remove all Makefiles in a final cleanup PR
4. Update developer docs

### The Dependency Graph Problem

In an interview, they'll likely ask: "How did you handle the dependency graph?"

With Make, dependencies are a mix of:

- What's written in the Makefile (manual, often incomplete)
- What the compiler discovers via `-MMD` (auto-generated .d files)
- What Make infers from implicit rules

With Bazel, everything is explicit in `deps`. If you forget a dependency, the
build fails immediately (because of sandboxing). This is GOOD — it means the
dependency graph is always accurate.

During migration, the hard part is figuring out the REAL dependency graph from
the Makefiles. Tools that help:

```bash
# Generate a compile_commands.json from Make
bear -- make -j$(nproc)

# Analyze includes to find actual dependencies
include-what-you-use <source_file>
```

### Interview Follow-up Questions You'll Get

**Q: How big was the codebase?**
Be specific: "~X thousand source files across Y components, roughly Z
lines of C/C++."

**Q: How long did the migration take?**
"We did it incrementally over N months. Both build systems coexisted during
migration. We migrated leaf libraries first, then worked inward."

**Q: Did you use any automation to generate BUILD files?**
"We used Gazelle/buildifier for formatting, and wrote a Python script to
parse existing Makefiles and generate initial BUILD file skeletons that we
then reviewed and corrected manually."

**Q: What was the hardest part?**
Good answers:

- "Untangling circular dependencies in the Makefiles that happened to work
due to recursive Make but are illegal in Bazel."
- "Getting the hermetic toolchain right — the codebase had assumptions
about system headers being in specific paths."
- "Convincing the team to change workflows — `bazel build //...` instead
of `make -C subdir`."

**Q: What speedup did you get?**
"Clean builds stayed about the same, but incremental builds dropped from
X minutes to Y seconds because of Bazel's content-addressed caching.
CI pipelines with remote cache went from X to Y minutes."

**Q: What about external dependencies (e.g., Boost, OpenSSL)?**
"We used Bazel's `http_archive` rule to download pinned versions, or
`rules_foreign_cc` to build CMake/Make-based external deps under Bazel."

Example:

```python
# MODULE.bazel
bazel_dep(name = "rules_foreign_cc", version = "0.10.1")

# third_party/BUILD
load("@rules_foreign_cc//foreign_cc:cmake.bzl", "cmake")

cmake(
    name = "openssl",
    lib_source = "@openssl_src//:all_srcs",
    out_static_libs = ["libssl.a", "libcrypto.a"],
    visibility = ["//visibility:public"],
)
```

---

## Point 2: Image Build & Packaging Workflow

> "Worked on the image build and packaging workflow, producing bootable system
> images and managing artifact promotion across dev, staging, and release
> channels with versioned traceability."

### What This Actually Means

At Cisco you're building **network devices** — routers, switches, firewalls.
These aren't normal apps. The final deliverable is a complete **bootable system
image** — a binary blob that gets flashed onto hardware and contains:

```
┌─────────────────────────────────────────────┐
│              System Image (.bin)            │
├─────────────────────────────────────────────┤
│  Bootloader (GRUB/U-Boot)                   │
│  Linux Kernel (customized)                  │
│  Root Filesystem                            │
│    ├── OS packages                          │
│    ├── Cisco IOS-XR / IOS-XE daemons        │
│    ├── Protocol stacks (BGP, OSPF, MPLS)    │
│    ├── Management plane (CLI, NETCONF)      │
│    └── Firmware blobs (FPGA, ASIC microcodes)│
│  Metadata (version, checksums, manifest)    │
└─────────────────────────────────────────────┘
```

This is NOT just "compile some C++ and ship a binary." It's:

1. Build dozens of components (kernel modules, daemons, protocol stacks)
2. Assemble them into a root filesystem
3. Overlay firmware, config, and metadata
4. Package into a flashable image with checksums
5. Sign the image (secure boot)

### The Packaging Workflow

```
Source code (Git)
       │
       ▼
┌─────────────┐
│  Component  │  Individual C/C++ libraries and daemons
│  Builds     │  Each produces RPMs/DEBs or raw binaries
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Package    │  RPMs/DEBs collected into a package repository
│  Repository │  (like a private apt/yum repo or Artifactory)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Image      │  A build script assembles packages into a root
│  Assembly   │  filesystem, adds kernel, bootloader, firmware
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Signing &  │  Image is checksummed, signed with a private key
│  Validatio  │  for secure boot verification
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Artifact    │  Final .bin image is uploaded to Artifactory
│  Upload      │  with version, metadata, build ID
└─────────────┘
```

### Artifact Promotion

This is the "managing artifact promotion across dev, staging, and release
channels" part. Not every build goes to customers. Images flow through stages:

```
   ┌──────────┐    promote    ┌───────────┐    promote    ┌───────────┐
   │   dev    │──────────────▶│  staging  │──────────────▶│  release  │
   │ (nightly)│               │  (tested) │               │ (shipped) │
   └──────────┘               └───────────┘               └───────────┘

   Every commit                Passed sanity               Passed full
   builds here                 + regression                regression +
   automatically               test suites                 qualification
```

In Artifactory, these are different repositories:

```
cisco-images-dev/       ← all nightly builds land here
cisco-images-staging/   ← promoted after automated test pass
cisco-images-release/   ← promoted after manual QA sign-off
```

Promotion is NOT rebuilding. It's moving/copying the exact same artifact
(same checksum) to the next repo. This guarantees what you tested is what
you ship.

**Promotion via Artifactory API:**

```bash
curl -X POST \
  "https://artifactory.cisco.com/api/move/cisco-images-dev/image-7.4.2-b1234.bin" \
  -d '{"targetRepo": "cisco-images-staging"}' \
  -H "X-JFrog-Art-Api: $API_KEY"
```

**Or via Jenkins pipeline:**

```groovy
stage('Promote to Staging') {
    when { expression { currentBuild.result == 'SUCCESS' } }
    steps {
        script {
            def server = Artifactory.server 'cisco-artifactory'
            server.promote(
                targetRepo: 'cisco-images-staging',
                sourceRepo: 'cisco-images-dev',
                status: 'staging',
                comment: "Promoted by pipeline ${env.BUILD_URL}"
            )
        }
    }
}
```

### Versioned Traceability

Every image has a version like `7.4.2-b1234` where:

- `7.4.2` = release version (semver)
- `b1234` = CI build number

The image also carries metadata so you can always trace back:

```
Version:       7.4.2-b1234
Git Commit:    a1b2c3d4
Branch:        release/7.4
Build Date:    2025-01-15T08:30:00Z
Jenkins Job:   image-build-pipeline #1234
Components:    bgp-daemon-3.2.1, ospf-daemon-2.1.0, kernel-5.15.42
Checksum:      sha256:e3b0c44298fc1c149afb...
Signed By:     release-signing-key-2025
```

If a customer reports a bug on image 7.4.2-b1234, you can instantly find:

- Exact git commit
- Exact versions of every component inside
- The CI build that produced it
- Who promoted it and when

### Interview Follow-up Questions

**Q: What format were the images?**
Depends on the platform — could be ISO, raw disk images, squashfs, or
proprietary formats. Mention what you worked with.

**Q: How did you handle image size?**
"We stripped debug symbols for release images, used squashfs compression,
and split optional packages into installable add-ons instead of bundling
everything."

**Q: How long did an image build take?**
Be specific. "Full image from clean took ~X minutes. Incremental with
cached component RPMs took ~Y minutes."

**Q: How did you ensure reproducibility?**
"Every build pinned exact package versions in a manifest file. We used
Docker containers for the build environment so the host OS didn't matter.
The manifest + Dockerfile + git SHA fully determine the output."

**Q: What's the difference between a nightly and a release image?**
"Same image, same bits. The difference is how much testing it passed.
A nightly is built every night from the latest code. If it passes
automated tests, it gets promoted to staging. After full regression and
QA sign-off, it becomes a release candidate."

---

## Point 3: End-to-End CI/CD Pipelines

> "Architected end-to-end CI/CD pipelines using Jenkins and GitHub, automating
> the full lifecycle from code commit to image build, static analysis gates,
> sanity test execution, and firmware deployment onto target hardware."

### What "End-to-End" Means Here

This isn't a simple "build and test" pipeline. At Cisco, the pipeline takes
code all the way from a developer's commit to firmware running on a physical
router. Every stage is automated.

```
Developer pushes code
         │
         ▼
    ┌─────────┐
    │ GitHub  │  Webhook fires
    │ Webhook │──────────────────┐
    └─────────┘                  │
                                 ▼
                          ┌──────────────┐
                          │   Jenkins    │
                          │  Controller  │
                          └──────┬───────┘
                                 │
          ┌──────────┬───────────┼──────────┬──────────┐
          ▼          ▼           ▼          ▼          ▼
    ┌──────────┐┌──────────┐┌────────┐┌────────┐┌──────────┐
    │  Build   ││  Static  ││ Unit   ││ Image  ││ Deploy   │
    │  Stage   ││ Analysis ││ Tests  ││ Build  ││ to HW    │
    └──────────┘└──────────┘└────────┘└────────┘└──────────┘
```

### The Pipeline Stage by Stage

Here's a realistic Jenkinsfile for this kind of pipeline:

```groovy
pipeline {
    agent none
    options {
        timestamps()
        timeout(time: 4, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '50'))
    }

    parameters {
        string(name: 'BRANCH', defaultValue: 'main')
        booleanParam(name: 'DEPLOY_TO_HW', defaultValue: false)
    }

    environment {
        ARTIFACTORY_URL = 'https://artifactory.cisco.com'
        IMAGE_REPO      = 'cisco-images-dev'
    }

    stages {

        stage('Checkout') {
            agent { label 'lightweight' }
            steps {
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: params.BRANCH]],
                    userRemoteConfigs: [[
                        url: 'https://github.cisco.com/xr/platform.git',
                        credentialsId: 'github-token'
                    ]]
                ])
                stash name: 'source', includes: '**'
            }
        }

        stage('Build') {
            agent { label 'build-agent && linux && x86_64' }
            steps {
                unstash 'source'
                sh '''
                    bazel build //... \
                        --remote_cache=grpc://bazel-cache:9092 \
                        --jobs=32
                '''
            }
            post {
                success { stash name: 'build-output', includes: 'bazel-bin/**' }
            }
        }

        stage('Quality Gates') {
            parallel {

                stage('Static Analysis') {
                    agent { label 'analysis' }
                    steps {
                        unstash 'source'
                        sh '''
                            # Coverity / cppcheck / clang-tidy
                            cov-build --dir cov-int bazel build //...
                            cov-analyze --dir cov-int --all
                        '''
                    }
                    post {
                        always {
                            recordIssues tool: coverity(pattern: 'cov-int/**/*.json')
                        }
                        failure {
                            error 'Static analysis found critical defects'
                        }
                    }
                }

                stage('Unit Tests') {
                    agent { label 'build-agent' }
                    steps {
                        unstash 'source'
                        sh 'bazel test //... --test_output=errors'
                    }
                    post {
                        always { junit 'bazel-testlogs/**/test.xml' }
                    }
                }
            }
        }

        stage('Image Build') {
            agent { label 'image-builder' }
            steps {
                unstash 'source'
                unstash 'build-output'
                sh '''
                    ./scripts/assemble-image.sh \
                        --version ${BUILD_NUMBER} \
                        --output image-${BUILD_NUMBER}.bin
                '''
                sh "sha256sum image-${BUILD_NUMBER}.bin > image-${BUILD_NUMBER}.sha256"
            }
            post {
                success {
                    archiveArtifacts artifacts: 'image-*.bin, image-*.sha256'
                    sh """
                        curl -H 'X-JFrog-Art-Api:${ARTIFACTORY_CRED}' \
                             -T image-${BUILD_NUMBER}.bin \
                             '${ARTIFACTORY_URL}/${IMAGE_REPO}/image-${BUILD_NUMBER}.bin'
                    """
                }
            }
        }

        stage('Sanity Tests') {
            agent { label 'test-lab' }
            steps {
                sh """
                    # Flash image onto test hardware
                    ./scripts/flash-device.sh \
                        --image image-${BUILD_NUMBER}.bin \
                        --device lab-router-01

                    # Wait for device to boot
                    ./scripts/wait-for-boot.sh --device lab-router-01 --timeout 300

                    # Run sanity suite (basic connectivity, CLI, process health)
                    pyats run job sanity_job.py \
                        --testbed testbed.yaml \
                        --device lab-router-01
                """
            }
            post {
                always { junit 'sanity-results/**/*.xml' }
            }
        }

        stage('Deploy to Target HW') {
            when { expression { params.DEPLOY_TO_HW == true } }
            agent { label 'deploy' }
            steps {
                input message: 'Approve deployment to staging hardware?',
                      submitter: 'build-team'
                sh """
                    ./scripts/deploy.sh \
                        --image image-${BUILD_NUMBER}.bin \
                        --targets staging-fleet.yaml
                """
            }
        }
    }

    post {
        failure {
            emailext(
                subject: "FAILED: ${env.JOB_NAME} #${env.BUILD_NUMBER}",
                body: "See ${env.BUILD_URL}console",
                recipientProviders: [[$class: 'DevelopersRecipientProvider']]
            )
        }
        success {
            slackSend(
                channel: '#build-status',
                message: "✓ ${env.JOB_NAME} #${env.BUILD_NUMBER} passed"
            )
        }
    }
}
```

### Breaking Down Each Stage

**1. Checkout + Stash**
Clones the code on a lightweight agent and `stash`es it. Other stages
`unstash` to get the source without re-cloning. This is efficient when
agents are on different machines.

**2. Build**
Runs `bazel build //...` (build everything). Uses remote cache so
unchanged targets are fetched instantly. The `--jobs=32` flag runs 32
compilation actions in parallel. Build output is stashed for later stages.

**3. Quality Gates (parallel)**
Two stages run simultaneously:

- **Static analysis**: tools like Coverity, cppcheck, or clang-tidy scan
the code for bugs without running it. If critical defects are found,
the pipeline fails (this is the "static analysis gate").
- **Unit tests**: `bazel test //...` runs all test targets. Results are
published as JUnit XML so Jenkins shows pass/fail in the UI.

Running these in parallel saves time — static analysis and tests don't
depend on each other.

**4. Image Build**
Assembles the compiled binaries into a flashable system image. Computes
a checksum. Uploads the image to Artifactory.

**5. Sanity Tests**
This is the "firmware deployment onto target hardware" part. The pipeline:

- Flashes the image onto a physical device in a test lab
- Waits for it to boot
- Runs pyATS (Cisco's test framework) to verify basic functionality
— interfaces come up, routing protocols start, CLI responds

**6. Deploy (gated)**
Only runs if explicitly requested (`DEPLOY_TO_HW` parameter). Requires
manual approval from the build team via Jenkins `input` step before
deploying to staging hardware.

### Key Concepts to Explain in Interview

**Webhook Trigger:**
GitHub sends an HTTP POST to Jenkins whenever code is pushed. Jenkins
starts the pipeline automatically — no polling, no cron.

**Agent Labels:**
`agent { label 'build-agent && linux && x86_64' }` — Jenkins picks a
machine that matches ALL those labels. Different stages can run on
different machines. Image builds might need a beefy machine. Sanity
tests need access to physical hardware in the test lab.

**Parallel Stages:**
`parallel { ... }` runs multiple stages at the same time on different
agents. Total pipeline time = max(stage_time) instead of sum(stage_time).

**stash/unstash:**
Transfers files between stages running on different agents. `stash` saves
files to Jenkins controller. `unstash` retrieves them on a different agent.

**Static Analysis Gate:**
A "gate" means the pipeline stops if this stage fails. You don't just
report warnings — you block the build if Coverity finds a critical defect
like buffer overflow, null dereference, or resource leak.

**input Step:**
Pauses the pipeline and waits for a human to click "Proceed" in the
Jenkins UI. Used for risky operations like deploying to staging/production.

### Interview Follow-up Questions

**Q: How did you handle flaky tests?**
"We had a retry mechanism — tests that failed were re-run once. If they
failed again, it was a real failure. We also tracked flaky test rates in
Kibana and had a weekly rotation to fix the top flaky tests."

**Q: How long did the full pipeline take?**
Be specific: "Build: ~X min, static analysis: ~Y min (parallel with
tests), image build: ~Z min, sanity: ~W min. Total ~T minutes end to end."

**Q: How did you handle multiple branches?**
"Multibranch pipeline in Jenkins. Every branch and PR gets its own
pipeline instance. PRs run build + unit tests only. The full pipeline
(image build + hardware tests) only runs on main and release branches."

**Q: What if the test lab hardware was busy?**
"We used Jenkins' lockable-resources plugin. Each device was a lockable
resource. If a device was in use, the pipeline queued until it was free.
We had multiple devices to parallelize across PRs."

**Q: How did you secure credentials?**
"Jenkins credentials store — no secrets in the Jenkinsfile. API tokens,
SSH keys, and Artifactory credentials are injected as environment
variables at runtime using `withCredentials {}` blocks."

**Q: How did you roll back a bad deployment?**
"Every image version is stored in Artifactory. Rolling back means
deploying the previous known-good image version. The deploy script
takes an image version as input, so it's just re-running the deploy
stage with an older build number."