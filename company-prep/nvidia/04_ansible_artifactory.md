# Ansible & JFrog Artifactory

Deep dive into configuration management and artifact lifecycle for NVIDIA compiler team interview.

---

## Table of Contents

1. [Ansible Fundamentals](#1-ansible-fundamentals)
2. [Inventory](#2-inventory)
3. [Playbooks](#3-playbooks)
4. [Modules](#4-modules)
5. [Variables and Facts](#5-variables-and-facts)
6. [Templates (Jinja2)](#6-templates-jinja2)
7. [Roles](#7-roles)
8. [Conditionals, Loops, Tags](#8-conditionals-loops-tags)
9. [Ansible Vault](#9-ansible-vault)
10. [Error Handling](#10-error-handling)
11. [Ansible for Build Infrastructure](#11-ansible-for-build-infrastructure)
12. [Idempotency](#12-idempotency)
13. [JFrog Artifactory](#13-jfrog-artifactory)
14. [Hands-On Exercises](#14-hands-on-exercises)
15. [Interview Questions](#15-interview-questions)

---

## 1. Ansible Fundamentals

### What is Ansible?

Ansible is an open-source automation tool for configuration management, application deployment, and orchestration. You describe desired state in YAML, and Ansible makes it happen.

### Architecture

```
┌──────────────────┐
│   Control Node   │
│  (your laptop    │
│   or CI runner)  │
│                  │     SSH
│  ansible-playbook├─────────────┐
│                  │             │
└──────────────────┘             │
                           ┌─────▼─────────┐
                           │  Managed Node  │
                           │  (build agent) │
                           │                │
                           │ Python runs    │
                           │ module ──▶ JSON│
                           │ result back    │
                           └────────────────┘
```

**Key characteristics:**


| Feature       | Ansible                           | Puppet/Chef               |
| ------------- | --------------------------------- | ------------------------- |
| Architecture  | **Agentless** — pushes over SSH   | Agents run on every node  |
| Model         | **Push** — control node initiates | Pull — agents poll master |
| Language      | YAML (declarative)                | Ruby DSL                  |
| Bootstrapping | Just needs SSH + Python on target | Install agent first       |


**Why agentless matters:** Simpler bootstrapping, less attack surface on build machines, easier to reason about "who initiated this change." The downside: requires SSH access and Python on the target.

---

## 2. Inventory

The inventory tells Ansible which machines to manage.

### Static Inventory (INI format)

```ini
[build_agents]
agent01 ansible_host=10.0.1.10 ansible_user=builder
agent02 ansible_host=10.0.1.11 ansible_user=builder

[gpu_builders]
gpu01 ansible_host=10.0.2.5

[build_agents:children]
gpu_builders

[build_agents:vars]
ansible_python_interpreter=/usr/bin/python3
```

### Static Inventory (YAML format)

```yaml
all:
  children:
    build_agents:
      hosts:
        agent01:
          ansible_host: 10.0.1.10
          ansible_user: builder
        agent02:
          ansible_host: 10.0.1.11
      vars:
        ansible_python_interpreter: /usr/bin/python3
    gpu_builders:
      hosts:
        gpu01:
          ansible_host: 10.0.2.5
```

### Dynamic Inventory

Generates the host list at runtime from a cloud API, CMDB, or custom script.

**Examples:**

- AWS: `amazon.aws.aws_ec2` inventory plugin
- Azure: `azure.azcollection.azure_rm` plugin
- Custom: executable script returning JSON with `--list` and `--host`

**For build fleets:** Dynamic inventory keeps CI agent lists accurate when VMs auto-scale. Combine with `group_by` or `add_host` in playbooks for late-discovered hosts.

---

## 3. Playbooks

A **playbook** contains one or more **plays**. Each play targets hosts and runs tasks.

```yaml
---
- name: Configure build agents
  hosts: build_agents
  become: true

  vars:
    docker_compose_version: "2.24.0"

  tasks:
    - name: Install Docker CE packages
      ansible.builtin.apt:
        name:
          - docker-ce
          - docker-ce-cli
          - containerd.io
        state: present
        update_cache: true
      notify: Restart docker

    - name: Add builder to docker group
      ansible.builtin.user:
        name: builder
        groups: docker
        append: true

  handlers:
    - name: Restart docker
      ansible.builtin.service:
        name: docker
        state: restarted
```

### Handlers

Handlers are tasks that run **once at the end of the play**, **only if notified**. Perfect for service restarts after config changes — you don't want to restart Docker 5 times if 5 tasks notify it.

```yaml
- name: Deploy Jenkins agent config
  ansible.builtin.template:
    src: agent.xml.j2
    dest: /var/lib/jenkins/agent-config.xml
  notify: Reload jenkins
```

---

## 4. Modules

Use fully qualified collection names (FQCN) in modern Ansible: `ansible.builtin.apt`, not just `apt`.

### `command` vs `shell`

```yaml
# command: no shell features (no pipes, no redirection)
- name: Check gcc version
  ansible.builtin.command: gcc --version
  register: gcc_ver
  changed_when: false

# shell: runs through /bin/sh (pipes, redirects work)
- name: Count running containers
  ansible.builtin.shell: docker ps -q | wc -l
  register: count
  changed_when: false
```

**Best practice:** Prefer dedicated modules (`apt`, `copy`, `template`) over `shell` — better idempotency and error reporting.

### Common modules

```yaml
# copy: push a file
- name: Install CA bundle
  ansible.builtin.copy:
    src: files/company-ca.crt
    dest: /usr/local/share/ca-certificates/company-ca.crt
    mode: "0644"

# template: push a Jinja2 template
- name: Render Docker daemon.json
  ansible.builtin.template:
    src: templates/daemon.json.j2
    dest: /etc/docker/daemon.json
  notify: Restart docker

# apt: install packages (Debian/Ubuntu)
- name: Install build toolchain
  ansible.builtin.apt:
    name: [build-essential, cmake, ninja-build, git]
    state: present
    update_cache: true

# dnf: install packages (RHEL/Fedora)
- name: Install dev tools
  ansible.builtin.dnf:
    name: [gcc-c++, cmake, ninja-build]
    state: present
  when: ansible_os_family == "RedHat"

# file: manage directories
- name: Create workspace
  ansible.builtin.file:
    path: /srv/builds
    state: directory
    owner: builder
    mode: "0755"

# service: manage daemons
- name: Enable Docker
  ansible.builtin.service:
    name: docker
    state: started
    enabled: true
```

---

## 5. Variables and Facts

### Variable Precedence (simplified, highest wins)

1. Extra vars (`-e`) — **highest**
2. Task vars / block vars
3. Role vars / include vars
4. `host_vars` / `group_vars`
5. Inventory vars
6. Role defaults — **lowest**

### `group_vars` / `host_vars`

```
inventory/
  hosts.yml
  group_vars/
    build_agents.yml     # applies to all build_agents
  host_vars/
    gpu01.yml            # applies only to gpu01
```

```yaml
# group_vars/build_agents.yml
compiler_packages:
  - gcc-12
  - g++-12
jenkins_url: "https://jenkins.example.com"
```

### Facts

Automatically gathered info about each host: `ansible_distribution`, `ansible_memtotal_mb`, `ansible_default_ipv4.address`.

```yaml
- name: Show OS
  ansible.builtin.debug:
    msg: "{{ ansible_distribution }} {{ ansible_distribution_version }}"
```

Disable when not needed (faster):

```yaml
- hosts: build_agents
  gather_facts: false
```

### `register` — capture task output

```yaml
- name: Check disk space
  ansible.builtin.command: df -h /
  register: df_out
  changed_when: false

- name: Print it
  ansible.builtin.debug:
    var: df_out.stdout_lines
```

---

## 6. Templates (Jinja2)

Combine static config with variables and logic:

```jinja2
{# templates/daemon.json.j2 #}
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "{{ docker_log_max_size }}",
    "max-file": "{{ docker_log_max_files }}"
  },
  "insecure-registries": [
  {% for reg in docker_insecure_registries %}
    "{{ reg }}"{% if not loop.last %},{% endif %}
  {% endfor %}
  ]
}
```

**Useful filters:** `default`, `join`, `to_json`, `quote`, `upper`, `lower`.

---

## 7. Roles

Roles package related tasks, handlers, templates, and defaults into a reusable unit.

### Directory Structure

```
roles/
  jenkins_agent/
    defaults/main.yml       # lowest precedence defaults
    vars/main.yml           # role vars (higher precedence)
    tasks/main.yml          # the actual work
    handlers/main.yml
    templates/
    files/
    meta/main.yml           # dependencies, galaxy metadata
```

### Using Roles

```yaml
- hosts: build_agents
  become: true
  roles:
    - role: jenkins_agent
      vars:
        jenkins_master_url: "https://jenkins.example.com"
```

### Ansible Galaxy

Public hub for roles and collections:

```bash
ansible-galaxy collection install community.docker
ansible-galaxy role install geerlingguy.docker
```

Prefer **collections** (namespaced modules) over standalone roles for new projects.

---

## 8. Conditionals, Loops, Tags

### `when` — conditional execution

```yaml
- name: Install CUDA toolkit
  ansible.builtin.apt:
    name: cuda-toolkit-12-3
    state: present
  when:
    - ansible_os_family == "Debian"
    - "'gpu_builders' in group_names"
```

### Loops

```yaml
# Modern: loop
- name: Create build users
  ansible.builtin.user:
    name: "{{ item.name }}"
    groups: "{{ item.groups }}"
  loop:
    - { name: builder, groups: "docker" }
    - { name: conan_ci, groups: "docker" }

# From variable
- name: Install packages
  ansible.builtin.apt:
    name: "{{ item }}"
    state: present
  loop: "{{ base_packages }}"
```

### Tags — selective execution

```yaml
- name: Heavy compiler install
  ansible.builtin.apt:
    name: "{{ compiler_packages }}"
    state: present
  tags: [toolchain, slow]
```

```bash
ansible-playbook site.yml --tags toolchain
ansible-playbook site.yml --skip-tags slow
```

---

## 9. Ansible Vault

Encrypts sensitive data at rest.

```bash
# Create encrypted file
ansible-vault create group_vars/build_agents/vault.yml

# Encrypt existing file
ansible-vault encrypt group_vars/build_agents/secrets.yml

# Run playbook
ansible-playbook site.yml --ask-vault-pass
ansible-playbook site.yml --vault-password-file ~/.vault_pass
```

**Pattern: plain + encrypted files together**

```yaml
# group_vars/build_agents/vars.yml (plain, committed)
artifactory_user: "ci_bot"
artifactory_password: "{{ vault_artifactory_password }}"

# group_vars/build_agents/vault.yml (encrypted, committed)
vault_artifactory_password: "supersecret123"
```

**CI pattern:** Store vault password in Jenkins credentials, write to ephemeral file, pass `--vault-password-file`.

---

## 10. Error Handling

### `ignore_errors` and `failed_when`

```yaml
- name: Probe optional service
  ansible.builtin.uri:
    url: "http://127.0.0.1:9100/metrics"
    timeout: 2
  register: metrics
  failed_when: false       # don't fail the play
  changed_when: false
```

### `block` / `rescue` / `always`

Like try/catch/finally:

```yaml
- name: Upgrade agent safely
  block:
    - name: Pull new image
      community.docker.docker_image:
        name: "{{ agent_image }}:{{ agent_tag }}"
        source: pull

    - name: Restart agent
      community.docker.docker_container:
        name: jenkins-agent
        image: "{{ agent_image }}:{{ agent_tag }}"
        state: started
        restart: true

  rescue:
    - name: Notify on failure
      ansible.builtin.debug:
        msg: "Upgrade failed — check logs and revert"

  always:
    - name: Run health check
      ansible.builtin.command: /usr/local/bin/healthcheck.sh
      changed_when: false
```

---

## 11. Ansible for Build Infrastructure

### Typical layers for a build machine

1. **Base OS** — users, sudo, firewall, time sync
2. **Build dependencies** — compilers, CMake, Ninja, Python, Java
3. **Artifact tooling** — Conan client, Docker, credential helpers
4. **CI agent** — Jenkins SSH agent, K8s pod template, GitLab runner

### Installing compiler toolchains

```yaml
- name: Add LLVM apt repo
  ansible.builtin.apt_repository:
    repo: "deb http://apt.llvm.org/{{ ansible_distribution_release }}/ llvm-toolchain-{{ ansible_distribution_release }}-17 main"
    state: present

- name: Install Clang 17
  ansible.builtin.apt:
    name: clang-17
    state: present
    update_cache: true
```

### Jenkins agent via systemd

```ini
# templates/jenkins-agent.service.j2
[Unit]
Description=Jenkins SSH Agent
After=network.target

[Service]
User=jenkins
WorkingDirectory=/var/lib/jenkins
ExecStart=/usr/bin/java -jar /var/lib/jenkins/agent.jar
Restart=always

[Install]
WantedBy=multi-user.target
```

### Ad-hoc commands

Quick one-off checks without a playbook:

```bash
ansible build_agents -m ping
ansible build_agents -m ansible.builtin.apt -a "name=git state=present" -b
ansible gpu01 -m ansible.builtin.shell -a "nvidia-smi"
```

---

## 12. Idempotency

A task is **idempotent** if running it multiple times leaves the system in the same state without side effects.

**Idempotent:**

- `apt state=present` → installs once, then reports `ok`
- `copy` with same content → `ok` (no change)

**NOT idempotent:**

- `shell: echo x >> /tmp/log` → appends every run

**Fix non-idempotent commands:**

- Use `creates:` parameter: `command: ./install.sh` with `creates: /opt/tool/bin/tool`
- Use `lineinfile` instead of `echo >>`
- Or refactor to a proper module

**Interview answer:** "Idempotency means automation converges to declared state. Ansible modules compare actual vs desired. For imperative steps, use `creates:`, `removes:`, or refactor to a module."

---

## 13. JFrog Artifactory

### What is Artifactory?

A **binary repository manager** — a central store for artifacts (packages, containers, tarballs, build outputs) with metadata, access control, and lifecycle management.

### Why Artifact Management Matters for Build Engineering

- **Reproducibility:** Same dependency versions across time and machines
- **Speed:** Cache proxied remotes (Docker Hub, PyPI, ConanCenter) — no external flakiness
- **Security:** Scanning integration, promotion gates, immutable releases
- **Compliance:** Retention policies, audit trails, license metadata

### Repository Types

```
┌─────────────────────────────────────────────┐
│              Virtual Repository             │
│  (clients talk to this single URL)          │
│                                             │
│  ┌─────────────┐    ┌───────────────────┐   │
│  │    Local    │    │     Remote        │   │
│  │ (your stuff)│    │ (proxy/cache of   │   │
│  │             │    │  external repos)  │   │
│  └─────────────┘    └───────────────────┘   │
└─────────────────────────────────────────────┘
```


| Type        | Purpose                                                         |
| ----------- | --------------------------------------------------------------- |
| **Local**   | Stores artifacts your org builds and uploads                    |
| **Remote**  | Proxy + cache of external repos (Docker Hub, ConanCenter, PyPI) |
| **Virtual** | Aggregates locals + remotes under one URL; clients use this     |


### Artifact Types


| Format           | Examples                                         |
| ---------------- | ------------------------------------------------ |
| **Generic**      | Tarballs, SDKs, license files — any file layout  |
| **Docker**       | Container images; `docker login` + push/pull     |
| **Conan**        | C++ packages — **highly relevant for NVIDIA**    |
| **Maven/Gradle** | Java ecosystem (JARs, POMs)                      |
| **PyPI**         | Python packages (`pip install` + `twine upload`) |


### Artifact Promotion

```
CI publishes to ──▶ libs-dev-local ──▶ libs-staging-local ──▶ libs-release-local
                     (automatic)        (after QA gate)        (immutable, signed)
```

**Promotion copies/moves artifacts between repos** — you never rebuild. Each stage adds verification.

Implementation options:

- JFrog Build Integration + promotion REST API
- AQL + copy/move API
- Conan channels mapping to lifecycle stages

### Jenkins + Artifactory Plugin

```groovy
stage('Publish') {
    steps {
        rtUpload(
            serverId: 'artifactory-prod',
            spec: '''{
                "files": [{
                    "pattern": "dist/*.tar.gz",
                    "target": "generic-local/myproject/${BUILD_NUMBER}/"
                }]
            }'''
        )
    }
}
```

### Conan + Artifactory (C++ / NVIDIA Relevance)

**Conan** manages C++ dependencies (recipes + prebuilt packages per OS/compiler/arch). Artifactory hosts Conan repositories.

```bash
# Add remote pointing to Artifactory virtual repo
conan remote add artifactory https://art.example.com/artifactory/api/conan/conan-virtual

# Upload internal package
conan remote login artifactory "$USER" -p "$TOKEN"
conan upload "mypackage/2.3.0@nvidia/stable" -r artifactory --all
```

**Why this matters at NVIDIA scale:** Large binaries (CUDA, optimized libs), many toolchains, many target architectures — Conan + Artifactory centralizes recipes and prebuilt artifacts.

### AQL (Artifactory Query Language)

Find artifacts by name, property, age, repo:

```json
items.find({
    "repo": "generic-local",
    "path": {"$match": "myproject/*"},
    "@build.number": {"$eq": "123"}
}).include("name", "repo", "path", "actual_sha1")
```

Use cases: cleanup jobs, finding promotion candidates, compliance reports.

### REST API

```bash
# Upload
curl -u "$USER:$TOKEN" -T ./sdk.tar.gz \
  "https://art.example.com/artifactory/generic-local/sdk/1.4.0/sdk.tar.gz"

# Set properties
curl -u "$USER:$TOKEN" -X PUT \
  "https://art.example.com/artifactory/api/storage/generic-local/sdk/1.4.0/sdk.tar.gz?properties=build.number=42;vcs.revision=abc1234"

# Copy (promotion)
curl -u "$USER:$TOKEN" -X POST \
  "https://art.example.com/artifactory/api/copy/generic-local/sdk/1.4.0?to=/release-local/sdk/1.4.0"
```

### Versioning and Retention

- **Semver** for libraries; build metadata (`1.2.3+build.45`) for traceability
- **Immutable releases:** forbid overwrites on release repos
- **Cleanup policies:** delete old snapshots, keep N latest per branch, time-based TTL
- Balance storage cost vs debuggability

---

## 14. Hands-On Exercises

### Ansible Exercises

**1.** Create `hosts.yml` with `build_agents` and `gpu_builders`; override `ansible_user` for one host via `host_vars`.

**2.** Write a playbook that idempotently installs git, cmake, ninja-build. Use `tags: toolchain`.

**3.** Use a template to render a config file. Verify the handler (service restart) fires only when the template content changes.

**4.** Encrypt a vault file with an API token. Run the playbook with `--vault-password-file`.

**5.** Convert your toolchain playbook into a role (`roles/build_toolchain`). Parameterize the package list via `defaults/main.yml`.

### Artifactory Exercises

**6.** Sketch a repo layout: local/remote/virtual for Conan + Docker + generic SDKs.

**7.** Use `curl` to upload a tarball and attach properties `build.number` and `vcs.revision`.

**8.** Write an AQL query finding artifacts in a path older than 30 days.

**9.** Write a Jenkins pipeline stage that uploads `dist/`* to a generic repo.

---

## 15. Interview Questions

**Q1: Why Ansible over shell scripts for build infra?**
Ansible provides idempotent modules, inventory grouping, secrets via Vault, and YAML playbooks in Git. Shell scripts tend to be non-idempotent, hard to test, and risky to run repeatedly.

**Q2: Static vs dynamic inventory?**
Static is checked into Git, suits stable hostnames. Dynamic queries cloud APIs so new build VMs appear automatically. Combine both: dynamic for cloud agents, static for fixed GPU builders.

**Q3: `command` vs `shell` module?**
`command` bypasses the shell (no pipes). `shell` runs through `/bin/sh`. Prefer `command` or dedicated modules; use `shell` only when you need pipes/redirection.

**Q4: How do handlers work?**
They run once at end of play if notified. Avoids restarting a service multiple times when several tasks change its config.

**Q5: Variable precedence — what wins?**
Extra vars (`-e`) override everything. Then task/block/role vars. Then host/group vars. Role defaults are lowest.

**Q6: What is Ansible Vault?**
Encrypts sensitive YAML files at rest. In CI: provide password via Jenkins credential, written to temp file, passed as `--vault-password-file`.

**Q7: What does idempotency mean? Non-idempotent example?**
Running multiple times converges to same state. Non-idempotent: `shell: echo x >> file` appends every time. Fix with `lineinfile` or `copy`.

**Q8: How to install CUDA with Ansible?**
Prefer vendor packages or internal apt repos mirrored through Artifactory. If only a `.run` installer exists, use `command` with `creates:` pointing to an installed path marker.

**Q9: Artifactory repository types?**
Local stores your artifacts. Remote proxies upstream (cache). Virtual merges them so clients use one URL.

**Q10: What is artifact promotion?**
Moving immutable build outputs through quality gates (dev → staging → release) without rebuilding. Each gate adds verification.

**Q11: Why Conan + Artifactory for C++?**
Conan models binary packages per settings (OS, compiler, arch, CUDA version). Artifactory stores recipes and binaries. Virtual repos proxy ConanCenter while keeping private packages internal.

**Q12: What is AQL?**
Artifactory Query Language for finding artifacts by repo, path, properties, checksum, age. Used for cleanup, promotion candidates, compliance reports.

**Q13: How to integrate Jenkins with Artifactory securely?**
JFrog plugin or jfrog CLI with scoped tokens stored in Jenkins credentials. Upload via spec files. Attach build-info for traceability.

**Q14: `block`/`rescue`/`always` vs `ignore_errors`?**
`ignore_errors` suppresses failure for one task (can hide bugs). `block`/`rescue`/`always` gives structured error handling — rescue for recovery, always for cleanup.

**Q15: How to keep build infra config drift-free?**
Run Ansible on a schedule (not just on changes). Use `--check` (dry run) for auditing. Pin versions in playbooks. Test playbooks against fresh VMs.