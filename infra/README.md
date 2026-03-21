# Infrastructure

This directory contains infrastructure and operational code.

## Put here

- Infrastructure as code (for example Terraform, Pulumi, CloudFormation)
- Environment definitions and deployment config
- CI/CD pipeline plumbing and release automation
- Networking, secrets wiring, and runtime infrastructure concerns
- Packaging assets (for example `infra/packaging/macos/`)

## Tests

- Place infra-focused checks under `infra/test/`

## Keep out

- Product business logic
- Service-specific application code
