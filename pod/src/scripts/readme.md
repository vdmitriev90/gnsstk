# `download_brdm.py`

This document describes the GNSS download script used in POD workflows.

## Location

- Script: `pod/src/scripts/download_brdm.py`

## Purpose

The script downloads and maintains GNSS data from CDDIS:

- Broadcast navigation (`BRDM`, with `BRDC` fallback)
- Precise orbits (`SP3`)
- Precise clocks (`CLK`)
- Observation files (`OBS`) for requested stations

## Behavior

For each day in the requested range (`--from`..`--to`):

1. Build a 3-day product window: `current-1`, `current`, `current+1`
2. Remove outdated files in `nav/`, `sp3/`, `clk/`
3. Download product files for that window
4. Clear `obs/` and download observation files only for `current`

## CLI arguments

- `--from` (required): start date (`YYYY-MM-DD`)
- `--to` (required): end date (`YYYY-MM-DD`)
- `--sites` (required): comma-separated station IDs, e.g. `ALGO,BAKO`
- `--out` (optional, last): output directory, default `./nav_files`

## Dependencies

- Python 3
- `requests`
- `unlzw3`
- `crx2rnx.exe` in `PATH` (for Hatanaka conversion: `.crx`, `.{yy}d`)

Install Python packages:

```bash
pip install requests unlzw3
```

## Example

```bash
python pod/src/scripts/download_brdm.py \
  --from 2024-03-01 \
  --to 2024-03-05 \
  --sites ALGO,BAKO \
  --out ./nav_files
```
