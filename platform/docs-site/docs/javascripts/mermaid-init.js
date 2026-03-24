(function () {
  function cssVar(name, fallback) {
    var value = getComputedStyle(document.documentElement).getPropertyValue(name);
    if (!value) {
      return fallback;
    }
    value = value.trim();
    return value || fallback;
  }

  function mermaidThemeOptions() {
    var scheme = document.body.getAttribute("data-md-color-scheme") || "default";
    var dark = scheme === "slate";

    return {
      startOnLoad: false,
      theme: "base",
      securityLevel: "strict",
      themeVariables: {
        fontFamily: "IBM Plex Sans, Inter, Segoe UI, sans-serif",
        primaryColor: cssVar("--md-primary-fg-color", dark ? "#38bdf8" : "#0ea5e9"),
        primaryTextColor: dark ? "#e5e7eb" : "#0f172a",
        primaryBorderColor: dark ? "#7dd3fc" : "#0284c7",
        lineColor: dark ? "#94a3b8" : "#334155",
        tertiaryColor: cssVar("--vfo-surface", dark ? "#111827" : "#ffffff"),
        mainBkg: cssVar("--vfo-surface", dark ? "#111827" : "#ffffff"),
        secondBkg: cssVar("--vfo-bg", dark ? "#0f172a" : "#f8fafc"),
        textColor: dark ? "#e5e7eb" : "#0f172a"
      }
    };
  }

  function renderMermaid() {
    if (typeof mermaid === "undefined") {
      return;
    }

    mermaid.initialize(mermaidThemeOptions());

    var blocks = document.querySelectorAll("div.mermaid");
    if (!blocks.length) {
      return;
    }

    for (var i = 0; i < blocks.length; i += 1) {
      blocks[i].removeAttribute("data-processed");
    }

    mermaid.run({
      querySelector: "div.mermaid"
    });
  }

  if (window.document$ && typeof window.document$.subscribe === "function") {
    window.document$.subscribe(function () {
      renderMermaid();
    });
    return;
  }

  document.addEventListener("DOMContentLoaded", function () {
    renderMermaid();
  });
})();
