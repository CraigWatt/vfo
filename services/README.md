# Services

This directory contains product and domain services.

## Put here

- Individual service codebases (one folder per service)
- Service-owned APIs, workers, and background jobs
- Service-specific tests and service-local configuration

## Suggested layout

- `services/<service-name>/` for each service
- Current service: `services/vfo/`
- Place service tests under `services/<service-name>/test/`

## Keep out

- Organization-wide shared platform code
- Global infrastructure definitions
