#pragma once

namespace WebBridge
{

// Host page for the locally served Strudel REPL. The Strudel build itself
// (index.js + workers) is fetched at runtime, see StrudelFetch.h.
inline const char* getStrudelPage()
{
    return R"HTML(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Strudel (offline)</title>
<style>
  html, body { margin: 0; min-height: 100%; background: #222; color: #eee; }
  strudel-editor { display: none; }   /* editor is inserted as the next sibling */
  .cm-editor { min-height: 100vh; }
</style>
</head>
<body>
<strudel-editor>
<!--
// offline: synth sounds only, samples need internet
note("c3 eb3 g3 bb3").s("sawtooth").lpf(800)
-->
</strudel-editor>
<script src="index.js"></script>
<script>
  // Bridge script looks for window.strudelMirror (code persistence, DAW sync).
  // Once the editor exists, ask the plugin for the code saved in the DAW project.
  const el = document.querySelector('strudel-editor');
  const wait = setInterval(() => {
    if (!el.editor) return;
    clearInterval(wait);
    window.strudelMirror = el.editor;
    window.__JUCE__?.backend?.emitEvent('requestCode', {});
  }, 100);
</script>
</body>
</html>
)HTML";
}

}
