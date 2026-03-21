# Platform

This directory contains shared platform components used by multiple services.

## Put here

- Shared libraries and internal SDKs
- Cross-cutting runtime capabilities (auth, observability, messaging)
- Reusable tooling and common integration modules

## Tests

- Co-locate tests with each platform component (for example `platform/<component>/test/`)

## Keep out

- One-off service implementations
- Environment-specific infrastructure definitions
