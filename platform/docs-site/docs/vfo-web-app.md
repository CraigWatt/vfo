# VFO Web App Demo

This page is the browser-friendly companion to `vfo-desktop`.

- it mirrors the desktop wireframe for convenient sharing
- it can be viewed directly on GitHub Pages without any runtime dependencies
- it can still be enhanced by the latest e2e artifact when the artifact-backed script is available

Use it as a lightweight prospect/demo view and as a convenient check that the current pipeline shape still reads well in a browser.

## Demo Dashboard

<div class="vfo-web-app vfo-web-app--static">
  <div class="vfo-web-app__shell">
    <header class="vfo-web-app__topbar">
      <div>
        <h2>Pipeline: UHD SDR Ladder</h2>
        <p>Run: 2026-04-02 18:42</p>
      </div>
      <div class="vfo-web-app__source">
        <span>Demo payload</span>
        <strong>Desktop + Pages view</strong>
      </div>
    </header>

    <section class="vfo-web-app__workspace">
      <aside class="vfo-web-app__panel vfo-web-app__assets">
        <div class="vfo-web-app__panel-head">
          <h3>Assets</h3>
          <span>Mezzanine folder</span>
        </div>

        <label class="vfo-web-app__search" aria-label="Search assets">
          <span>Search assets...</span>
          <input type="text" value="movie_01" />
        </label>

        <div class="vfo-web-app__asset-list">
          <button type="button" class="vfo-web-app__asset is-active">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-failed"></span>
            <span class="vfo-web-app__asset-name">movie_01.mxf</span>
            <span class="vfo-web-app__asset-icon">✖</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-complete"></span>
            <span class="vfo-web-app__asset-name">movie_02.mxf</span>
            <span class="vfo-web-app__asset-icon">✔</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-running"></span>
            <span class="vfo-web-app__asset-name">movie_03.mxf</span>
            <span class="vfo-web-app__asset-icon">⏳</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-waiting"></span>
            <span class="vfo-web-app__asset-name">movie_04.mxf</span>
            <span class="vfo-web-app__asset-icon">○</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-complete"></span>
            <span class="vfo-web-app__asset-name">movie_05.mxf</span>
            <span class="vfo-web-app__asset-icon">✔</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-failed"></span>
            <span class="vfo-web-app__asset-name">movie_06.mxf</span>
            <span class="vfo-web-app__asset-icon">✖</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-complete"></span>
            <span class="vfo-web-app__asset-name">movie_07.mxf</span>
            <span class="vfo-web-app__asset-icon">✔</span>
          </button>
          <button type="button" class="vfo-web-app__asset">
            <span class="vfo-web-app__asset-dot vfo-web-app__status-waiting"></span>
            <span class="vfo-web-app__asset-name">movie_08.mxf</span>
            <span class="vfo-web-app__asset-icon">○</span>
          </button>
        </div>

        <div class="vfo-web-app__filter-block">
          <h4>Filters</h4>
          <div class="vfo-web-app__filter-list">
            <label class="vfo-web-app__filter"><input type="checkbox" checked /> <span>All</span></label>
            <label class="vfo-web-app__filter"><input type="checkbox" checked /> <span>Failed</span></label>
            <label class="vfo-web-app__filter"><input type="checkbox" checked /> <span>Running</span></label>
            <label class="vfo-web-app__filter"><input type="checkbox" checked /> <span>Waiting</span></label>
            <label class="vfo-web-app__filter"><input type="checkbox" checked /> <span>Complete</span></label>
          </div>
        </div>
      </aside>

      <section class="vfo-web-app__panel vfo-web-app__workflow">
        <div class="vfo-web-app__panel-head">
          <h3>Workflow</h3>
          <span>All nodes visible by default</span>
        </div>

        <div class="vfo-web-app__workflow-shell">
          <svg class="vfo-web-app__workflow-edges" viewBox="0 0 1200 460" aria-hidden="true">
            <path d="M 258 204 C 297 204, 297 204, 280 204"></path>
            <path d="M 510 204 C 535 204, 535 204, 510 204"></path>
            <path d="M 740 204 C 810 204, 810 294, 994 294"></path>
            <path d="M 510 204 C 570 204, 570 360, 512 360"></path>
            <path d="M 950 204 C 920 204, 920 360, 770 360"></path>
            <path d="M 972 294 C 936 294, 936 360, 770 360"></path>
          </svg>

          <div class="vfo-web-app__workflow-nodes">
            <button type="button" class="vfo-web-app__node vfo-web-app__status-complete is-active" style="left:48px; top:146px;">
              <span class="vfo-web-app__node-head"><strong>Input</strong><span>✔</span></span>
              <span class="vfo-web-app__node-body">Singular mezzanine asset</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-complete" style="left:280px; top:146px;">
              <span class="vfo-web-app__node-head"><strong>Probe</strong><span>✔</span></span>
              <span class="vfo-web-app__node-body">ffprobe summary and QC hints</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-running" style="left:510px; top:146px;">
              <span class="vfo-web-app__node-head"><strong>Deint</strong><span>⏳</span></span>
              <span class="vfo-web-app__node-body">Optional cleanup / normalization</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-failed" style="left:742px; top:146px;">
              <span class="vfo-web-app__node-head"><strong>Encode</strong><span>✖</span></span>
              <span class="vfo-web-app__node-body">Primary profile action</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-waiting" style="left:994px; top:236px;">
              <span class="vfo-web-app__node-head"><strong>HLS</strong><span>○</span></span>
              <span class="vfo-web-app__node-body">Delivery packaging</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-waiting" style="left:770px; top:302px;">
              <span class="vfo-web-app__node-head"><strong>QC</strong><span>○</span></span>
              <span class="vfo-web-app__node-body">PSNR / SSIM / VMAF</span>
            </button>
            <button type="button" class="vfo-web-app__node vfo-web-app__status-complete" style="left:512px; top:302px;">
              <span class="vfo-web-app__node-head"><strong>Metadata</strong><span>✔</span></span>
              <span class="vfo-web-app__node-body">Sidecar manifests and notes</span>
            </button>
          </div>
        </div>

        <div class="vfo-web-app__caption">
          Node status for the selected asset is shown inline, with failures, running, waiting, and
          complete states carried directly on the lane.
        </div>
      </section>

      <aside class="vfo-web-app__panel vfo-web-app__inspector">
        <div class="vfo-web-app__panel-head">
          <h3>Inspector</h3>
          <span>Current selection</span>
        </div>

        <div class="vfo-web-app__density-map">
          <div class="vfo-web-app__density-item">
            <span class="vfo-web-app__density-icon">✔</span>
            <strong>91</strong>
            <span>Complete</span>
          </div>
          <div class="vfo-web-app__density-item">
            <span class="vfo-web-app__density-icon">✖</span>
            <strong>7</strong>
            <span>Failed</span>
          </div>
          <div class="vfo-web-app__density-item">
            <span class="vfo-web-app__density-icon">⏳</span>
            <strong>12</strong>
            <span>Running</span>
          </div>
          <div class="vfo-web-app__density-item">
            <span class="vfo-web-app__density-icon">○</span>
            <strong>18</strong>
            <span>Waiting</span>
          </div>
        </div>

        <div class="vfo-web-app__inspector-card">
          <div class="vfo-web-app__row"><span>Asset</span><strong>movie_01.mxf</strong></div>
          <div class="vfo-web-app__row"><span>Node</span><strong>Encode</strong></div>
          <div class="vfo-web-app__row"><span>Status</span><strong>Failed</strong></div>
          <div class="vfo-web-app__row"><span>Exit code</span><strong>1</strong></div>
        </div>

        <div class="vfo-web-app__code-panel">
          <div class="vfo-web-app__code-head">
            <span>stdout / stderr / cmd</span>
            <div class="vfo-web-app__actions">
              <button type="button">Copy</button>
              <button type="button">Expand</button>
            </div>
          </div>
          <pre class="vfo-web-app__code">ffmpeg -i movie_01.mxf -c:v libx265 ...

error=encoder initialization failed
stderr=codec context rejected
hint=inspect profile guardrails
result=failed</pre>
        </div>
      </aside>
    </section>

    <footer class="vfo-web-app__legend">
      <div class="vfo-web-app__legend-summary">
        <span>⏺ Total: 128</span>
        <span>✔ Complete: 91</span>
        <span>✖ Failed: 7</span>
        <span>⏳ Running: 12</span>
        <span>○ Waiting: 18</span>
      </div>

      <div class="vfo-web-app__stage-badges">
        <span class="vfo-web-app__stage-badge">Input 128</span>
        <span class="vfo-web-app__stage-badge">Probe 128</span>
        <span class="vfo-web-app__stage-badge">Deint 97</span>
        <span class="vfo-web-app__stage-badge">Encode 91</span>
        <span class="vfo-web-app__stage-badge">HLS 88</span>
        <span class="vfo-web-app__stage-badge">QC 73</span>
        <span class="vfo-web-app__stage-badge">Metadata 128</span>
      </div>
    </footer>
  </div>
</div>

## How It Works

- the left rail shows singular assets from the mezzanine folder
- the center lane shows the left-to-right workflow
- the right rail shows the selected node output in a code-style inspector
- the lower band shows summary totals and stage counts

If the latest e2e artifact is available, the enhancement script can hydrate the same shell with source metadata from `tests/e2e/.reports/latest/`.
If not, the static demo above still gives you the full page.
