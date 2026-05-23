# Git & Perforce (Helix Core)

Deep dive into version control for NVIDIA compiler team interview. Covers Git internals, advanced workflows, and Perforce — which NVIDIA-scale teams often use for large binaries and monorepos.

---

## Table of Contents

1. [Git Internals](#1-git-internals)
2. [Branching, Merging, Rebasing](#2-branching-merging-rebasing)
3. [Advanced Git](#3-advanced-git)
4. [Perforce (Helix Core)](#4-perforce-helix-core)
5. [Git + Perforce Together](#5-git--perforce-together)
6. [Branching Strategies for Compiler Teams](#6-branching-strategies-for-compiler-teams)
7. [Hands-On Exercises](#7-hands-on-exercises)
8. [Interview Questions](#8-interview-questions)

---

## 1. Git Internals

### Object Model

Git is a content-addressable filesystem. Everything is an object named by SHA-1 hash.

```
┌──────────────────────────────────────────────────┐
│                    commit                         │
│  tree: abc123                                     │
│  parent: def456                                   │
│  author: John <john@nvidia.com>                   │
│  message: "Fix codegen for sm_90"                 │
│                                                   │
│         ┌─────── tree abc123 ───────┐             │
│         │                           │             │
│    blob (main.c)             tree (src/)          │
│    "int main..."             ├── blob (foo.c)     │
│                              └── blob (bar.h)     │
└──────────────────────────────────────────────────┘
```

| Object Type | What it stores |
|-------------|---------------|
| **blob** | File contents only (no filename). Same content = same hash everywhere. |
| **tree** | Directory listing: mode, name, pointer to blob or subtree. |
| **commit** | Snapshot (tree SHA), parent(s), author, committer, message. |
| **tag** | Annotated tag: points to commit + tagger + message. |

### Inspecting Objects

```bash
git cat-file -t <sha>         # show object type
git cat-file -p <sha>         # pretty-print content
git rev-parse HEAD:src/main.c # find blob SHA for a file at HEAD
```

### `.git` Directory

```
.git/
  HEAD            # ref: refs/heads/main (current branch)
  config          # repo + remote settings
  index           # staging area (binary)
  objects/        # all blobs, trees, commits
    pack/         # packed objects + .idx for fast lookup
  refs/
    heads/        # local branches (files containing commit SHA)
    tags/         # tags
    remotes/      # remote-tracking branches
  logs/           # reflog (history of ref updates)
  hooks/          # client-side hook scripts
```

**Key insight:** A branch is just a file under `refs/heads/` containing a commit SHA. Creating a branch costs almost nothing.

### The Three States

```
Working Tree ──(git add)──▶ Index/Staging ──(git commit)──▶ HEAD (repository)
```

```bash
git diff            # working tree vs index (unstaged changes)
git diff --cached   # index vs HEAD (staged changes)
git restore --staged path   # unstage
git restore path            # discard working tree changes
```

---

## 2. Branching, Merging, Rebasing

### The DAG (Directed Acyclic Graph)

```
       C---D---E  (feature)
      /
 A---B---F---G    (main)
```

- Each commit has one or more parents (merge commits have 2+)
- Branches are movable pointers into this graph
- HEAD = "where you are" (usually points to a branch)

### Merge Strategies

**Fast-forward:** If main has no new commits since branch point, just move the pointer.

```bash
git checkout main
git merge feature    # main advances to feature's tip, no merge commit
```

**Three-way merge:** When both branches have diverged, Git finds the merge base (common ancestor) and creates a merge commit.

```bash
git merge other-branch
# If conflicts: edit files, git add, git commit
```

**Octopus merge:** Merge 3+ branches at once. Fails if conflicts arise.

```bash
git merge topic-a topic-b topic-c
```

**`-X ours` / `-X theirs`:** Auto-resolve conflicting hunks by picking one side. Does NOT mean "take entire files from one branch" — only resolves overlapping hunks.

```bash
git merge -X theirs other-branch    # prefer their side on conflicts
```

**`-s ours` strategy (different!):** Creates merge commit but keeps YOUR tree entirely. Records that a branch was "merged away" without taking its content.

### Rebase vs Merge

| | Merge | Rebase |
|---|-------|--------|
| History | Preserves topology; merge commits show integration | Rewrites commits onto new base; linear history |
| Safety | Safe (adds commits) | Dangerous if rewriting shared commits |
| Conflicts | One merge resolution | Per-commit during rebase |

**When to merge:** Public/shared branches. True integration events.

**When to rebase:** Local cleanup before PR. Updating private feature branch.

```bash
git checkout feature
git rebase origin/main           # replay feature commits onto latest main

# Interactive rebase: squash, reword, reorder
git rebase -i origin/main
```

**Interactive rebase todo commands:**

```
pick abc1234 Add lexer           # keep as-is
squash def5678 fix typo          # meld into previous commit
reword 89abcde WIP tests         # keep but edit message
fixup 1111111 noise              # meld silently (drop message)
drop 2222222 experimental spike  # remove entirely
```

**Golden rule:** Don't rebase commits that exist on branches others use unless your team explicitly coordinates (force-push with lease).

```bash
git rebase --abort       # bail out
git rebase --continue    # after resolving conflicts
```

---

## 3. Advanced Git

### 3.1 Cherry-pick

Apply a specific commit's patch onto current branch (new commit, new SHA):

```bash
git cherry-pick abc1234
git cherry-pick -x abc1234      # append "cherry-picked from" line
git cherry-pick abc1234^..def5678  # range (abc1234 exclusive)
```

**Build engineer use:** Backporting a fix from main to a release branch without merging everything.

### 3.2 Bisect — Finding the Regressing Commit

Binary search through history:

```bash
git bisect start
git bisect bad                   # current commit is bad
git bisect good v1.2.0          # known good

# Git checks out midpoint. Build and test, then:
git bisect good    # or: git bisect bad

# When done:
git bisect reset
```

**Automated bisect:**

```bash
git bisect start HEAD v1.2.0
git bisect run ./scripts/build_and_test.sh
# Script exit: 0=good, 125=skip, 1-127(not 125)=bad
```

### 3.3 Git Hooks

Scripts under `.git/hooks/` (or `core.hooksPath`).

**Client-side hooks:**

| Hook | When |
|------|------|
| `pre-commit` | Before commit; exit non-zero to abort |
| `commit-msg` | Validate message format |
| `pre-push` | Before refs sent to remote |

```bash
#!/bin/sh
# pre-commit: run formatter
make lint-quick || exit 1
```

**Server-side hooks:**

| Hook | When |
|------|------|
| `pre-receive` | Before accepting push; can reject entire push |
| `update` | Per-ref (branch/tag) |
| `post-receive` | After push; trigger CI, mirrors, notifications |

**Important:** Client hooks can be skipped with `--no-verify`. Server-side hooks (or platform branch policies) are the only trustworthy enforcement.

### 3.4 Submodules vs Subtrees

**Submodules:** Separate repo pinned at a specific commit.

```bash
git submodule add https://example.com/lib.git third_party/lib
git clone --recurse-submodules <url>

# Update submodule
cd third_party/lib && git pull origin main
cd ../.. && git add third_party/lib && git commit -m "Bump lib"
```

Pros: Clear boundaries, separate history.
Cons: Easy to forget `submodule update`; CI must init submodules.

**Subtrees:** Vendor code merged into same repo.

```bash
git subtree add --prefix=vendor/lib https://example.com/lib.git main --squash
```

Pros: Single clone, simpler for consumers.
Cons: Heavier history, fiddly upstream merges.

### 3.5 Git LFS

Pointer files in Git; large blobs on LFS server.

```bash
git lfs install
git lfs track "*.bin"
git add .gitattributes
git add model.bin && git commit -m "Add model via LFS"
```

CI implications: agents need `git lfs pull`, bandwidth/storage quotas apply.

### 3.6 Branching Strategies

**GitFlow:** `main`, `develop`, `feature/*`, `release/*`, `hotfix/*`. Explicit but heavy.

**GitHub Flow:** Short branches off main, PR + CI + merge. Simple, CD-friendly.

**Trunk-based:** Small frequent merges to trunk, feature flags hide incomplete work. Minimal merge pain.

### 3.7 Monorepo vs Polyrepo

| | Monorepo | Polyrepo |
|---|---------|---------|
| Cross-cutting refactors | Easy | Hard (multiple PRs) |
| CI | Needs affected-target detection (Bazel) | Per-repo pipelines |
| Clone size | Large (use sparse checkout) | Small per repo |

### 3.8 Worktrees

Multiple working directories sharing one `.git`:

```bash
git worktree add ../hotfix-tree hotfix/bug-123
cd ../hotfix-tree   # work on hotfix without stashing
git worktree list
git worktree remove ../hotfix-tree
```

### 3.9 Reflog — Recovery

Local history of HEAD and branch movements:

```bash
git reflog
git checkout -b recovery HEAD@{3}   # recover "lost" commits
```

Reflog is local only, time-limited. Not pushed.

### 3.10 Stash

```bash
git stash push -m "wip cuda kernel" -- src/kernel.cu
git stash list
git stash apply stash@{0}   # apply without removing from stash
git stash pop                # apply and remove
```

### 3.11 Blame and Log Tricks

```bash
git blame -M -C path/to/file.cpp          # track moved/copied lines
git blame -L 40,60 path/to/file.cpp       # specific line range
git log -p -- path/to/file.cpp            # patches per commit
git log -S "cudaMalloc" --oneline          # pickaxe: find when string was added
git log --oneline --graph --all -n 30      # visual graph
```

### 3.12 Shallow Clone and Sparse Checkout

```bash
# Shallow: truncated history
git clone --depth 1 https://example.com/huge.git
git fetch --unshallow   # get full history later

# Sparse: partial tree
git clone --filter=blob:none --sparse https://example.com/huge.git
cd huge
git sparse-checkout set compiler/ tests/smoke/
```

Combine for massive monorepos: reduces network and disk dramatically.

### 3.13 `git gc` and Maintenance

```bash
git gc --prune=now          # pack objects, prune unreachable
git maintenance start       # schedule background maintenance (Git 2.30+)
git count-objects -vH       # repo size insight
```

---

## 4. Perforce (Helix Core)

### 4.1 Why Perforce?

Organizations choose Helix Core when:
- **Very large** codebases + **binary assets** (firmware, test vectors, CAD data)
- **Fine-grained permissions** at depot/path level
- **Exclusive checkout** (locking) for binaries that can't be merged
- **Partial sync** via workspace views (only download what you need)
- Native support for long-lived release/integration streams

### 4.2 Core Concepts

| Concept | What it is |
|---------|-----------|
| **Depot** | Top-level namespace on server (`//depot/...`) |
| **Workspace (Client)** | Your machine's view + root: maps depot paths to local disk |
| **Changelist (CL)** | Atomic bundle of edits; numbered, pending until submitted |
| **Shelve** | Upload pending work to server without submitting (for sharing, CI) |
| **Revision** | Per-file version (`#head`, `#3`, `@=changelist`, `@label`) |
| **Integrate** | Perforce's merge/port between branches |

### 4.3 How Perforce Differs from Git

| | Git | Perforce |
|---|-----|---------|
| Model | Distributed; full history locally | Centralized; server holds everything |
| Commits | Local until push | `p4 submit` = commit to server |
| Branches | Lightweight pointers | Different depot paths or streams |
| Offline | Full capability | Very limited without server |
| Large binaries | Needs LFS | First-class support |

### 4.4 Common Commands

```bash
# Environment setup
export P4PORT=ssl:p4.example.com:1666
export P4USER=buildbot
export P4CLIENT=buildbot_linux_amd64
p4 login

# Sync
p4 sync                       # latest in your client view
p4 sync ...@12345              # at or below changelist 12345
p4 sync ...@=12345             # exactly at changelist 12345 (strict)
p4 sync ...@mylabel            # at label

# Edit/Add/Delete
p4 edit src/main.cpp           # open for edit (checks out)
p4 add newfile.txt
p4 delete oldfile.txt

# Status
p4 opened                     # files you have open
p4 diff                       # local changes

# Submit
p4 submit -d "Fix CUDA codegen for sm_90"

# Shelve/Unshelve
p4 shelve -c 123456
p4 unshelve -s 123456 -c 123457

# History
p4 filelog //depot/path/file.cpp
p4 describe 123456             # show changelist details
p4 changes -m 5 //depot/compiler/...   # recent changelists
```

### 4.5 Revision Specifiers

| Spec | Meaning |
|------|---------|
| `file#3` | Exact file revision 3 |
| `file#head` | Latest on server |
| `...@12345` | Files at or below changelist 12345 |
| `...@=12345` | Files exactly at changelist 12345 |
| `...@labelname` | Files at a label |
| `...@2025/01/15` | As of date |

The `@` vs `@=` difference is a common interview gotcha. `@=` is strict snapshot.

### 4.6 Streams

Streams encode branch relationships and merge direction:

- **Mainline:** Integration hub
- **Development:** Child of mainline (ongoing work)
- **Release:** Stabilization; receives cherry-picks from mainline

Streams enforce merge direction — reduces "random integrate" errors.

### 4.7 Labels and Views

**Labels** = named set of file revisions (snapshot):

```bash
p4 tag -l mylabel //depot/proj/...@12345
p4 sync ...@mylabel
```

**Client view** = maps depot paths to local paths:

```
View:
    //depot/proj/compiler/... //client/compiler/...
    -//depot/proj/experimental/... //client/experimental/...
```

Narrow views = faster sync, less disk. Build machines should only map what they need.

### 4.8 Triggers

Server-side scripts invoked on events (like Git server-side hooks):

```
Triggers:
    myValidate change-submit //depot/... "python3 /opt/p4/triggers/validate.py %changelist%"
```

- `change-submit`: validate before accepting (lint, require ticket ID)
- `change-commit`: after submit (notifications, trigger CI)

Script exits non-zero to reject the submit.

### 4.9 CI Integration (Jenkins P4 Plugin)

```groovy
pipeline {
    agent any
    environment {
        P4PORT = 'ssl:p4.company.com:1666'
        P4USER = 'jenkins'
    }
    stages {
        stage('Sync') {
            steps {
                checkout perforce(
                    credential: 'p4-jenkins',
                    populate: syncOnly(pin: '123456'),
                    workspace: manualSpec(
                        name: "jenkins-${NODE_NAME}",
                        view: "//depot/compiler/... //jenkins-${NODE_NAME}/compiler/..."
                    )
                )
            }
        }
    }
}
```

Key: credential + workspace view + sync pinned to changelist/label.

### 4.10 Perforce for Build Engineering

- **One client per agent** (or dynamic clients with templated views)
- **Sync to specific CL** for reproducible builds: `p4 sync ...@12345`
- Record CL number in build metadata alongside compiler version
- **Atomic changelists** — all files in a submit land together (consistent snapshots)

---

## 5. Git + Perforce Together

### `git-p4` Bridge

Clone Perforce depot into Git, submit Git commits back as Perforce CLs:

```bash
git p4 clone //depot/project/main@all myrepo
cd myrepo
git p4 sync      # fetch new CLs
git p4 rebase    # rebase local work
git p4 submit    # push git commits to perforce
```

**Caveats:**
- Binary file types and exclusive locks don't map cleanly to Git
- Large history clones are slow — narrow the depot path
- Author identity mapping needs manual config

### When Companies Use Both

- Perforce for canonical code + binaries (hardware, firmware)
- Git for open-source mirrors, modern tooling, contractor workflows
- Build engineer challenge: two sets of credentials, sync pinning, hash consistency

---

## 6. Branching Strategies for Compiler Teams

### Release Branching

```
trunk:    A---B---C---D---E
               \       \
release:        R0--R1--R2  (tags: 12.1.0, 12.1.1)
```

- **Trunk/main:** Next release development
- **release/x.y:** Stabilization, only bugfixes + release engineering
- **Tags:** Known-good builds shipped to customers
- Cherry-pick security fixes to release branches

### Feature Flags vs Feature Branches

| Feature Branches | Feature Flags |
|-----------------|---------------|
| Isolate code until ready | Ship dark code, enable at runtime |
| Merge cost grows with age | Requires runtime discipline |
| Good for risky refactors | Good for incremental rollout |

Compilers use flags for experimental `-std` modes, new target enablement.

### Integration Branches

Collect features until CI is green. Merge queue / batched merges reduce breakage. Perforce equivalent: frequent integrate from task stream to mainline with resolve discipline.

---

## 7. Hands-On Exercises

### Exercise A: Git Object Exploration

```bash
mkdir git-lab && cd git-lab && git init
echo 'v1' > f.txt && git add f.txt && git commit -m 'first'
echo 'v2' >> f.txt && git add f.txt && git commit -m 'second'
git cat-file -p HEAD          # see parent, tree
git cat-file -p HEAD^{tree}   # see blob
git cat-file -p HEAD:f.txt    # see file content
```

### Exercise B: No-Fast-Forward Merge

Create divergent branches, `git merge --no-ff feature`, inspect the merge commit with `git cat-file -p HEAD` (two parents).

### Exercise C: Interactive Rebase

Create 5 WIP commits, squash them into 1-2 clean commits via `git rebase -i HEAD~5`.

### Exercise D: Automated Bisect

Create 8 commits with a deliberate test failure at commit 5. Use `git bisect run` to find it automatically.

### Exercise E: pre-commit Hook

Write a hook that runs `clang-format --dry-run --Werror` on staged `.c`/`.cpp` files and rejects unformatted commits.

### Exercise F: Perforce Client + Exact Sync

Write a client view, explain how Jenkins would `p4 sync ...@=5000`, and why CL 5000 goes in build metadata.

---

## 8. Interview Questions

**Q1: What is a Git blob vs tree?**
Blob stores raw file contents (no filename). Tree lists directory entries (names, modes, references to blobs/subtrees).

**Q2: What is a merge base?**
Best common ancestor between branches. Three-way merge uses it to distinguish ours vs theirs changes.

**Q3: When rebase over merge?**
Local cleanup or updating a private branch to latest upstream while keeping linear history. Never rebase shared commits.

**Q4: Risk of `git push --force`?**
Overwrites remote history, orphaning others' work. Use `--force-with-lease` to check expected remote tip first.

**Q5: `cherry-pick` vs `merge` for a single fix?**
Cherry-pick applies one commit's patch as a new commit. Merge brings the entire branch history.

**Q6: Explain `git bisect`.**
Binary search between known good and bad commits to find the first bad change. Automatable with `bisect run`.

**Q7: Why are client-side hooks insufficient for security?**
Can be bypassed with `--no-verify`. Server-side hooks/policies enforce rules for all contributors.

**Q8: Submodule vs subtree tradeoff?**
Submodule: separate repo, clean boundaries, workflow friction. Subtree: single clone, simpler but heavier history.

**Q9: What does Git LFS store in Git vs LFS server?**
Git stores a small pointer file. The large blob lives on the LFS server, keyed by OID.

**Q10: Why does a company use Perforce?**
Partial workspaces (views) for huge trees. Strong handling of large binaries with locking. Centralized control.

**Q11: What is a Perforce shelve?**
Upload pending changelist to server without submitting. CI can unshelve for pre-submit validation.

**Q12: Reproducible Perforce sync?**
Sync to specific changelist or label (`p4 sync ...@12345`), not `#head`.

**Q13: Perforce trigger example?**
`change-submit` trigger runs a script to reject submits lacking ticket IDs or failing static analysis.

**Q14: How does Perforce "integrate" relate to Git merge?**
Integrate ports changes between branches/streams. Resolve handles conflicts. Submit finalizes — analogous to pushing a merge commit.

**Q15: GitFlow weakness for CD?**
Long-lived branches and complex choreography slow integration. Trunk-based + flags fits CD better.

**Q16: What is `git reflog`?**
Records local HEAD movements. Recover "lost" commits after bad resets/rebases. Local only, not pushed.

**Q17: Shallow clone + sparse checkout — why combine?**
Shallow limits history depth. Sparse limits tree breadth. Together: minimal clone time and disk for monorepos.

**Q18: Wire Jenkins to Perforce?**
P4 plugin, scoped workspace view, credentials, sync to pinned CL/label, archive artifacts, trigger from post-submit hooks.

**Q19: Why release branches for compilers?**
Stabilize a shipping line while trunk continues. Targeted hotfixes without releasing half-baked features.

**Q20: `git rebase --onto X Y Z`?**
Replays commits after Y up to Z onto base X. Used to slice commits off one branch onto another.

**Q21: Fast-forward vs no-fast-forward merge?**
FF moves the pointer (no merge commit, linear). `--no-ff` creates a merge commit to preserve branch topology.

**Q22: What's in the Git index?**
The proposed next tree (staged snapshot). `git commit` records the index, not the working tree.

**Q23: Atomic changelist in Perforce?**
All files in a submit land together at one revision. No inconsistent intermediate states during sync.

**Q24: `p4 edit` explained to a Git user?**
Opens a file for write on the server (may lock it). Centralized "checkout" — not the same as `git checkout`.
