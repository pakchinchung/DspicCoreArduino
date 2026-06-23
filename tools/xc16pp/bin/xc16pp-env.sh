#!/usr/bin/env bash
# xc16pp-env.sh — shared discovery logic, sourced by all xc16pp-* wrappers.
#
# Locates two things the build needs:
#   1. XC-DSC / XC16 compiler install (GPL binaries + generic runtime)
#   2. Device Family Pack (DFP) — device headers (.h), linker scripts (.gld),
#      and processor support libs. In the XC-DSC era these live under MPLAB X
#      packs, NOT inside the compiler.
#
# Exports:
#   XCDSC        — compiler install root  (e.g. .../xc-dsc/v3.31.01)
#   XCDSC_BIN    — compiler bin dir
#   XC_PREFIX    — tool name prefix: "xc-dsc" (v3.x) or "xc16" (v2.x)
#   DFP_SUPPORT  — DFP support dir for the target family (h/, gld/, lib/)
#   OUR_BIN      — dir holding our GPL-rebuilt cc1plus / xc-dsc-g++

OUR_BIN="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Our GPL-rebuilt C++ compiler, bundled alongside the wrappers in the tool
# package: tools/xc16pp/dspic-cpp-toolchain-win/. (Windows is the built host;
# mac/linux builds are a future add — the .sh path fails gracefully if absent.)
OUR_TC="$OUR_BIN/../dspic-cpp-toolchain-win"
_find_our_gpp() {
    local g
    for g in "$OUR_TC/bin/elf-g++.exe" "$OUR_TC/bin/elf-g++"; do
        [ -x "$g" ] && { echo "$g"; return; }
    done
    return 1
}
# Directory holding our cc1plus (passed to the driver via -B).
_our_libexec() { echo "$OUR_TC/libexec/gcc/pic30-elf/8.3.1"; }

_pick_latest_ver() {
    # echo highest vX.Y[.Z] subdirectory of $1
    ls -1 "$1" 2>/dev/null | grep -E '^v[0-9]' | sort -V | tail -1
}

find_xcdsc() {
    if [ -n "$XCDSC_PATH" ] && [ -d "$XCDSC_PATH" ]; then echo "$XCDSC_PATH"; return; fi
    local base ver
    for base in \
        "/c/Program Files/Microchip/xc-dsc" \
        "/c/Program Files/Microchip/xc16" \
        "/Applications/microchip/xc-dsc" \
        "/Applications/microchip/xc16" \
        "/opt/microchip/xc-dsc" \
        "/opt/microchip/xc16"; do
        if [ -d "$base" ]; then
            ver=$(_pick_latest_ver "$base")
            [ -n "$ver" ] && { echo "$base/$ver"; return; }
            echo "$base"; return
        fi
    done
    return 1
}

# Detect tool prefix from what binaries actually exist in the install.
detect_prefix() {
    local bin="$1"
    if   [ -x "$bin/xc-dsc-gcc.exe" ] || [ -x "$bin/xc-dsc-gcc" ]; then echo "xc-dsc"
    elif [ -x "$bin/xc16-gcc.exe" ]   || [ -x "$bin/xc16-gcc" ];   then echo "xc16"
    elif [ -x "$bin/pic30-gcc.exe" ]  || [ -x "$bin/pic30-gcc" ];  then echo "pic30"
    else echo "xc-dsc"; fi   # default to newest naming
}

# Given a device, echo the DFP ROOT dir to pass to -mdfp (the .../<ver>/xc16 dir).
# Verified in the Phase 1 spike: the compiler needs -mdfp=<DFP>/xc16.
find_dfp_root() {
    local support
    support=$(find_dfp_support "$1") || return 1
    # support = .../<ver>/xc16/support/<arch>  ->  -mdfp wants .../<ver>/xc16
    echo "$(dirname "$(dirname "$support")")"
}

# Given a device, echo the absolute path to its .gld linker script.
# Verified in the spike: the link step must pass -T <device>.gld explicitly.
find_device_gld() {
    local support gld
    support=$(find_dfp_support "$1") || return 1
    gld="$support/gld/p$1.gld"
    [ -f "$gld" ] && { echo "$gld"; return; }
    # fallback: search
    find "$support/gld" -iname "p$1.gld" 2>/dev/null | head -1
}

# Find the DFP support dir for a device family.
# $1 = device, e.g. 33CK256MP508 -> family token dsPIC33CK-MP, support/dsPIC33C
find_dfp_support() {
    local dev="$1"
    [ -n "$DFP_PATH" ] && [ -d "$DFP_PATH" ] && { echo "$DFP_PATH"; return; }

    # Map device to DFP name + support subdir (extend as families are added).
    # Supported families: dsPIC33CK (16-bit) and dsPIC33AK (32-bit). Match the
    # device to its DFP and the support/<arch> subdir the headers/gld live under.
    local dfp_glob support_sub
    case "$dev" in
        33CK*MP*)  dfp_glob="dsPIC33CK-MP_DFP"; support_sub="dsPIC33C" ;;
        33CK*MC*)  dfp_glob="dsPIC33CK-MC_DFP"; support_sub="dsPIC33C" ;;
        33AK*MC*)  dfp_glob="dsPIC33AK-MC_DFP"; support_sub="dsPIC33A" ;;
        33AK*MP*)  dfp_glob="dsPIC33AK-MP_DFP"; support_sub="dsPIC33A" ;;
        33AK*)     dfp_glob="dsPIC33AK-*_DFP";  support_sub="dsPIC33A" ;;
        33CK*)     dfp_glob="dsPIC33CK-*_DFP";  support_sub="dsPIC33C" ;;
        *)         dfp_glob="dsPIC33CK-MP_DFP"; support_sub="dsPIC33C" ;;
    esac

    local packroot base dfpdir ver
    for packroot in \
        "$HOME/.mchp_packs/Microchip" \
        "/c/Program Files/Microchip/MPLABX"/*/packs/Microchip \
        "/Applications/microchip/mplabx"/*/packs/Microchip \
        "/opt/microchip/mplabx"/*/packs/Microchip; do
        [ -d "$packroot" ] || continue
        for dfpdir in "$packroot"/$dfp_glob; do
            [ -d "$dfpdir" ] || continue
            ver=$(_pick_ver_plain "$dfpdir")
            [ -n "$ver" ] || continue
            # XC-DSC packs keep an xc16/support tree for source compatibility
            local s="$dfpdir/$ver/xc16/support/$support_sub"
            [ -d "$s" ] && { echo "$s"; return; }
            s="$dfpdir/$ver/xc16/support/$(ls -1 "$dfpdir/$ver/xc16/support" 2>/dev/null | head -1)"
            [ -d "$s" ] && { echo "$s"; return; }
        done
    done
    return 1
}

_pick_ver_plain() {
    # DFP versions are plain numbers like 1.15.423 (no leading v)
    ls -1 "$1" 2>/dev/null | grep -E '^[0-9]' | sort -V | tail -1
}

die_no_xcdsc() {
    cat >&2 <<'EOF'

ERROR: XC-DSC / XC16 compiler not found.
dspicArduino requires a free Microchip XC-DSC install for device headers and runtime.

  Download: https://www.microchip.com/en-us/tools-resources/develop/mplab-xc-compilers
  Then set XCDSC_PATH=/path/to/xc-dsc/vX.YY if it is in a non-standard location.

EOF
    exit 1
}

die_no_dfp() {
    cat >&2 <<EOF

ERROR: Device Family Pack for '$1' not found.
Install it via MPLAB X (Tools > Packs) or set DFP_PATH to the support dir, e.g.
  .../packs/Microchip/dsPIC33CK-MP_DFP/<ver>/xc16/support/dsPIC33C

EOF
    exit 1
}
