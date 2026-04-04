(function () {
  "use strict";

  var CARD_CLASS = "vfo-diagram-card";
  var CONTENT_CLASS = "vfo-diagram-card__content";

  function downloadDataUrl(filename, dataUrl) {
    var link = document.createElement("a");
    link.href = dataUrl;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }

  function safeSlug(text) {
    return (text || "diagram")
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-+|-+$/g, "") || "diagram";
  }

  function diagramFileName(card, extension) {
    var label = card.getAttribute("data-vfo-diagram-label") || "diagram";
    return "vfo-" + safeSlug(label) + "." + extension;
  }

  function updateFullscreenLabel(card) {
    var openButton = card.querySelector("[data-vfo-diagram-open]");
    if (!openButton) {
      return;
    }

    if (document.fullscreenElement === card) {
      openButton.textContent = "Close";
      card.setAttribute("data-vfo-fullscreen", "1");
      window.setTimeout(function () {
        window.dispatchEvent(new Event("resize"));
      }, 60);
      return;
    }

    openButton.textContent = "Open";
    card.removeAttribute("data-vfo-fullscreen");
    window.setTimeout(function () {
      window.dispatchEvent(new Event("resize"));
    }, 60);
  }

  function createToolbarButton(label, attrName) {
    var button = document.createElement("button");
    button.className = "vfo-diagram-card__button";
    button.type = "button";
    button.textContent = label;
    button.setAttribute(attrName, "1");
    return button;
  }

  function openDiagram(card) {
    if (document.fullscreenElement === card) {
      if (document.exitFullscreen) {
        document.exitFullscreen();
      }
      return;
    }

    if (card.requestFullscreen) {
      card.requestFullscreen();
      return;
    }

    card.classList.toggle("vfo-diagram-card--expanded");
    window.dispatchEvent(new Event("resize"));
  }

  function exportSvgFallback(card, target) {
    var svg = target.querySelector("svg");
    var serializer = null;
    var svgMarkup = "";
    var blob = null;
    var url = "";
    var link = null;

    if (!svg || typeof XMLSerializer === "undefined") {
      return false;
    }

    serializer = new XMLSerializer();
    svgMarkup = serializer.serializeToString(svg);
    blob = new Blob([svgMarkup], { type: "image/svg+xml;charset=utf-8" });
    url = URL.createObjectURL(blob);
    link = document.createElement("a");
    link.href = url;
    link.download = diagramFileName(card, "svg");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
    return true;
  }

  function exportDiagram(card) {
    var target = card.querySelector("." + CONTENT_CLASS);
    var background = getComputedStyle(document.documentElement).getPropertyValue("--vfo-surface").trim() || "#ffffff";

    if (!target) {
      return;
    }

    if (window.htmlToImage && typeof window.htmlToImage.toPng === "function") {
      window.htmlToImage.toPng(target, {
        cacheBust: true,
        backgroundColor: background,
        pixelRatio: Math.min(window.devicePixelRatio || 1, 2)
      }).then(function (dataUrl) {
        downloadDataUrl(diagramFileName(card, "png"), dataUrl);
      }).catch(function () {
        if (!exportSvgFallback(card, target)) {
          console.warn("[vfo-docs] could not export diagram image");
        }
      });
      return;
    }

    exportSvgFallback(card, target);
  }

  function deriveLabel(diagram) {
    var heading = diagram.closest("section");
    var headingText = "";

    if (heading) {
      headingText = (heading.querySelector("h1, h2, h3, h4") || {}).textContent || "";
    }
    if (!headingText) {
      headingText = diagram.getAttribute("data-vfo-reactflow") || "diagram";
    }

    return headingText.trim();
  }

  function wrapDiagram(diagram, type) {
    var wrapper = null;
    var header = null;
    var title = null;
    var actions = null;
    var content = null;
    var hint = null;

    if (diagram.closest("." + CARD_CLASS)) {
      return;
    }

    wrapper = document.createElement("div");
    wrapper.className = CARD_CLASS;
    wrapper.setAttribute("data-vfo-diagram-type", type);
    wrapper.setAttribute("data-vfo-diagram-label", deriveLabel(diagram));

    header = document.createElement("div");
    header.className = "vfo-diagram-card__toolbar";

    title = document.createElement("div");
    title.className = "vfo-diagram-card__title";
    title.textContent = wrapper.getAttribute("data-vfo-diagram-label");

    actions = document.createElement("div");
    actions.className = "vfo-diagram-card__actions";
    actions.appendChild(createToolbarButton("Open", "data-vfo-diagram-open"));
    actions.appendChild(createToolbarButton("Save PNG", "data-vfo-diagram-export"));

    header.appendChild(title);
    header.appendChild(actions);

    content = document.createElement("div");
    content.className = CONTENT_CLASS;
    content.setAttribute("role", "button");
    content.setAttribute("tabindex", "0");
    content.setAttribute("aria-label", "Open diagram in fullscreen");

    hint = document.createElement("div");
    hint.className = "vfo-diagram-card__hint";
    hint.textContent = "Open fullscreen or save as PNG";

    diagram.parentNode.insertBefore(wrapper, diagram);
    wrapper.appendChild(header);
    wrapper.appendChild(content);
    content.appendChild(diagram);
    wrapper.appendChild(hint);

    content.addEventListener("click", function (event) {
      if (event.target.closest("button, a")) {
        return;
      }
      openDiagram(wrapper);
    });

    content.addEventListener("keydown", function (event) {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        openDiagram(wrapper);
      }
    });

    header.querySelector("[data-vfo-diagram-open]").addEventListener("click", function () {
      openDiagram(wrapper);
    });

    header.querySelector("[data-vfo-diagram-export]").addEventListener("click", function () {
      exportDiagram(wrapper);
    });

    updateFullscreenLabel(wrapper);
  }

  function decorateDiagrams() {
    var mermaids = document.querySelectorAll("div.mermaid");
    var reactflows = document.querySelectorAll("[data-vfo-reactflow]");
    var i = 0;

    for (i = 0; i < mermaids.length; i += 1) {
      wrapDiagram(mermaids[i], "mermaid");
    }
    for (i = 0; i < reactflows.length; i += 1) {
      wrapDiagram(reactflows[i], "reactflow");
    }
  }

  document.addEventListener("fullscreenchange", function () {
    var cards = document.querySelectorAll("." + CARD_CLASS);
    var i = 0;
    for (i = 0; i < cards.length; i += 1) {
      updateFullscreenLabel(cards[i]);
    }
  });

  if (window.document$ && typeof window.document$.subscribe === "function") {
    window.document$.subscribe(function () {
      window.requestAnimationFrame(function () {
        decorateDiagrams();
      });
    });
  } else {
    document.addEventListener("DOMContentLoaded", function () {
      decorateDiagrams();
    });
  }
})();
