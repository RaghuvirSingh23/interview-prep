/* =====================================================================
   site.js — nav, code copy, progress, syntax highlighting for
             C, C++, Shell, Make, GDB, ASM. Linear tokenizers (no
             marker-substitution leakage).
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

  // ---------- Highlight current page ----------
  const path = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.sidebar a').forEach(a => {
    const href = a.getAttribute('href') || '';
    const file = href.split('/').pop();
    if (file === path) a.classList.add('current');
  });

  // ---------- Progress ----------
  const TOTAL_CHAPTERS = 14;
  const key = 'c-linux-debug:visited';
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
      const text = pre.innerText.replace(/^\$ /gm, '').replace(/^\(gdb\) /gm, '');
      navigator.clipboard.writeText(text).then(() => {
        btn.textContent = 'copied';
        btn.classList.add('done');
        setTimeout(() => { btn.textContent = 'copy'; btn.classList.remove('done'); }, 1500);
      }).catch(() => { btn.textContent = 'err'; });
    });
  });

  // ---------- Highlighters ----------

  function escapeHTML(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  // ----- C / C++ -----
  const C_KW = new Set([
    // C
    'auto','break','case','char','const','continue','default','do','double',
    'else','enum','extern','float','for','goto','if','inline','int','long',
    'register','restrict','return','short','signed','sizeof','static','struct',
    'switch','typedef','union','unsigned','void','volatile','while','_Bool',
    '_Atomic','_Alignas','_Alignof','_Generic','_Noreturn','_Static_assert',
    '_Thread_local','bool','true','false','NULL',
    // C++
    'class','public','private','protected','namespace','using','template',
    'typename','virtual','override','final','new','delete','this','nullptr',
    'try','catch','throw','noexcept','constexpr','consteval','constinit',
    'decltype','friend','operator','explicit','mutable','export','static_cast',
    'dynamic_cast','reinterpret_cast','const_cast','typeid','asm','and','or',
    'not','xor','bitand','bitor','compl','co_await','co_return','co_yield',
    'concept','requires','module','import','thread_local','alignas','alignof'
  ]);
  function highlightC(src) {
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
        i = j; continue;
      }
      // Line comment
      if (r2 === '//') {
        let j = src.indexOf('\n', i);
        if (j === -1) j = n;
        out += `<span class="tok-cmt">${escapeHTML(src.substring(i, j))}</span>`;
        i = j; continue;
      }
      // Strings (incl. R"..." raw, L"...", u8"..." prefixes)
      if (c === '"' || c === "'" ||
          (/[LuU]/.test(c) && (src[i+1] === '"' || src[i+1] === "'")) ||
          (c === 'u' && src[i+1] === '8' && (src[i+2] === '"' || src[i+2] === "'"))) {
        // skip prefix
        let p = i;
        while (p < n && /[LuU8]/.test(src[p])) p++;
        const q = src[p];
        let j = p + 1;
        while (j < n && src[j] !== q) {
          if (src[j] === '\\' && j + 1 < n) j += 2;
          else if (src[j] === '\n') break;
          else j++;
        }
        if (j < n && src[j] === q) j++;
        out += `<span class="tok-str">${escapeHTML(src.substring(i, j))}</span>`;
        i = j; atLineStart = false; continue;
      }
      // Preprocessor — only at line start (after optional whitespace)
      if (atLineStart && c === '#') {
        let j = i;
        while (j < n) {
          if (src[j] === '\n' && src[j-1] !== '\\') break;
          j++;
        }
        out += `<span class="tok-kw">${escapeHTML(src.substring(i, j))}</span>`;
        i = j; continue;
      }
      // Numbers
      if (/\d/.test(c) && (i === 0 || !/[A-Za-z_0-9]/.test(src[i - 1]))) {
        let j = i;
        while (j < n && /[\dxXa-fA-FoOpPlLuU._']/.test(src[j])) j++;
        out += `<span class="tok-num">${src.substring(i, j)}</span>`;
        i = j; atLineStart = false; continue;
      }
      // Identifier / keyword
      if (/[A-Za-z_]/.test(c)) {
        let j = i;
        while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++;
        const w = src.substring(i, j);
        if (C_KW.has(w))                    out += `<span class="tok-kw">${w}</span>`;
        else if (/^[A-Z_][A-Z0-9_]+$/.test(w)) out += `<span class="tok-num">${w}</span>`;  // MACRO_LIKE
        else                                 out += w;
        i = j; atLineStart = false; continue;
      }
      // line tracking
      if (c === '\n') atLineStart = true;
      else if (!/\s/.test(c)) atLineStart = false;
      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  // ----- Shell + GDB transcript -----
  const SH_KW = new Set([
    'if','then','else','elif','fi','for','in','do','done','while','until',
    'case','esac','function','return','local','export','set','unset','read'
  ]);
  const SH_BUILTIN = new Set([
    'echo','printf','cd','pwd','ls','cat','grep','sed','awk','find','touch',
    'mkdir','rm','cp','mv','chmod','chown','curl','wget','make','gcc','g++',
    'clang','clang++','cmake','ninja','perf','strace','ltrace','gdb','lldb',
    'objdump','readelf','nm','addr2line','valgrind','ldd','file','dmesg',
    'ssh','scp','rsync','tar','gzip','docker','kubectl','python','python3',
    'bash','sh','sudo','time','env'
  ]);
  function highlightShell(src) {
    return src.split('\n').map(line => {
      const gdb = line.match(/^(\(gdb\) )(.*)$/);
      if (gdb) return `<span class="prompt">${gdb[1]}</span>` + highlightShellLine(gdb[2]);
      const lldb = line.match(/^(\(lldb\) )(.*)$/);
      if (lldb) return `<span class="prompt">${lldb[1]}</span>` + highlightShellLine(lldb[2]);
      const prm = line.match(/^(\$ |# )(.*)$/);
      if (prm) return `<span class="prompt">${prm[1]}</span>` + highlightShellLine(prm[2]);
      if (line.startsWith('#')) return `<span class="tok-cmt">${escapeHTML(line)}</span>`;
      return `<span class="out">${escapeHTML(line)}</span>`;
    }).join('\n');
  }
  function highlightShellLine(line) {
    let out = '';
    let i = 0;
    const n = line.length;
    while (i < n) {
      const c = line[i];
      if (c === '#') { out += `<span class="tok-cmt">${escapeHTML(line.slice(i))}</span>`; break; }
      if (c === "'" || c === '"') {
        const q = c; let j = i + 1;
        while (j < n && line[j] !== q) { if (line[j] === '\\') j++; j++; }
        if (j < n) j++;
        out += `<span class="tok-str">${escapeHTML(line.slice(i, j))}</span>`;
        i = j; continue;
      }
      if (c === '$' && line[i+1] === '{') {
        let j = i + 2; while (j < n && line[j] !== '}') j++;
        out += `<span class="tok-var">${escapeHTML(line.slice(i, j + 1))}</span>`;
        i = j + 1; continue;
      }
      if (c === '$' && /[A-Za-z_]/.test(line[i+1] || '')) {
        let j = i + 1;
        while (j < n && /[A-Za-z0-9_]/.test(line[j])) j++;
        out += `<span class="tok-var">${escapeHTML(line.slice(i, j))}</span>`;
        i = j; continue;
      }
      const wm = line.slice(i).match(/^[A-Za-z_][A-Za-z0-9_+-]*/);
      if (wm) {
        const w = wm[0];
        if (SH_KW.has(w))           out += `<span class="tok-kw">${w}</span>`;
        else if (SH_BUILTIN.has(w)) out += `<span class="tok-fn">${w}</span>`;
        else                         out += escapeHTML(w);
        i += w.length; continue;
      }
      out += escapeHTML(c);
      i++;
    }
    return out;
  }

  // ----- Assembly (simple) -----
  function highlightAsm(src) {
    return src.split('\n').map(line => {
      const cmt = line.match(/^(.*?)([;#].*)$/);
      let body = cmt ? cmt[1] : line;
      let tail = cmt ? `<span class="tok-cmt">${escapeHTML(cmt[2])}</span>` : '';
      const lbl = body.match(/^(\S+:)(.*)$/);
      if (lbl) return `<span class="tok-fn">${escapeHTML(lbl[1])}</span>${escapeHTML(lbl[2])}${tail}`;
      const ins = body.match(/^(\s+)(\S+)(.*)$/);
      if (ins) return escapeHTML(ins[1]) + `<span class="tok-kw">${escapeHTML(ins[2])}</span>` + escapeHTML(ins[3]) + tail;
      return escapeHTML(body) + tail;
    }).join('\n');
  }

  // ----- Make (reuse from make-tutorial: simple line-based) -----
  function highlightMake(src) {
    return src.split('\n').map(line => {
      if (line.startsWith('#'))
        return `<span class="tok-cmt">${escapeHTML(line)}</span>`;
      if (line.startsWith('\t'))
        return '<span class="tab-glyph"></span>' + escapeHTML(line.slice(1));
      const tg = line.match(/^([^:#]+)(:)([^=]*)$/);
      if (tg) return `<span class="tok-tgt">${escapeHTML(tg[1])}</span><span class="tok-op">${tg[2]}</span>${escapeHTML(tg[3])}`;
      const va = line.match(/^([A-Za-z_][\w]*)(\s*[:?+!]?=)(.*)$/);
      if (va) return `<span class="tok-var">${va[1]}</span><span class="tok-op">${va[2]}</span>${escapeHTML(va[3])}`;
      return escapeHTML(line);
    }).join('\n');
  }

  // Dispatch
  document.querySelectorAll('.code pre code').forEach(code => {
    const block = code.closest('.code');
    const lang = (block.dataset.lang || code.className || '').toLowerCase();
    const src = code.textContent;
    if (/^c\+\+|cpp|cxx|hpp|^c\b/.test(lang)) code.innerHTML = highlightC(src);
    else if (/asm|s\b/.test(lang))             code.innerHTML = highlightAsm(src);
    else if (/make/.test(lang))                code.innerHTML = highlightMake(src);
    else if (/^(sh|bash|shell|term|gdb|lldb)/.test(lang) || block.classList.contains('terminal'))
                                                code.innerHTML = highlightShell(src);
  });

  // ---------- Anchor links ----------
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
