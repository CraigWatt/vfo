# Observability Contract

vfo should emit one canonical run model that can feed the browser dashboard now and metrics, logs, and traces later without changing the underlying meaning of the data.

## Objective

- keep the web app, CI reports, and future observability backends aligned
- avoid one-off schemas that only serve a single UI
- preserve low-cardinality metrics while still carrying rich per-run detail

## Core Model

The app should emit structured events for a small set of stable entities:

- `run_id`
- `pipeline_id`
- `asset_id`
- `node_id`
- `stage_id`
- `attempt_id`

Each event should carry:

- timestamp
- state transition
- duration when available
- exit code when available
- command line or action name
- input and output asset references
- error class and human-readable detail when failed

## Canonical States

Use a small stable state set across all sinks:

- `queued`
- `running`
- `waiting`
- `complete`
- `failed`
- `skipped`

## Sink Mapping

The same event stream should be adapted into multiple outputs:

- browser dashboard: denormalized snapshot for UI rendering
- Prometheus: numeric counters, gauges, and histograms with low-cardinality labels
- Grafana Mimir: long-term storage for Prometheus-style metrics
- Grafana Loki: structured logs with stable metadata
- Grafana Tempo: traces and span timing for pipeline execution

## Guiding Rules

- never rely on UI labels as the canonical schema
- keep labels stable and machine-friendly
- avoid high-cardinality Prometheus labels such as raw paths or arbitrary filenames
- preserve full detail in logs or snapshot payloads instead
- version the dashboard payload so readers can evolve safely

## Suggested Shape

At a minimum, the dashboard snapshot should include:

- run header metadata
- one or more pipelines
- asset lists per pipeline
- workflow nodes and edges
- node details for the selected asset/pipeline
- summary counts and stage totals

That gives the web app enough information to render the live dashboard while leaving the event stream available for future observability exports.

## Next Step

The next implementation step is to derive this snapshot from runtime events, then fan the same events into metrics, logs, and traces.
