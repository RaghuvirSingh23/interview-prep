# CI/CD, Jenkins & GitLab CI

Deep dive into continuous integration/delivery, Jenkins pipelines, and GitLab CI for NVIDIA compiler team interview.

---

## Table of Contents

1. [CI/CD Fundamentals](#1-cicd-fundamentals)
2. [Jenkins Deep Dive](#2-jenkins-deep-dive)
3. [GitLab CI/CD](#3-gitlab-cicd)
4. [CI/CD for Compiler Teams](#4-cicd-for-compiler-teams)
5. [Hands-On Exercises](#5-hands-on-exercises)
6. [Interview Questions](#6-interview-questions)

---

## 1. CI/CD Fundamentals

### What is CI (Continuous Integration)?

Every developer merges code into a shared mainline frequently (multiple times a day). Each merge triggers an automated build + test cycle.

```
Developer A ─── push ──┐
                       ├──▶ Shared Repo ──▶ Automated Build ──▶ Tests ──▶ Report
Developer B ─── push ──┘
```

**Why it matters for compiler teams:**

- Catch ABI breakage, codegen regressions, test failures early
- Keep the compiler always in a buildable state
- Fast feedback loop (minutes, not hours)

### What is CD (Continuous Delivery vs Deployment)?

```
                    ┌──────────────────────────────────┐
                    │        Continuous Delivery        │
                    │                                   │
  Code ──▶ Build ──▶ Test ──▶ Stage ──▶ [MANUAL GATE] ──▶ Production
                    │                                   │
                    └──────────────────────────────────┘

                    ┌──────────────────────────────────┐
                    │       Continuous Deployment       │
                    │                                   │
  Code ──▶ Build ──▶ Test ──▶ Stage ──▶ [AUTO] ──▶ Production
                    │                                   │
                    └──────────────────────────────────┘
```


| Aspect               | Continuous Delivery              | Continuous Deployment          |
| -------------------- | -------------------------------- | ------------------------------ |
| Deploy to production | Manual approval/gate             | Automatic after green pipeline |
| Risk                 | Controlled releases              | Needs strong tests + rollback  |
| Typical use          | Compilers, enterprise, regulated | Web SaaS, internal tools       |


**For compilers:** teams almost always use **delivery**, not deployment. Nightly builds go to internal users first, GA releases require qualification + hardware testing.

### Pipeline Stages

A pipeline is a sequence of stages. Each stage has jobs/steps.

```
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│  SOURCE  │──▶│  BUILD   │──▶│  TEST    │──▶│ PACKAGE  │──▶│ DEPLOY/  │
│ checkout │   │ compile  │   │ unit     │   │ tar/deb  │   │ PROMOTE  │
│ deps     │   │ link     │   │ integ    │   │ rpm/img  │   │          │
└──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
```

**Stages for a compiler CI pipeline specifically:**

1. **Prepare** — clone, submodules, toolchain setup, restore caches
2. **Build** — compile the compiler itself (cmake + ninja), cross-compile if needed
3. **Static Analysis** — clang-tidy, cppcheck, Coverity (can run in parallel with build)
4. **Test** — unit tests, lit tests, regression suites, self-hosting checks
5. **Package** — tarballs, installers, SDK bundles, Docker images
6. **Promote** — push to artifact server, trigger downstream pipelines

**Fail-fast principle:** Run cheap checks (lint, formatting) before expensive full builds.

---

## 2. Jenkins Deep Dive

### 2.1 Architecture

```
                    ┌──────────────────────────┐
                    │    Jenkins Controller    │
                    │  (UI, scheduling, queue, │
                    │   pipeline orchestration)│
                    └────────────┬─────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                   ▼
       ┌────────────┐    ┌────────────┐      ┌────────────┐
       │  Agent A   │    │  Agent B   │      │  Agent C   │
       │ Linux x86  │    │ Linux ARM  │      │ K8s Pod    │
       │ 4 executors│    │ 2 executors│      │ ephemeral  │
       └────────────┘    └────────────┘      └────────────┘
```

**Key terms:**


| Term           | What it is                                                                                  |
| -------------- | ------------------------------------------------------------------------------------------- |
| **Controller** | Runs the UI, stores config, schedules builds. Should NOT run heavy builds itself.           |
| **Agent**      | A machine (or container) that executes build steps. Connected via SSH, JNLP, or K8s plugin. |
| **Node**       | Logical machine registered with Jenkins (often interchangeable with "agent").               |
| **Executor**   | A slot on a node that runs one build at a time. 4 executors = 4 concurrent builds.          |
| **Label**      | Tags on nodes (e.g., `linux`, `gpu`, `cuda12`) used to route builds to the right machines.  |


**Important:** For compiler farms, you need many beefy agents (lots of RAM/CPU for compilation), not a beefy controller. The controller just orchestrates.

### 2.2 Declarative vs Scripted Pipeline

Jenkins has two pipeline syntaxes. Both are written in Groovy and stored as `Jenkinsfile` in the repo.

**Declarative Pipeline** — structured, opinionated, easier to read:

```groovy
pipeline {
    agent any

    options {
        timestamps()
        timeout(time: 2, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '30'))
    }

    environment {
        LLVM_DIR = "${WORKSPACE}/llvm-install"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build') {
            steps {
                sh '''
                    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
                    cmake --build build --parallel $(nproc)
                '''
            }
        }

        stage('Test') {
            steps {
                sh 'cd build && ctest --output-on-failure'
            }
        }
    }

    post {
        always  { junit 'build/**/*.xml' }
        failure { emailext subject: "FAILED: ${env.JOB_NAME} #${env.BUILD_NUMBER}",
                           body: "See ${env.BUILD_URL}",
                           recipientProviders: [[$class: 'DevelopersRecipientProvider']] }
    }
}
```

**Scripted Pipeline** — full Groovy, maximum flexibility:

```groovy
node('linux && llvm') {
    stage('Checkout') {
        checkout scm
    }

    stage('Build') {
        sh 'cmake -S . -B build && cmake --build build -j$(nproc)'
    }

    stage('Test') {
        sh 'cd build && ctest --output-on-failure'
    }
}
```

**When to use which:**


| Declarative                              | Scripted                                   |
| ---------------------------------------- | ------------------------------------------ |
| Standard build/test/deploy flows         | Dynamic stage generation at runtime        |
| Easier for code review and Blue Ocean UI | Complex conditional logic                  |
| Has built-in `post`, `when`, `matrix`    | When Declarative can't express your needs  |
| Prefer this by default                   | Use `script { }` blocks for escape hatches |


### 2.3 Stages, Steps, and Post Actions

**Stages** are logical phases shown in the Jenkins UI. **Steps** are individual actions within a stage.

**Post actions** run after all stages complete:

```groovy
post {
    always   { cleanWs() }                    // always runs
    success  { echo 'Green build!' }          // only on success
    failure  { slackSend channel: '#ci', message: "FAILED: ${env.JOB_NAME}" }
    unstable { echo 'Tests had failures but build completed' }
    changed  { echo 'State changed from previous build' }
}
```

Post conditions execute in order: `always` → `changed` → `fixed`/`regression` → `success`/`unstable`/`failure` → `cleanup`.

### 2.4 Parallel Stages

Run multiple builds concurrently (different platforms, compilers, configs):

```groovy
stage('Platform Builds') {
    parallel {
        stage('Linux GCC') {
            agent { label 'linux && gcc' }
            steps { sh './build.sh gcc' }
        }
        stage('Linux Clang') {
            agent { label 'linux && clang' }
            steps { sh './build.sh clang' }
        }
        stage('Windows MSVC') {
            agent { label 'windows' }
            steps { bat 'build.bat msvc' }
        }
    }
}
```

### 2.5 Matrix Builds

Declarative matrix generates combinations automatically:

```groovy
stage('Build Matrix') {
    matrix {
        axes {
            axis {
                name 'COMPILER'
                values 'gcc', 'clang'
            }
            axis {
                name 'BUILD_TYPE'
                values 'Debug', 'Release'
            }
        }
        excludes {
            exclude {
                axis { name 'COMPILER'; values 'gcc' }
                axis { name 'BUILD_TYPE'; values 'Debug' }
            }
        }
        agent { label 'linux' }
        stages {
            stage('Compile') {
                steps {
                    sh "./build.sh ${COMPILER} ${BUILD_TYPE}"
                }
            }
        }
    }
}
```

This generates 3 combinations (4 minus 1 excluded). Very useful for compiler teams that test across multiple compilers and configs.

### 2.6 Groovy and Shared Libraries

Jenkins Pipeline is Groovy. You can extract reusable logic into **shared libraries**.

**Shared library structure:**

```
(repo: compiler-ci-lib)
├── vars/
│   ├── buildCompiler.groovy      # global function: buildCompiler(...)
│   └── runTests.groovy           # global function: runTests(...)
├── src/org/nvidia/ci/
│   └── CompilerPipeline.groovy   # class-based logic
└── resources/
    └── scripts/
        └── run_lint.sh           # static resources
```

**vars/buildCompiler.groovy:**

```groovy
def call(Map cfg) {
    def buildType = cfg.get('buildType', 'Release')
    def generator = cfg.get('generator', 'Ninja')

    sh """
        cmake -S . -B build -G '${generator}' -DCMAKE_BUILD_TYPE=${buildType}
        cmake --build build --parallel \$(nproc)
    """
}
```

**Using in a Jenkinsfile:**

```groovy
@Library('compiler-ci-lib@main') _

pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                buildCompiler(buildType: 'RelWithDebInfo', generator: 'Ninja')
            }
        }
    }
}
```

**Class-based example (src/):**

```groovy
// src/org/nvidia/ci/CompilerBuild.groovy
package org.nvidia.ci

class CompilerBuild implements Serializable {
    def steps
    CompilerBuild(steps) { this.steps = steps }

    def cmakeBuild(String type) {
        steps.sh "cmake -B build -DCMAKE_BUILD_TYPE=${type}"
        steps.sh 'cmake --build build --parallel $(nproc)'
    }
}
```

**Groovy closures (you'll see these in Jenkins DSL):**

```groovy
def withTimeout = { minutes, body ->
    timeout(time: minutes, unit: 'MINUTES') {
        body()
    }
}

withTimeout(30) {
    sh 'ninja -C build check-all'
}
```

### 2.7 CPS (Continuation-Passing Style) — Common Interview Topic

Jenkins Pipeline Groovy runs in a CPS-transformed interpreter. This means:

- Pipeline can **pause and resume** (e.g., `input` step, agent restart)
- **Non-serializable** objects can't survive checkpoint boundaries
- Heavy Groovy computation on the controller blocks the UI thread

**Safe pattern:** Use Groovy only for orchestration. Heavy work goes in `sh` steps on agents.

```groovy
// GOOD: shell does the heavy lifting
def changedFiles = sh(script: 'git diff --name-only HEAD~1', returnStdout: true).trim()

// BAD: parsing huge logs in Groovy on the controller
def log = readFile('giant.log')  // don't do this
```

`**@NonCPS**` — marks a function as not CPS-transformed. Use only for small pure-Groovy utilities that don't call pipeline steps. Misuse causes subtle bugs.

### 2.8 Jenkins Agents

**Label-based routing:**

```groovy
agent { label 'gpu && cuda12' }
```

**Docker agent (spin up container per build):**

```groovy
agent {
    docker {
        image 'nvcr.io/nvidia/cuda:12.0.0-devel-ubuntu22.04'
        args '-v /opt/toolchains:/opt/toolchains:ro --gpus all'
    }
}
```

**Kubernetes agent (ephemeral pods):**

```groovy
agent {
    kubernetes {
        yaml """
            apiVersion: v1
            kind: Pod
            spec:
              containers:
              - name: builder
                image: my-registry/compiler-ci:latest
                command: ['cat']
                tty: true
                resources:
                  requests:
                    memory: "32Gi"
                    cpu: "8"
        """
    }
}

// Use: container('builder') { sh 'ninja -C build' }
```

K8s agents scale to zero when idle — you only pay for what you use. Each build gets a clean environment.

### 2.9 Multi-Branch Pipelines

A **Multibranch Pipeline** job scans a repo for branches and PRs, creating a child pipeline for each one using the same `Jenkinsfile`.

**How it works:**

1. Jenkins periodically scans the repo (or receives webhook)
2. For each branch with a `Jenkinsfile`, it creates/updates a job
3. Each PR gets its own isolated build history
4. Branches without a `Jenkinsfile` are ignored

**Skip builds for docs-only changes:**

```groovy
stage('Build') {
    when {
        not { changeset '**/*.md' }
    }
    steps { sh './build.sh' }
}
```

### 2.10 Credentials Management

**Types:** username/password, secret text, secret file, SSH key, certificate.

```groovy
stage('Publish') {
    steps {
        withCredentials([
            usernamePassword(
                credentialsId: 'artifactory-creds',
                usernameVariable: 'ART_USER',
                passwordVariable: 'ART_PASS'
            )
        ]) {
            sh '''
                curl -u "$ART_USER:$ART_PASS" \
                     -T package.tar.gz \
                     https://artifacts.example.com/compiler/nightly/
            '''
        }
    }
}
```

**Or via environment block:**

```groovy
environment {
    API_KEY = credentials('my-secret-text-id')
}
```

**Best practices:**

- Never echo secrets; Jenkins masks them in `withCredentials`
- Use per-job scoped credentials, not global admin tokens
- Prefer short-lived tokens; rotate regularly

### 2.11 Triggers and Webhooks


| Trigger         | How it works                                     | When to use                                 |
| --------------- | ------------------------------------------------ | ------------------------------------------- |
| **Webhook**     | Git server POSTs to Jenkins on push/PR           | Preferred — instant, efficient              |
| **SCM Polling** | Jenkins checks Git periodically ("any changes?") | Fallback when webhooks aren't possible      |
| **Cron**        | Time-based schedule                              | Nightly full builds, long regression suites |
| **Upstream**    | Trigger when another job finishes                | Chain dependent pipelines                   |


```groovy
pipeline {
    agent any
    triggers {
        cron('H 2 * * *')  // nightly around 2am, H spreads load across minute
    }
    stages { /* ... */ }
}
```

**Webhook setup:** In GitHub/GitLab, add URL like `https://jenkins.example.com/github-webhook/` with a shared secret.

### 2.12 Build Artifacts

**Archiving (persisted on controller):**

```groovy
stage('Package') {
    steps {
        sh 'tar czf compiler-sdk-${BUILD_NUMBER}.tar.gz -C install .'
        archiveArtifacts artifacts: 'compiler-sdk-*.tar.gz', fingerprint: true
    }
}
```

**Stash/unstash (pass files between stages on different agents):**

```groovy
stage('Build') {
    agent { label 'fast-builder' }
    steps {
        sh 'cmake --build build'
        stash includes: 'build/**', name: 'build-output'
    }
}
stage('Test') {
    agent { label 'test-farm' }
    steps {
        unstash 'build-output'
        sh './run_tests.sh'
    }
}
```

`stash` = within a single pipeline run. `archiveArtifacts` = persisted for download and history.

### 2.13 Parameterized Builds

```groovy
parameters {
    choice(name: 'BUILD_TYPE', choices: ['Release', 'Debug', 'RelWithDebInfo'])
    booleanParam(name: 'RUN_EXPENSIVE_TESTS', defaultValue: false)
    string(name: 'TARGET_ARCH', defaultValue: 'x86_64')
}

stages {
    stage('Configure') {
        steps {
            sh "cmake -B build -DCMAKE_BUILD_TYPE=${params.BUILD_TYPE}"
        }
    }
    stage('Test') {
        when { expression { params.RUN_EXPENSIVE_TESTS } }
        steps { sh './expensive_tests.sh' }
    }
}
```

### 2.14 Environment Variables

**Built-in:** `WORKSPACE`, `BUILD_NUMBER`, `BUILD_URL`, `JOB_NAME`, `GIT_COMMIT`, `GIT_BRANCH`, `NODE_NAME`.

```groovy
environment {
    CC  = 'clang'
    CXX = 'clang++'
    PATH = "/opt/llvm/bin:${env.PATH}"
}
```

### 2.15 Important Plugins


| Plugin                            | Purpose                                                      |
| --------------------------------- | ------------------------------------------------------------ |
| **Pipeline**                      | Core pipeline engine (`Jenkinsfile`, stages, steps)          |
| **Git**                           | SCM checkout, branches, tags                                 |
| **Docker Pipeline**               | Build inside containers, `agent { docker { } }`              |
| **Kubernetes**                    | Dynamic pod-based agents                                     |
| **Credentials**                   | Secret management                                            |
| **Blue Ocean**                    | Modern pipeline visualization                                |
| **JUnit**                         | Parse and display test results                               |
| **Warnings Next Gen**             | Track compiler warnings as build health metrics              |
| **Configuration as Code (JCasC)** | YAML-based Jenkins configuration                             |
| **Lockable Resources**            | Serialize access to scarce hardware (e.g., one GPU test rig) |
| **AnsiColor**                     | Colorized compiler diagnostics in console                    |
| **Throttle Concurrent Builds**    | Prevent oversubscribing shared resources                     |


### 2.16 `input` Step (Human Gates)

For promotion or release pipelines, pause for approval:

```groovy
stage('Promote to Staging') {
    when { branch 'release/*' }
    steps {
        input message: 'Deploy this build to staging?',
              ok: 'Deploy',
              submitter: 'release-team'
    }
}
```

Always use `timeout` with `input` to avoid blocking an executor forever.

---

## 3. GitLab CI/CD

### 3.1 `.gitlab-ci.yml` Structure

```yaml
stages:
  - lint
  - build
  - test
  - deploy

variables:
  CMAKE_BUILD_TYPE: "Release"
  GIT_DEPTH: "20"            # shallow clone for speed

default:
  image: ubuntu:22.04
  before_script:
    - apt-get update -qq && apt-get install -y -qq cmake ninja-build git
```

### 3.2 Jobs, Stages, Rules

```yaml
lint:cppcheck:
  stage: lint
  script:
    - cppcheck --enable=all --error-exitcode=1 src/
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'

build:release:
  stage: build
  script:
    - cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    - cmake --build build
  artifacts:
    paths:
      - build/
    expire_in: 1 week

test:unit:
  stage: test
  needs: ["build:release"]     # DAG: doesn't wait for all of 'build' stage
  script:
    - cd build && ctest --output-on-failure
```

`**rules:` vs old `only/except`:** Always use `rules:` — more powerful, supports `changes`, `exists`, variables.

**Run only when compiler sources change:**

```yaml
build:compiler:
  rules:
    - changes:
        - compiler/**/*
        - cmake/**/*
```

### 3.3 `workflow:rules` — Control When Pipelines Are Created

```yaml
workflow:
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
    - if: '$CI_COMMIT_TAG'
    - when: never       # skip everything else (bot commits, draft MRs, etc.)
```

This saves runner minutes on large compiler repos by skipping pipelines that shouldn't exist.

### 3.4 `extends` and Templates

Reuse job definitions without copy-paste:

```yaml
.compiler_job:
  image: my-registry/compiler-ci:v3
  before_script:
    - source /opt/sdk/env.sh

build:clang:
  extends: .compiler_job
  script: ./build.sh clang

build:gcc:
  extends: .compiler_job
  script: ./build.sh gcc
```

`**include:` for modular configs:**

```yaml
include:
  - local: '/ci/templates/.compiler-build.yml'
  - project: 'devops/ci-templates'
    ref: main
    file: '/linux-docker.yml'
```

### 3.5 GitLab Runners


| Runner type         | Description                                                       |
| ------------------- | ----------------------------------------------------------------- |
| **Shared**          | Provided by admin, queued with other projects                     |
| **Specific**        | Tied to project/group — use for GPU machines, internal toolchains |
| **Docker executor** | Each job runs in a fresh container (common default)               |
| **Shell executor**  | Runs directly on host — more power, less isolation                |


```bash
# Register a runner
gitlab-runner register \
  --url https://gitlab.example.com/ \
  --registration-token TOKEN \
  --executor docker \
  --docker-image ubuntu:22.04 \
  --tag-list "linux,compiler-farm"
```

**Route jobs by tags:**

```yaml
build:heavy:
  tags: [compiler-farm]
  script: ./build_all.sh
```

### 3.6 Cache vs Artifacts


|               | Cache                                              | Artifacts                       |
| ------------- | -------------------------------------------------- | ------------------------------- |
| **Purpose**   | Speed up repeated installs (ccache, conan, pip)    | Pass build outputs between jobs |
| **Scope**     | Keyed, best-effort, can be shared across pipelines | Tied to specific pipeline/job   |
| **Guarantee** | May be evicted any time                            | Available until `expire_in`     |


```yaml
variables:
  CCACHE_DIR: "$CI_PROJECT_DIR/.ccache"

cache:
  key: "$CI_COMMIT_REF_SLUG"
  paths:
    - .ccache/

build:
  script:
    - ccache -z
    - cmake --build build
    - ccache -s        # show hit stats
  artifacts:
    paths: [build/]
```

### 3.7 Parallel Matrix

```yaml
build:matrix:
  parallel:
    matrix:
      - COMPILER: [gcc, clang]
        BUILD_TYPE: [Debug, Release]
  script:
    - ./ci/build.sh "$COMPILER" "$BUILD_TYPE"
```

This generates 4 jobs automatically — one for each combination.

### 3.8 Pipeline Triggers and Schedules

**API trigger:**

```bash
curl -X POST \
  -F token=TOKEN \
  -F ref=main \
  https://gitlab.example.com/api/v4/projects/1/trigger/pipeline
```

**Schedules:** GitLab UI → CI/CD → Schedules → set cron for nightly builds.

---

## 4. CI/CD for Compiler Teams

### 4.1 Build Matrix Strategy

You can't run every combination on every PR — too expensive. Common approach:


| When          | What to run                                              |
| ------------- | -------------------------------------------------------- |
| Every PR      | Primary config: Linux + Clang + Release                  |
| Merge to main | Expanded matrix: Linux + {GCC, Clang} × {Debug, Release} |
| Nightly       | Full matrix: all platforms, all compilers, all configs   |
| Pre-release   | Full matrix + long regression suites + hardware tests    |


### 4.2 Static Analysis Gates

- Run clang-tidy, cppcheck, Coverity on diffs (faster than full codebase)
- Publish results via Warnings NG plugin (Jenkins) or Code Quality (GitLab)
- **Fail only on new findings** using baselines — avoids blocking on legacy issues

```groovy
stage('Static Analysis') {
    steps {
        sh 'scan-build cmake --build build || true'
        recordIssues tools: [clangAnalyzer()]
    }
}
```

### 4.3 Test Layering


| Layer           | Scope                    | When                 |
| --------------- | ------------------------ | -------------------- |
| **Unit**        | Fast, isolated           | Every commit/PR      |
| **Integration** | Components together      | PR or merge to main  |
| **Regression**  | Large suites (LNT, SPEC) | Nightly, pre-release |


### 4.4 Artifact Promotion

```
CI Build ──▶ dev-local (auto) ──▶ staging-local (after QA) ──▶ release-local (signed, immutable)
```

**Key principles:**

- Artifacts are **immutable** — never rebuild for promotion, just copy/tag
- Version with semver or calver + git SHA
- Store build provenance (who built, which commit, which Dockerfile)

### 4.5 Build Notifications

- Slack/email on **failure and recovery** (not on every success — too noisy)
- Include direct link to console, culprit from git blame
- For compiler teams: attach first failing test log snippet

---

## 5. Hands-On Exercises

### Exercise 1: Basic Declarative Pipeline

Write a `Jenkinsfile` that:

1. Checks out SCM
2. Runs `cmake -S . -B build && cmake --build build`
3. Runs `ctest --output-on-failure`
4. Archives `build/Testing/**/*.xml` as JUnit results
5. Sends email on failure

### Exercise 2: Parameterized Build

Add parameters `BUILD_TYPE` (choice: Release/Debug) and `SIGN` (boolean). When `SIGN=true`, call a `./sign.sh` script using credentials from Jenkins.

### Exercise 3: Multi-Platform Matrix

Design a Jenkins matrix with `COMPILER` (gcc, clang) × `ARCH` (x86_64, aarch64). Exclude gcc+aarch64 (your farm doesn't have that combo).

### Exercise 4: GitLab CI with Cache

Write `.gitlab-ci.yml` with:

- `build` job producing `install/` artifact
- `test` job consuming it via `needs:`
- ccache in `cache:` with branch-based key

### Exercise 5: Shared Library

Extract `buildCompiler()` and `runTests()` into a Jenkins shared library. Call them from two different project Jenkinsfiles.

### Exercise 6: Debugging Webhook Issues

A push doesn't trigger Jenkins. List the debugging steps in order:

1. Check webhook URL and secret in GitLab/GitHub settings
2. Check webhook delivery logs (recent deliveries tab)
3. Check Jenkins system log for incoming webhook events
4. Verify branch filters in Multibranch config
5. Check firewall/proxy between Git server and Jenkins
6. Verify CSRF protection settings

---

## 6. Interview Questions

**Q1: What is the difference between CI and CD?**
CI integrates code frequently with automated build/test. CD ensures software is always deployable. Continuous *deployment* additionally deploys every passing change automatically. Compiler teams typically stop at continuous *delivery* with manual release promotion.

**Q2: Why should the Jenkins controller not run heavy builds?**
The controller handles scheduling, UI, and pipeline state. Heavy compilation competes for its resources, risks stability, and if a malicious build compromises the controller, it compromises everything.

**Q3: Declarative vs Scripted — when would you choose Scripted?**
When you need dynamic stage generation at runtime, complex Groovy control flow, or logic that doesn't fit Declarative's structure. Prefer Declarative for standard flows.

**Q4: What is an executor?**
A slot on an agent that runs one build at a time. A machine with 4 executors can run 4 concurrent builds. More executors = more parallelism but also more resource contention.

**Q5: How do `stash` and `archiveArtifacts` differ?**
`stash` passes files between stages/agents *within a single pipeline run*. `archiveArtifacts` persists files on the controller for download and history *after* the build.

**Q6: How do you pass secrets safely in Jenkins?**
Use the Credentials plugin with `withCredentials` or environment binding. Never echo them. Use short-lived tokens, rotate regularly, scope credentials per-job when possible.

**Q7: What is a Multibranch Pipeline?**
A job that scans a repo for branches/PRs and creates per-branch pipelines from the `Jenkinsfile` in each branch. Each PR gets isolated build history.

**Q8: SCM polling vs webhook?**
Polling checks Git periodically (wasteful, latent). Webhooks push events from Git server (instant, efficient). Webhooks are always preferred when possible.

**Q9: What are Jenkins Shared Libraries?**
Reusable Groovy code (in `vars/`, `src/`, `resources/`) versioned in a Git repo, loaded with `@Library`. Standardizes pipelines across teams and avoids copy-pasting Jenkinsfile logic.

**Q10: How does GitLab `cache` differ from `artifacts`?**
Cache speeds up repeated installs (best-effort, keyed). Artifacts are explicit outputs passed between jobs with guaranteed retention until expiry.

**Q11: What is `needs:` in GitLab CI?**
Creates a DAG: a job depends on specific earlier jobs, not the entire previous stage. Enables faster parallel execution.

**Q12: How would you design a build matrix without exploding CI time?**
Run a representative subset on every PR (e.g., Linux + Clang Release). Full matrix nightly or on main merges. Shard tests across runners. Use ccache with careful keying.

**Q13: What is CPS in Jenkins and why does it matter?**
CPS (Continuation-Passing Style) transforms Pipeline Groovy so it can pause and resume. Non-serializable state can't survive checkpoint boundaries. Don't do heavy computation in Groovy — delegate to `sh` steps on agents.

**Q14: What is artifact promotion?**
Moving immutable build outputs through quality gates (dev → staging → release) without rebuilding. Each gate adds verification, not a new build.

**Q15: How do Docker agents help compiler CI?**
Reproducible toolchains in every build. Isolation between builds. Pinned base images prevent dependency drift. Pair with ccache volumes for speed.

**Q16: What is Blue Ocean?**
An optional modern UI for Jenkins focused on pipeline visualization. Shows parallel/matrix flows clearly, highlights failures. Not a replacement for JCasC or Job DSL — it's primarily visualization.

**Q17: How do you handle flaky tests in CI?**
Quarantine and fix root cause. Don't hide flakes with unlimited retries. Track flake rate, bisect to the commit, gate release branches more strictly than feature branches.

**Q18: What is `workflow:rules` in GitLab?**
Controls whether a pipeline is created for an event *at all* (unlike per-job `rules`). Use it to suppress pipelines for bots, draft MRs, or documentation-only changes.

**Q19: When would you use Lockable Resources?**
When a job needs exclusive access to scarce hardware (single GPU rig, license dongle) so concurrent builds don't corrupt each other's runs.

**Q20: Difference between `agent any` and `agent none`?**
`agent any` assigns the whole pipeline to one available agent. `agent none` requires each stage to declare its own `agent` — essential for multi-platform pipelines where different stages run on different machines.

---

## Jenkins vs GitLab CI Quick Reference


| Topic                   | Jenkins                          | GitLab CI                               |
| ----------------------- | -------------------------------- | --------------------------------------- |
| Pipeline file           | `Jenkinsfile`                    | `.gitlab-ci.yml`                        |
| Parallelism             | `parallel { }`, `matrix { }`     | `parallel: matrix:`, runner concurrency |
| Pass files between jobs | `stash` / `unstash`              | `artifacts` + `needs:`                  |
| Secrets                 | Credentials + `withCredentials`  | CI/CD Variables (protected, masked)     |
| Cron builds             | `triggers { cron('H 2 * * *') }` | Pipeline Schedules (UI)                 |
| Container builds        | `agent { docker { } }`           | `image:` per job                        |
| Reuse logic             | Shared Libraries (`@Library`)    | `extends:`, `include:`                  |


