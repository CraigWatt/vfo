# Self-Hosted Runner For Full Media E2E

This guide sets up a repository-level GitHub Actions runner on macOS for full-media E2E tests.

Related workflow:

- `.github/workflows/on-self-hosted-e2e.yml`

Required runner labels:

- `self-hosted`
- `macOS`
- `vfo-media`

## 1) Prepare media mount

Ensure your open-source media pack is available on the runner machine at a stable path, for example:

```bash
/Volumes/vfo-e2e-media/vfo-runner-mezzanines
```

You can use another path and pass it as `assets_dir` when triggering the workflow.

## 2) Add runner in GitHub

In GitHub:

1. Repository `Settings` -> `Actions` -> `Runners`
2. Click `New self-hosted runner`
3. Choose macOS and copy the setup commands

On the runner machine, run the provided commands from GitHub (download, configure, run).

During `config.sh`:

- Use a stable runner name (for example `vfo-media-mac-01`)
- Add labels: `vfo-media,macOS`

Example shape (token is generated in GitHub UI):

```bash
./config.sh \
  --url https://github.com/CraigWatt/vfo \
  --token <RUNNER_TOKEN> \
  --name vfo-media-mac-01 \
  --labels vfo-media,macOS
```

## 3) Install runner dependencies (once)

```bash
brew install ffmpeg cmocka
```

Also ensure Xcode Command Line Tools are installed (`gcc`, `make` available).

## 4) Run as service

From the runner install directory:

```bash
./svc.sh install
./svc.sh start
```

## 5) Trigger full-media E2E

The workflow runs automatically on `main` pushes using defaults.

You can also run it manually from GitHub Actions:

In GitHub Actions:

1. Open workflow `🧪 Full E2E (self-hosted media)`
2. Click `Run workflow`
3. Set `assets_dir` to your mounted media path
4. Optionally set `max_seeds` (for example `4`)
5. Run

The workflow executes `make ci` with:

- `VFO_E2E_ASSET_MODE=local`
- `VFO_E2E_ASSETS_DIR=<assets_dir>`
- `VFO_E2E_MAX_SEEDS=<max_seeds>`

and uploads `tests/e2e/.tmp/` as artifact for debugging.
