(function () {
  function renderMermaid() {
    if (typeof mermaid === "undefined") {
      return;
    }

    mermaid.initialize({
      startOnLoad: false
    });

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
