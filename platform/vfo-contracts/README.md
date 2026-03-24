# vfo-contracts

Shared machine-readable contracts for integrations (desktop, automation, CI tools).

Scope:

- CLI output schemas
- progress/event payload schemas
- error envelope schemas for adapters/wrappers

Design rules:

- keep schema versions explicit and stable
- add new optional fields before changing required fields
- avoid UI-specific vocabulary in shared schemas
