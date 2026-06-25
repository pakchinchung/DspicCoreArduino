#!/usr/bin/env bash
###############################################################################
# run-cpp-sim-test-win.sh — 100% WINDOWS-NATIVE C++-on-dsPIC test + verify.
#
# NO WSL. Proves the end-user toolchain path: our native Windows C++ compiler
# (cc1plus.exe) compiles a sketch that includes the real <xc.h>, the stock
# XC-DSC binutils assemble/link/hex it, and Microchip's MPLAB X Simulator (mdb)
# runs it and checks the result globals.
#
#   our g++  : C:\dspic-cpp-toolchain-win\bin\elf-g++.exe  (GPL C++ rebuild)
#   binutils : C:\Program Files\Microchip\xc-dsc\v3.31.01\bin\xc-dsc-*
#   device   : DFP from MPLAB X packs
#   sim      : MPLAB X mdb.bat
#
# Usage (Git-bash on Windows):
#   tools/test/run-cpp-sim-test-win.sh
###############################################################################
set -uo pipefail

# --- locations (override via env if your install differs) --------------------
TC="${TC:-/c/dspic-cpp-toolchain-win}"
XCDSC="${XCDSC:-/c/Program Files/Microchip/xc-dsc/v3.31.01}"
DFP="${DFP:-/c/Program Files/Microchip/MPLABX/v6.30/packs/Microchip/dsPIC33CK-MP_DFP/1.15.423/xc16}"
MPLABX_BIN="${MPLABX_BIN:-C:/PROGRA~1/MICROC~1/MPLABX/v6.30/MPLAB_~1/bin}"
DEVICE="${DEVICE:-33CK256MP508}"
GLDSUB="${GLDSUB:-dsPIC33C}"          # support/<GLDSUB>/gld/p<DEVICE>.gld

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="${1:-$REPO/spike/phase2/blink_button_test.cpp}"
SHIM="$REPO/cores/dspic/dspic_cpp_compat.h"
WORK="$REPO/spike/phase2"
BASE="$(basename "${SRC%.*}")_win"

GPP="$TC/bin/elf-g++.exe"
LX="$TC/libexec/gcc/pic30-elf/8.3.1"
INC="$XCDSC/include"
XCGCC="$XCDSC/bin/xc-dsc-gcc.exe"
B2H="$XCDSC/bin/xc-dsc-bin2hex.exe"
GLD="$DFP/support/$GLDSUB/gld/p${DEVICE}.gld"

log(){ printf '\n>> %s\n' "$*"; }
die(){ printf 'ERROR: %s\n' "$*" >&2; exit 1; }

[ -x "$GPP" ]   || die "native Windows C++ compiler not found: $GPP"
[ -x "$XCGCC" ] || die "XC-DSC driver not found: $XCGCC (install XC-DSC v3.31.01)"
[ -f "$GLD" ]   || die "linker script not found: $GLD"
[ -f "$SHIM" ]  || die "C++ compat shim not found: $SHIM"

cd "$WORK"

# --- 1. compile C++ -> .s with our native Windows compiler (debug on) --------
# -S (stop at assembly): our gcc-only toolchain has no assembler; xc-dsc-gcc
# assembles+links below. -include the shim so <xc.h>'s __prog__ parses in C++.
log "compile (native Windows cc1plus): $(basename "$SRC")"
"$GPP" -B"$LX" -mcpu="$DEVICE" -mdfp="$DFP" -I"$INC" -include "$SHIM" \
   -g -O1 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
   -S "$SRC" -o "$BASE.s" 2>&1 | grep -v short-double
[ -f "$BASE.s" ] || die "C++ compile failed"

# --- 2. assemble + link with stock XC-DSC -> .elf ----------------------------
log "link (xc-dsc-gcc + device gld)"
"$XCGCC" -mcpu="$DEVICE" -mdfp="$DFP" -T "$GLD" -g "$BASE.s" -o "$BASE.elf" 2>&1 \
  | grep -vi "Loading file"
[ -f "$BASE.elf" ] || die "link failed"

# --- 3. bin2hex (proves the deployable artifact builds) ----------------------
"$B2H" -mdfp="$DFP" "$BASE.elf" >/dev/null 2>&1
[ -f "$BASE.hex" ] && log "hex built: $BASE.hex ($(wc -l < "$BASE.hex") lines)"

# --- 4. run on MPLAB X simulator (mdb) ---------------------------------------
# Result globals + expected values. r_sig is the "reached done()" sentinel.
VARS=(r_sig r_led_on r_led_off r_led_follows)
EXPECT=(0xABCD 0x0001 0x0000 0x0001)
LABELS=("reached done()" "LED drive HIGH" "LED drive LOW" "button-follows-LED")

{
  echo "device dsPIC$DEVICE"
  echo "set system.disableerrormsg true"
  echo "hwtool Sim"
  echo "program $BASE.elf"
  echo "break $(basename "$SRC"):101"
  echo "run"
  for i in $(seq 1 12); do echo "print /x /datasize:2 r_sig"; done   # settle async run
  for v in "${VARS[@]}"; do echo "print /x /datasize:2 $v"; done
  echo "quit"
} > "$BASE.mdb"

log "simulate (MPLAB X mdb, headless)"
OUT="$("$MPLABX_BIN/mdb.bat" "$BASE.mdb" 2>&1)"

# values print on the line AFTER "var=" ; take the last reading of each
PASS=1
norm(){ echo "$1" | tr 'A-F' 'a-f'; }
for i in "${!VARS[@]}"; do
  v="${VARS[$i]}"
  got=$(printf '%s\n' "$OUT" | grep -A1 "^$v=$" | grep -oE "0x[0-9A-Fa-f]+" | tail -1)
  if [ "$(norm "$got")" = "$(norm "${EXPECT[$i]}")" ]; then mark="OK"; else mark="MISMATCH"; PASS=0; fi
  printf '   %-14s %-20s expect %-8s got %-10s %s\n' "$v" "${LABELS[$i]}" "${EXPECT[$i]}" "${got:-<none>}" "$mark"
done

echo ""
if [ "$PASS" = 1 ]; then
  echo "=== PASS — native Windows C++ blink+button verified on $DEVICE (no WSL) ==="
else
  echo "=== FAIL/INCOMPLETE on $DEVICE ==="; exit 1
fi
