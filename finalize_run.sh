#!/bin/bash
# =============================================================================
# finalize_run.sh - archive Results/ and Plots/ under an auto-generated tag.
#
# After a completed scan, this renames:
#     Results/  ->  Results_<TAG>/
#     Plots/    ->  Plots_<TAG>/
# where <TAG> is built from the custom settings in configs/flags_MP_EIC.cfg,
# so you can run the next scan (e.g. pion) without renaming directories by hand.
#
# Example tag:  p120GeV_e10MeV_mp_coin
#   p120GeV   = signal particle + energy   (particleType / particleEnergy)
#   e10MeV    = background particle + energy (bkgparticleType / bkgparticleEnergy)
#   mp        = multi-plane geometry (MULTIMALTA)
#   coin      = coincidence scan
#
# Usage (from malta_simulation/):
#   ./finalize_run.sh                    # auto tag from cfg
#   ./finalize_run.sh --suffix w36ns     # append an extra label
#   ./finalize_run.sh --dry-run          # only print the tag, move nothing
# =============================================================================
set -euo pipefail
cd "$(dirname "$0")"

CFG="configs/flags_MP_EIC.cfg"
DRY=false
SUFFIX=""

while [ $# -gt 0 ]; do
    case "$1" in
        --suffix) SUFFIX="$2"; shift 2 ;;
        --dry-run|-n) DRY=true; shift ;;
        -h|--help) sed -n '2,24p' "$0"; exit 0 ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

[ -f "$CFG" ] || { echo "ERROR: missing $CFG"; exit 1; }

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
# Read `key = value` from the cfg (strips comments and surrounding whitespace).
get_cfg() {
    awk -v key="$1" '
        {
            line = $0
            sub(/#.*/, "", line)                 # strip trailing comments
            n = split(line, a, "=")
            gsub(/^[ \t]+|[ \t]+$/, "", a[1])
            if (a[1] == key) {
                gsub(/^[ \t]+|[ \t]+$/, "", a[2])
                print a[2]
                found = 1
                exit
            }
        }
        END { if (!found) exit 1 }
    ' "$CFG"
}

# Compact particle name used in the tag.
short_particle() {
    case "$1" in
        proton|protons)             echo "p"  ;;
        pion*|pi-|pi+|\"pi\")       echo "pi" ;;
        e-|electron|electrons)      echo "e"  ;;
        mu-|muon|muons)             echo "mu" ;;
        *)                          echo "$1" ;;
    esac
}

# Trim a float for a tidy label (0.01 -> "0.01", 120 -> "120", 10.0 -> "10").
fmt_num() { awk -v x="$1" 'BEGIN { printf "%.6g", x }'; }

# Dump every `key = value` line from the cfg (comments/blank lines removed).
dump_settings() {
    awk '
        {
            line = $0
            sub(/#.*/, "", line)
            gsub(/^[ \t]+|[ \t]+$/, "", line)
            if (line ~ /=/) print line
        }
    ' "$CFG"
}

# ---------------------------------------------------------------------------
# Extract the settings that define this scan
# ---------------------------------------------------------------------------
particleType="$(get_cfg particleType)"
particleEnergy="$(get_cfg particleEnergy)"
energyDistribution="$(get_cfg energyDistribution)"
bkgparticleType="$(get_cfg bkgparticleType)"
bkgparticleEnergy="$(get_cfg bkgparticleEnergy)"
bkgenergyDistribution="$(get_cfg bkgenergyDistribution)"
beamGeometry="$(get_cfg beamGeometry)"
geoFlag="$(get_cfg preDefinedGeometryFlag)"

# Optional extra context from the pipeline (env or run_efficiency.sh).
WINDOW_NS="${WINDOW_NS:-}"
if [ -z "$WINDOW_NS" ] && [ -f run_efficiency.sh ]; then
    WINDOW_NS="$(sed -n 's/^WINDOW_NS=\([0-9.]*\).*/\1/p' run_efficiency.sh | head -1)"
fi
SAVE="${SAVE:-analysis_results_MP}"
THRESHOLD="${THRESHOLD:-100}"

# ---------------------------------------------------------------------------
# Build the tag
# ---------------------------------------------------------------------------
# Energy label: use the fixed value only when the distribution mode is 'none';
# otherwise the energy is sampled from a distribution (e.g. EIC), so name it after that.
if [ "$energyDistribution" = "none" ] || [ -z "$energyDistribution" ]; then
    SIG="$(short_particle "$particleType")$(fmt_num "$particleEnergy")GeV"
else
    SIG="$(short_particle "$particleType")${energyDistribution}"
fi

if [ "$bkgenergyDistribution" = "none" ] || [ -z "$bkgenergyDistribution" ]; then
    BKG="$(short_particle "$bkgparticleType")$(fmt_num "$(awk -v e="$bkgparticleEnergy" 'BEGIN { print e*1000 }')")MeV"
else
    BKG="$(short_particle "$bkgparticleType")${bkgenergyDistribution}"
fi

# Geometry: MULTIMALTA -> mp (matches existing Results_* naming).
GEOSUF="mp"
[ "$geoFlag" = "MULTIMALTA" ] || GEOSUF="$(echo "$geoFlag" | tr '[:upper:]' '[:lower:]')"

TAG="${SIG}_${BKG}_${GEOSUF}_coin"
[ -n "$SUFFIX" ] && TAG="${TAG}_${SUFFIX}"

echo "============================================================================="
echo " Scan tag:  $TAG"
echo "   signal  : $particleType @ $(fmt_num "$particleEnergy") GeV   (dist: ${energyDistribution:-none}, beam: $beamGeometry)"
echo "   bkg     : $bkgparticleType @ $(fmt_num "$bkgparticleEnergy") GeV   (dist: ${bkgenergyDistribution:-none})"
echo "   window  : ${WINDOW_NS:-?} ns | SAVE=$SAVE | THRESHOLD=$THRESHOLD"
echo "============================================================================="

# ---------------------------------------------------------------------------
# Archive Results/ and Plots/
# ---------------------------------------------------------------------------
write_meta() {  # $1 = dir path
    local dir="$1"
    {
        echo "# Scan finalized on $(date '+%Y-%m-%d %H:%M:%S')"
        echo "TAG=$TAG"
        echo "SAVE=$SAVE"
        echo "THRESHOLD=$THRESHOLD"
        echo "WINDOW_NS=${WINDOW_NS:-?}"
        echo "# ---- custom settings from $CFG ----"
        dump_settings
    } > "$dir/scan_settings.txt"
}

move_one() {  # $1 = base name (Results|Plots)
    local base="$1"
    local dest="${base}_${TAG}"

    if [ ! -e "$base" ]; then
        echo "WARNING: $base/ not found, skipping"
        return
    fi
    if [ -e "$dest" ]; then
        echo "ERROR: $dest already exists. Move it away or use --suffix." >&2
        exit 1
    fi

    write_meta "$base"

    if $DRY; then
        echo "[dry-run] would move: $base -> $dest"
    else
        mv "$base" "$dest"
        echo "moved: $base -> $dest"
    fi
}

move_one Results
move_one Plots

if $DRY; then
    echo
    echo "Dry run: nothing was moved."
fi
