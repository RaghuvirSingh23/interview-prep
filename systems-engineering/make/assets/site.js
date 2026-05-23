/* =====================================================================
   site.js — nav, code copy, tab-char highlighting, progress tracking,
             and a small Makefile/Shell/C syntax highlighter.
   ===================================================================== */

(function () {
  'use strict';

  // ---------- Sidebar toggle (mobile) ----------
  const toggle = document.querySelector('.sidebar-toggle');
  const sidebar = document.querySelector('.sidebar');
  if (toggle && sidebar) {
    toggle.addEventListener('click', () => sidebar.classList.toggle('open'));
    document.addEventListener('click', (e) => {
      if (window.innerWidth > 900) return;
      if (sidebar.contains(e.target) || toggle.contains(e.target)) return;
      sidebar.classList.remove('open');
    });
  }

  // ---------- Highlight current page in sidebar ----------
  const path = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.sidebar a').forEach(a => {
    const href = a.getAttribute('href') || '';
    const file = href.split('/').pop();
    if (file === path) a.classList.add('current');
  });

  // ---------- Progress tracking (marks chapters visited) ----------
  const TOTAL_CHAPTERS = 14;
  const key = 'make-tutorial:visited';
  let visited = new Set(JSON.parse(localStorage.getItem(key) || '[]'));

  const chapMatch = path.match(/^(\d{2})-/);
  if (chapMatch) {
    visited.add(chapMatch[1]);
    localStorage.setItem(key, JSON.stringify([...visited]));
  }

  const progEl = document.querySelector('.progress');
  if (progEl) {
    const n = visited.size;
    progEl.innerHTML = `progress · ${n}/${TOTAL_CHAPTERS}
      <div class="progress-bar"><div style="width:${(n / TOTAL_CHAPTERS) * 100}%"></div></div>`;
  }

  // ---------- Copy buttons on code blocks ----------
  document.querySelectorAll('.code').forEach(block => {
    const pre = block.querySelector('pre');
    if (!pre) return;
    let hdr = block.querySelector('.code-hdr');
    if (!hdr) {
      // auto-create header if missing
      hdr = document.createElement('div');
      hdr.className = 'code-hdr';
      const lang = block.dataset.lang || '';
      hdr.innerHTML = `<span class="lang">${lang}</span><button class="copy">copy</button>`;
      block.insertBefore(hdr, pre);
    } else if (!hdr.querySelector('.copy')) {
      const btn = document.createElement('button');
      btn.className = 'copy';
      btn.textContent = 'copy';
      hdr.appendChild(btn);
    }
    const btn = hdr.querySelector('.copy');
    btn.addEventListener('click', () => {
      const text = pre.innerText
        .replace(/→\u00a0/g, '\t')   // restore visible tab glyphs
        .replace(/^\$ /gm, '');       // drop shell prompts for real-paste
      navigator.clipboard.writeText(text).then(() => {
        btn.textContent = 'copied';
        btn.classList.add('done');
        setTimeout(() => { btn.textContent = 'copy'; btn.classList.remove('done'); }, 1500);
      }).catch(() => { btn.textContent = 'err'; });
    });
  });

  // ---------- Syntax highlighter (Makefile, Shell, C) ----------

  // A light, pragmatic highlighter. Not perfect — this is a tutorial, not
  // a code editor. It handles the constructs we actually show.

  function escapeHTML(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  const MAKE_DIRECTIVES = new Set([
    'include', '-include', 'sinclude', 'ifeq', 'ifneq', 'ifdef', 'ifndef',
    'else', 'endif', 'define', 'endef', 'override', 'export', 'unexport',
    'private', 'vpath', 'undefine'
  ]);

  const MAKE_FUNCS = new Set([
    'subst','patsubst','strip','findstring','filter','filter-out','sort',
    'word','words','wordlist','firstword','lastword','dir','notdir','suffix',
    'basename','addsuffix','addprefix','join','wildcard','realpath','abspath',
    'error','warning','info','shell','origin','flavor','foreach','call','eval',
    'value','if','or','and','not','file','guile'
  ]);

  const MAKE_AUTO = new Set(['@','<','^','?','*','+','|','%']);

  function highlightMake(src) {
    const lines = src.split('\n');
    const out = [];

    for (let line of lines) {
      // Comment
      const cmtIdx = findUnquoted(line, '#');
      let tail = '';
      if (cmtIdx >= 0) {
        tail = `<span class="tok-cmt">${escapeHTML(line.slice(cmtIdx))}</span>`;
        line = line.slice(0, cmtIdx);
      }

      // Recipe line: starts with \t — render visible tab glyph
      if (line.startsWith('\t')) {
        const recipe = line.slice(1);
        // Inside recipe: highlight $(...) and $@ $< etc and shell-ish strings.
        out.push('<span class="tab-glyph"></span>' + highlightMakeExpr(recipe) + tail);
        continue;
      }

      // Directive at line start
      let m = line.match(/^(\s*)(-?[a-zA-Z]+)(\b.*)?$/);
      if (m && MAKE_DIRECTIVES.has(m[2]) && (/^(\s*)/.test(line))) {
        const [_, ws, dir, rest] = m;
        out.push(ws + `<span class="tok-kw">${dir}</span>` + highlightMakeExpr(rest || '') + tail);
        continue;
      }

      // Target line?  `foo bar: baz`   `%.o: %.c`   `$(OBJ): $(DEPS)`
      // Anything with an unquoted ':' that's not := ::= ?= and not inside a fn call.
      const colonIdx = findTargetColon(line);
      if (colonIdx >= 0) {
        const left  = line.slice(0, colonIdx);
        const sep   = line[colonIdx] === ':' && line[colonIdx+1] === ':' ? '::' : ':';
        const right = line.slice(colonIdx + sep.length);
        out.push(`<span class="tok-tgt">${highlightMakeExpr(left)}</span><span class="tok-op">${sep}</span>${highlightMakeExpr(right)}${tail}`);
        continue;
      }

      // Variable assignment: NAME = value, NAME := value, etc.
      m = line.match(/^(\s*)([A-Za-z_][A-Za-z0-9_]*)\s*(\+=|\?=|::=|:=|!=|=)(.*)$/);
      if (m) {
        const [_, ws, name, op, val] = m;
        out.push(ws + `<span class="tok-var">${name}</span> <span class="tok-op">${op}</span>${highlightMakeExpr(val)}${tail}`);
        continue;
      }

      out.push(highlightMakeExpr(line) + tail);
    }
    return out.join('\n');
  }

  // Find the index of a '#' that starts a comment (not inside $()).
  function findUnquoted(line, ch) {
    let depth = 0;
    for (let i = 0; i < line.length; i++) {
      const c = line[i];
      if (c === '\\' && i + 1 < line.length) { i++; continue; }
      if (c === '(' || c === '{') depth++;
      else if (c === ')' || c === '}') depth--;
      else if (c === ch && depth <= 0) return i;
    }
    return -1;
  }

  // Find ':' that's a target separator — i.e., not inside $(...) and not
  // part of := / ::= / ?= assignments.
  function findTargetColon(line) {
    let depth = 0;
    for (let i = 0; i < line.length; i++) {
      const c = line[i];
      if (c === '(' || c === '{') depth++;
      else if (c === ')' || c === '}') depth--;
      else if (c === ':' && depth === 0) {
        // Peek: if this is part of := or ::= or ?= then it's an assignment.
        const rest = line.slice(i);
        if (/^:=|^::=/.test(rest)) return -1;
        // if there's a '=' somewhere before this on the line, probably assignment
        // but handled above.
        return i;
      } else if (c === '=' && depth === 0) {
        // if we hit '=' before ':', it's a plain assignment
        return -1;
      }
    }
    return -1;
  }

  // Highlight $(...) references, $@ etc, quoted strings, shell pipes inside.
  function highlightMakeExpr(s) {
    let out = '';
    let i = 0;
    while (i < s.length) {
      const c = s[i];

      // escape
      if (c === '\\' && i + 1 < s.length) {
        out += escapeHTML(c + s[i+1]);
        i += 2;
        continue;
      }

      // strings (double or single)
      if (c === '"' || c === "'") {
        let j = i + 1;
        while (j < s.length && s[j] !== c) {
          if (s[j] === '\\') j++;
          j++;
        }
        out += `<span class="tok-str">${escapeHTML(s.slice(i, j + 1))}</span>`;
        i = j + 1;
        continue;
      }

      // $(fn args) or ${fn args} or $V or $@
      if (c === '$' && i + 1 < s.length) {
        const n = s[i + 1];
        if (n === '(' || n === '{') {
          const close = n === '(' ? ')' : '}';
          let depth = 1;
          let j = i + 2;
          while (j < s.length && depth > 0) {
            if (s[j] === '(' || s[j] === '{') depth++;
            else if (s[j] === ')' || s[j] === '}') depth--;
            if (depth > 0) j++;
          }
          const inner = s.slice(i + 2, j);
          // split function name if present
          const fnMatch = inner.match(/^([a-zA-Z_-][a-zA-Z0-9_-]*)(\s+)([\s\S]*)$/);
          if (fnMatch && MAKE_FUNCS.has(fnMatch[1])) {
            out += `<span class="tok-op">$${n}</span><span class="tok-fn">${fnMatch[1]}</span>${fnMatch[2]}${highlightMakeExpr(fnMatch[3])}<span class="tok-op">${close}</span>`;
          } else {
            out += `<span class="tok-op">$${n}</span><span class="tok-var">${highlightMakeExpr(inner)}</span><span class="tok-op">${close}</span>`;
          }
          i = j + 1;
          continue;
        } else if (MAKE_AUTO.has(n)) {
          out += `<span class="tok-au">$${escapeHTML(n)}</span>`;
          i += 2;
          continue;
        } else if (/[A-Za-z_]/.test(n)) {
          out += `<span class="tok-var">$${n}</span>`;
          i += 2;
          continue;
        }
      }

      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  function highlightShell(src) {
    const kw = new Set([
      'if','then','else','elif','fi','for','in','do','done','while','until',
      'case','esac','function','return','local','export','echo','cd','exit',
      'set','unset','read','test','[','[[',']',']]','&&','||','|'
    ]);
    const builtins = new Set([
      'echo','printf','cd','pwd','ls','cat','grep','sed','awk','find','touch',
      'mkdir','rm','cp','mv','chmod','chown','make','gcc','cc','clang','g++',
      'time','env','source','which'
    ]);
    return src.split('\n').map(line => {
      // Prompt
      const prm = line.match(/^(\$ |# |\$  )(.*)$/);
      if (prm) {
        return `<span class="prompt">${prm[1]}</span>` + highlightShellLine(prm[2], kw, builtins);
      }
      // Output (lines without $) — dim them unless it's a comment
      if (line.startsWith('#')) {
        return `<span class="tok-cmt">${escapeHTML(line)}</span>`;
      }
      // Otherwise treat as output
      return `<span class="out">${escapeHTML(line)}</span>`;
    }).join('\n');
  }
  function highlightShellLine(line, kw, builtins) {
    // Comment
    const idx = findUnquoted(line, '#');
    let tail = '';
    if (idx >= 0) { tail = `<span class="tok-cmt">${escapeHTML(line.slice(idx))}</span>`; line = line.slice(0, idx); }
    // Strings
    let out = '';
    let i = 0;
    while (i < line.length) {
      const c = line[i];
      if (c === "'" || c === '"') {
        let j = i + 1;
        while (j < line.length && line[j] !== c) { if (line[j] === '\\') j++; j++; }
        out += `<span class="tok-str">${escapeHTML(line.slice(i, j + 1))}</span>`;
        i = j + 1; continue;
      }
      if (c === '$' && line[i+1] === '{') {
        let j = i + 2;
        while (j < line.length && line[j] !== '}') j++;
        out += `<span class="tok-var">${escapeHTML(line.slice(i, j + 1))}</span>`;
        i = j + 1; continue;
      }
      if (c === '$' && /[A-Za-z_]/.test(line[i+1] || '')) {
        let j = i + 1;
        while (j < line.length && /[A-Za-z0-9_]/.test(line[j])) j++;
        out += `<span class="tok-var">${escapeHTML(line.slice(i, j))}</span>`;
        i = j; continue;
      }
      // words
      const wm = line.slice(i).match(/^[A-Za-z_][A-Za-z0-9_-]*/);
      if (wm) {
        const w = wm[0];
        if (kw.has(w))              out += `<span class="tok-kw">${w}</span>`;
        else if (builtins.has(w))   out += `<span class="tok-fn">${w}</span>`;
        else                         out += escapeHTML(w);
        i += w.length; continue;
      }
      out += escapeHTML(c);
      i++;
    }
    return out + tail;
  }

  // Linear tokenizer (strings & comments are atomic, their inner content
  // is never re-scanned — avoids marker leakage if a string contains `//`
  // or a comment contains `"`).
  function highlightC(src) {
    const kw = new Set([
      'auto','break','case','char','const','continue','default','do','double',
      'else','enum','extern','float','for','goto','if','inline','int','long',
      'register','restrict','return','short','signed','sizeof','static','struct',
      'switch','typedef','union','unsigned','void','volatile','while','_Bool',
      'bool','true','false','NULL'
    ]);
    let out = '';
    let i = 0;
    const n = src.length;
    let atLineStart = true;

    while (i < n) {
      const c = src[i];
      const r2 = src.substr(i, 2);

      // Block comment
      if (r2 === '/*') {
        let j = src.indexOf('*/', i + 2);
        j = j === -1 ? n : j + 2;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Line comment
      if (r2 === '//') {
        let j = src.indexOf('\n', i);
        if (j === -1) j = n;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // String or char literal
      if (c === '"' || c === "'") {
        let j = i + 1;
        while (j < n && src[j] !== c) {
          if (src[j] === '\\' && j + 1 < n) j += 2;
          else if (src[j] === '\n') break;
          else j++;
        }
        if (j < n && src[j] === c) j++;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        atLineStart = false;
        continue;
      }
      // Preprocessor directive (at start of line, ignoring whitespace)
      if (atLineStart && c === '#') {
        let j = i;
        while (j < n && src[j] !== '\n') j++;
        out += `<span class="tok-kw">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Number
      if (/\d/.test(c) && (i === 0 || !/[A-Za-z_0-9]/.test(src[i - 1]))) {
        let j = i;
        while (j < n && /[\d]/.test(src[j])) j++;
        if (src[j] === '.' && /\d/.test(src[j + 1])) {
          j++;
          while (j < n && /\d/.test(src[j])) j++;
        }
        out += `<span class="tok-num">${src.substring(i, j)}</span>`;
        i = j;
        atLineStart = false;
        continue;
      }
      // Identifier / keyword
      if (/[A-Za-z_]/.test(c)) {
        let j = i;
        while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++;
        const word = src.substring(i, j);
        if (kw.has(word)) out += `<span class="tok-kw">${word}</span>`;
        else              out += word;
        i = j;
        atLineStart = false;
        continue;
      }

      // Track "at start of line" for preprocessor detection
      if (c === '\n') atLineStart = true;
      else if (!/\s/.test(c)) atLineStart = false;

      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  document.querySelectorAll('.code pre code').forEach(code => {
    const block = code.closest('.code');
    const lang = block.dataset.lang || code.className || '';
    const src = code.textContent;
    if (/make/i.test(lang))              code.innerHTML = highlightMake(src);
    else if (/^(sh|bash|shell|term)/i.test(lang) || block.classList.contains('terminal'))
                                         code.innerHTML = highlightShell(src);
    else if (/^c\b|c\+\+/i.test(lang))   code.innerHTML = highlightC(src);
  });

  // ---------- Anchor-hover link visibility for headings ----------
  document.querySelectorAll('h2[id], h3[id]').forEach(h => {
    const a = document.createElement('a');
    a.href = '#' + h.id;
    a.textContent = '§';
    a.style.marginLeft = '8px';
    a.style.color = 'var(--text-faint)';
    a.style.opacity = '0';
    a.style.borderBottom = 'none';
    a.style.transition = 'opacity 0.15s';
    h.appendChild(a);
    h.addEventListener('mouseenter', () => a.style.opacity = '0.6');
    h.addEventListener('mouseleave', () => a.style.opacity = '0');
  });
})();
