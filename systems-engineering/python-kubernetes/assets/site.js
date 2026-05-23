/* =====================================================================
   site.js — nav, code copy, progress, and syntax highlighting for
             YAML, Groovy (Jenkinsfile), Shell, JSON.
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

  // ---------- Progress ----------
  const TOTAL_CHAPTERS = 17;
  const key = 'python-k8s-tutorial:visited';
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

  // ---------- Copy buttons ----------
  document.querySelectorAll('.code').forEach(block => {
    const pre = block.querySelector('pre');
    if (!pre) return;
    let hdr = block.querySelector('.code-hdr');
    if (!hdr) {
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
      const text = pre.innerText.replace(/^\$ /gm, '');
      navigator.clipboard.writeText(text).then(() => {
        btn.textContent = 'copied';
        btn.classList.add('done');
        setTimeout(() => { btn.textContent = 'copy'; btn.classList.remove('done'); }, 1500);
      }).catch(() => { btn.textContent = 'err'; });
    });
  });

  // ---------- Syntax highlighters ----------

  function escapeHTML(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  // --- YAML ---
  function highlightYAML(src) {
    return src.split('\n').map(line => {
      // Comment (not inside a string)
      const cmt = line.match(/^(.*?)(\s*#.*)$/);
      let body = line, tail = '';
      if (cmt && !/["']/.test(cmt[1].split('#')[0])) {
        body = cmt[1];
        tail = `<span class="tok-cmt">${escapeHTML(cmt[2])}</span>`;
      }

      // Document separator / directives
      if (/^---\s*$/.test(body) || /^\.\.\.\s*$/.test(body)) {
        return `<span class="tok-kw">${escapeHTML(body)}</span>${tail}`;
      }

      // Key: value
      const kv = body.match(/^(\s*-?\s*)([A-Za-z_][\w\-\.]*|"[^"]*"|'[^']*')(\s*:)(\s?)(.*)$/);
      if (kv) {
        const [, lead, k, colon, sp, val] = kv;
        return escapeHTML(lead) +
               `<span class="tok-var">${escapeHTML(k)}</span>` +
               `<span class="tok-op">${colon}</span>` +
               sp +
               highlightYAMLValue(val) +
               tail;
      }

      // List item without key
      const li = body.match(/^(\s*)(-\s*)(.*)$/);
      if (li) {
        return escapeHTML(li[1]) + `<span class="tok-op">${li[2]}</span>` +
               highlightYAMLValue(li[3]) + tail;
      }

      return escapeHTML(body) + tail;
    }).join('\n');
  }

  function highlightYAMLValue(v) {
    if (v === '') return '';
    // strings
    if (/^["']/.test(v)) {
      const m = v.match(/^(["'])(.*?)\1(.*)$/);
      if (m) return `<span class="tok-str">${escapeHTML(m[1] + m[2] + m[1])}</span>` + escapeHTML(m[3]);
    }
    // booleans, null
    if (/^(true|false|null|yes|no|on|off|~)\s*$/i.test(v)) {
      return `<span class="tok-kw">${escapeHTML(v)}</span>`;
    }
    // numbers
    if (/^-?\d+(\.\d+)?\s*$/.test(v)) {
      return `<span class="tok-num">${escapeHTML(v)}</span>`;
    }
    // GitHub Actions / GitLab references: $CI_ , ${{ … }}
    let out = escapeHTML(v);
    out = out.replace(/(\$\{\{[^}]*\}\})/g, '<span class="tok-fn">$1</span>');
    out = out.replace(/(\$\{?[A-Z_][A-Z0-9_]*\}?)/g, '<span class="tok-var">$1</span>');
    return out;
  }

  // --- Groovy (Jenkinsfile) ---
  const GROOVY_KW = new Set([
    'def','class','if','else','for','while','do','switch','case','break',
    'continue','return','try','catch','finally','throw','new','null','true',
    'false','void','int','String','boolean','double','float','long','as',
    'in','import','package','static','final','public','private','protected',
    'pipeline','stages','stage','steps','agent','post','always','success',
    'failure','unstable','aborted','changed','environment','options','parameters',
    'triggers','when','parallel','script','node','sh','bat','dir','withEnv',
    'withCredentials','checkout','git','echo','error','input','timeout',
    'retry','milestone','lock','build','readFile','writeFile','tools','matrix',
    'axes','axis','values','excludes','exclude'
  ]);
  // Linear tokenizer: processes strings and comments as atomic units so
  // their inner content is never re-scanned (which was the source of
  // visible "S", "K", "/" leakage from \x01-marker residue).
  function highlightGroovy(src) {
    let out = '';
    let i = 0;
    const n = src.length;

    while (i < n) {
      const rest2 = src.substr(i, 2);
      const rest3 = src.substr(i, 3);
      const c = src[i];

      // Block comment
      if (rest2 === '/*') {
        let j = src.indexOf('*/', i + 2);
        j = j === -1 ? n : j + 2;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Line comment
      if (rest2 === '//') {
        let j = src.indexOf('\n', i);
        if (j === -1) j = n;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Triple-quoted string (Groovy """...""" or '''...''')
      if (rest3 === '"""' || rest3 === "'''") {
        const q = rest3;
        let j = src.indexOf(q, i + 3);
        j = j === -1 ? n : j + 3;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Single or double quoted string
      if (c === '"' || c === "'") {
        let j = i + 1;
        while (j < n && src[j] !== c) {
          if (src[j] === '\\' && j + 1 < n) j += 2;
          else if (src[j] === '\n') break; // don't eat across lines for '..' / ".."
          else j++;
        }
        if (j < n && src[j] === c) j++;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
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
        continue;
      }
      // Identifier / keyword
      if (/[A-Za-z_]/.test(c)) {
        let j = i;
        while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++;
        const word = src.substring(i, j);
        if (GROOVY_KW.has(word)) {
          out += `<span class="tok-kw">${word}</span>`;
        } else {
          out += word;
        }
        i = j;
        continue;
      }
      // Plain char
      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  // --- Shell ---
  const SH_KW = new Set([
    'if','then','else','elif','fi','for','in','do','done','while','until',
    'case','esac','function','return','local','export','set','unset','read',
    'test','&&','||','|'
  ]);
  const SH_BUILTIN = new Set([
    'echo','printf','cd','pwd','ls','cat','grep','sed','awk','find','touch',
    'mkdir','rm','cp','mv','chmod','chown','curl','wget','docker','kubectl',
    'helm','git','make','npm','yarn','node','python','python3','pip','bash',
    'ssh','scp','rsync','jq','yq','tar','gzip','unzip','aws','gcloud','az'
  ]);
  function highlightShell(src) {
    return src.split('\n').map(line => {
      const prm = line.match(/^(\$ |# |\$  )(.*)$/);
      if (prm) return `<span class="prompt">${prm[1]}</span>` + highlightShellLine(prm[2]);
      if (line.startsWith('#')) return `<span class="tok-cmt">${escapeHTML(line)}</span>`;
      return `<span class="out">${escapeHTML(line)}</span>`;
    }).join('\n');
  }
  function highlightShellLine(line) {
    // Comment
    let idx = -1, depth = 0;
    for (let i = 0; i < line.length; i++) {
      const c = line[i];
      if (c === '"' || c === "'") {
        const q = c;
        i++;
        while (i < line.length && line[i] !== q) { if (line[i] === '\\') i++; i++; }
      } else if (c === '#') { idx = i; break; }
    }
    let tail = '';
    if (idx >= 0) { tail = `<span class="tok-cmt">${escapeHTML(line.slice(idx))}</span>`; line = line.slice(0, idx); }
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
      const wm = line.slice(i).match(/^[A-Za-z_][A-Za-z0-9_-]*/);
      if (wm) {
        const w = wm[0];
        if (SH_KW.has(w))             out += `<span class="tok-kw">${w}</span>`;
        else if (SH_BUILTIN.has(w))   out += `<span class="tok-fn">${w}</span>`;
        else                          out += escapeHTML(w);
        i += w.length; continue;
      }
      out += escapeHTML(c);
      i++;
    }
    return out + tail;
  }

  // --- Python ---
  const PY_KW = new Set([
    'False','None','True','and','as','assert','async','await','break','class',
    'continue','def','del','elif','else','except','finally','for','from','global',
    'if','import','in','is','lambda','nonlocal','not','or','pass','raise','return',
    'try','while','with','yield','match','case'
  ]);
  const PY_BUILTIN = new Set([
    'print','len','range','enumerate','zip','map','filter','sorted','reversed',
    'sum','min','max','abs','round','any','all','int','float','str','bool',
    'list','tuple','dict','set','frozenset','bytes','bytearray','type','isinstance',
    'issubclass','super','open','input','format','repr','hash','id','iter','next',
    'object','property','staticmethod','classmethod','dataclass','Exception',
    'ValueError','TypeError','KeyError','IndexError','RuntimeError','OSError',
    'FileNotFoundError','NotImplementedError','StopIteration','self','cls'
  ]);
  function highlightPython(src) {
    let out = '';
    let i = 0;
    const n = src.length;
    while (i < n) {
      const c = src[i];
      const r2 = src.substr(i, 2);
      const r3 = src.substr(i, 3);

      // Triple-quoted strings (and docstrings)
      if (r3 === '"""' || r3 === "'''") {
        const q = r3;
        let j = src.indexOf(q, i + 3);
        j = j === -1 ? n : j + 3;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // f-string / r-string / b-string prefixes
      if (/[fFrRbBuU]/.test(c) && (src[i+1] === '"' || src[i+1] === "'")) {
        let j = i + 1;
        const q = src[j];
        j++;
        while (j < n && src[j] !== q) {
          if (src[j] === '\\' && j + 1 < n) j += 2;
          else if (src[j] === '\n') break;
          else j++;
        }
        if (j < n) j++;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Plain string
      if (c === '"' || c === "'") {
        const q = c;
        let j = i + 1;
        while (j < n && src[j] !== q) {
          if (src[j] === '\\' && j + 1 < n) j += 2;
          else if (src[j] === '\n') break;
          else j++;
        }
        if (j < n && src[j] === q) j++;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Comment
      if (c === '#') {
        let j = src.indexOf('\n', i);
        if (j === -1) j = n;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Decorator
      if (c === '@' && /[A-Za-z_]/.test(src[i+1] || '')) {
        let j = i + 1;
        while (j < n && /[A-Za-z0-9_\.]/.test(src[j])) j++;
        out += `<span class="tok-fn">${escapeHTML(src.substring(i, j))}</span>`;
        i = j;
        continue;
      }
      // Number
      if (/\d/.test(c) && (i === 0 || !/[A-Za-z_0-9]/.test(src[i - 1]))) {
        let j = i;
        while (j < n && /[\d_xXa-fA-F]/.test(src[j])) j++;
        if (src[j] === '.' && /\d/.test(src[j + 1])) {
          j++;
          while (j < n && /[\d_]/.test(src[j])) j++;
        }
        out += `<span class="tok-num">${src.substring(i, j)}</span>`;
        i = j;
        continue;
      }
      // Identifier / keyword
      if (/[A-Za-z_]/.test(c)) {
        let j = i;
        while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++;
        const word = src.substring(i, j);
        if (PY_KW.has(word))           out += `<span class="tok-kw">${word}</span>`;
        else if (PY_BUILTIN.has(word)) out += `<span class="tok-fn">${word}</span>`;
        else                            out += word;
        i = j;
        continue;
      }
      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  // --- JSON ---
  function highlightJSON(src) {
    return src
      .replace(/</g, '&lt;').replace(/>/g, '&gt;')
      .replace(/"([^"\\]|\\.)*"(\s*:)/g, '<span class="tok-var">"$&</span>'.replace('$&', '$&').replace(/<\/span>(\s*:)/, ':</span>'))
      .replace(/"([^"\\]|\\.)*"/g, '<span class="tok-str">$&</span>')
      .replace(/\b(true|false|null)\b/g, '<span class="tok-kw">$&</span>')
      .replace(/\b-?\d+(\.\d+)?\b/g, '<span class="tok-num">$&</span>');
  }

  // Dispatch
  document.querySelectorAll('.code pre code').forEach(code => {
    const block = code.closest('.code');
    const lang = (block.dataset.lang || code.className || '').toLowerCase();
    const src = code.textContent;
    if (/yaml|yml/.test(lang))                      code.innerHTML = highlightYAML(src);
    else if (/python|py\b/.test(lang))              code.innerHTML = highlightPython(src);
    else if (/groovy|jenkins/.test(lang))           code.innerHTML = highlightGroovy(src);
    else if (/^(sh|bash|shell|term)/.test(lang) || block.classList.contains('terminal'))
                                                    code.innerHTML = highlightShell(src);
    else if (/json/.test(lang))                     code.innerHTML = highlightJSON(src);
  });

  // Heading anchors
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
