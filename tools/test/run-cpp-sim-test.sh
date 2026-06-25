#!/usr/bin/env bash
###############################################################################
# run-cpp-sim-test.sh — self-running C++-on-dsPIC test + verify.
#
# Compiles a multi-object C++ program with the rebuilt C++ compiler (in WSL),
# links it with the Windows XC-DSC binutils for the chosen device, runs it on
# Microchip's MPLAB X Simulator (mdb), reads the results, and prints PASS/FAIL.
#
# Two device families are handled automatically:
#   * dsPIC33C/E/F/30  -> multi_object_test.cpp, run+breakpoint, read array R[]
#   * dsPIC33A (33AK)  -> scalar_33a_test.cpp, reset+stepi, read scalar globals
#     (mdb on 33A won't halt C++ binaries at breakpoints and returns null for C++
#      array reads, so we step to the end and read simple scalars instead.)
#
# Run from a Windows MSYS/Git-Bash shell:
#   tools/test/run-cpp-sim-test.sh [DEVICE]
# DEVICE defaults to 33CK256MP508.
###############################################################################
set -uo pipefail

DEVICE="${1:-33CK256MP508}"
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && cd .. && pwd)"
SPIKE="$REPO/spike/phase1"
WHOME="$(wsl.exe -d Ubuntu -- bash -lc 'echo $HOME' 2>/dev/null | tr -d '\r')"
GPP="$WHOME/xcdsc-build/out/bin/elf-g++"
LX="$WHOME/xcdsc-build/out/libexec/gcc/pic30-elf/8.3.1"
XCBIN="/c/Program Files/Microchip/xc-dsc/v3.31.01/bin"
MDB="$(ls /c/Program\ Files/Microchip/MPLABX/*/mplab_platform/bin/mdb.bat 2>/dev/null | sort -V | tail -1)"

log(){ printf '\n>> %s\n' "$*"; }
die(){ printf 'ERROR: %s\n' "$*" >&2; exit 1; }
[ -n "$MDB" ] || die "mdb (MPLAB X simulator) not found"

# --- pick test program + verification method by device family ---------------
case "$DEVICE" in
  33A*|33a*) FAMILY=33A; CPP=scalar_33a_test.cpp
             VARS=(rv_poly rv_tag rv_base rv_sig); EXPECT=(0x0002 0x0063 0x0001 0xABCD)
             LABELS=("poly dispatch=2" "ctor chain tag=99" "base virtual=1" "sig=0xABCD") ;;
  *)         FAMILY=other; CPP=multi_object_test.cpp
             VARS=("R[0]" "R[1]" "R[2]" "R[3]" "R[4]" "R[5]" "R[6]" "R[7]")
             EXPECT=(0x000A 0x000B 0x0064 0x0066 0x0007 0x007B 0x0003 0xABCD)
             LABELS=("a.start=10" "a.next=11" "b.start=100" "b.next=102(ovr)" "c=7(poly)" "sum=123(poly)" "count=3" "sig=ABCD") ;;
esac
[ -f "$SPIKE/$CPP" ] || die "test source not found: $SPIKE/$CPP"

# --- locate the device's DFP support (header -> support/<arch> -> xc16 + gld) ---
log "Device $DEVICE (family $FAMILY), test $CPP"
HDR="$(find /c/Program\ Files/Microchip/MPLABX/*/packs/Microchip "$HOME/.mchp_packs/Microchip" \
        -ipath '*support*' -iname "p${DEVICE}.h" 2>/dev/null | sort -V | tail -1)"
[ -n "$HDR" ] || die "no installed DFP provides p${DEVICE}.h"
SUPPORT="$(dirname "$(dirname "$HDR")")"; DFP_XC16="$(dirname "$(dirname "$SUPPORT")")"
GLD="$(find "$SUPPORT/gld" -iname "p${DEVICE}.gld" 2>/dev/null | head -1)"
[ -n "$GLD" ] || die "no linker script p${DEVICE}.gld"
DFP_WSL="${DFP_XC16/\/c\//\/mnt\/c\/}"
CPP_WSL="${SPIKE/\/c\//\/mnt\/c\/}/$CPP"
S_OUT="$SPIKE/_sim_${DEVICE}.s"; ELF="$SPIKE/_sim_${DEVICE}.elf"

# --- 1. compile C++ -> dsPIC assembly with the rebuilt compiler (WSL) --------
log "Compiling (rebuilt C++ compiler, WSL)"
wsl.exe -d Ubuntu -- bash -s <<WEOF 2>&1 | grep -v short-double | sed 's/^/   /'
"$GPP" -B"$LX" -mcpu=$DEVICE -mdfp="$DFP_WSL" -S -g -O1 \
  -fno-exceptions -fno-rtti -fno-threadsafe-statics \
  "$CPP_WSL" -o "${S_OUT/\/c\//\/mnt\/c\/}"
WEOF
[ -f "$S_OUT" ] || die "C++ compile failed"

# --- 2. assemble + link with the Windows XC-DSC binutils ---------------------
log "Linking for $DEVICE"
"$XCBIN/xc-dsc-gcc.exe" -mcpu=$DEVICE -mdfp="$DFP_XC16" -omf=elf -g -T"$GLD" \
    "$S_OUT" -o "$ELF" 2>&1 | grep -vi "loading file" | sed 's/^/   /'
[ -f "$ELF" ] || die "link failed"

# --- 3. build the mdb script per family --------------------------------------
ELFBASE="$(basename "$ELF")"; MDBSCRIPT="$SPIKE/_sim_${DEVICE}.mdb"
{
  echo "device DSPIC${DEVICE}"; echo "set system.disableerrormsg true"; echo "hwtool Sim"
  echo "program $ELFBASE"
  if [ "$FAMILY" = "33A" ]; then
    # 33A: run+breakpoint doesn't halt; step to the end, then read scalars.
    echo "reset"; for i in $(seq 1 500); do echo "stepi"; done
  else
    DLINE="$(grep -n 'done();' "$SPIKE/$CPP" | head -1 | cut -d: -f1)"
    echo "break ${CPP}:${DLINE}"; echo "run"
    for i in $(seq 1 10); do echo "print /x /datasize:2 R[7]"; done   # settle async run
  fi
  for v in "${VARS[@]}"; do echo "print /x /datasize:2 $v"; done
  echo "quit"
} > "$MDBSCRIPT"

log "Simulating on MPLAB X (this can take ~1-3 min)"
RAW="$(cd "$SPIKE" && timeout 240 "$MDB" "$(basename "$MDBSCRIPT")" 2>&1 | grep -iE '^0x[0-9a-f]')"
mapfile -t VALS < <(printf '%s\n' "$RAW" | tail -"${#VARS[@]}")

# --- 4. verify ----------------------------------------------------------------
log "Result vs expected"
PASS=1
norm(){ echo "$1" | tr 'A-F' 'a-f' | sed 's/^0x0*/0x/; s/^0x$/0x0/'; }
[ "${#VALS[@]}" -eq "${#VARS[@]}" ] || { echo "   only ${#VALS[@]}/${#VARS[@]} values read"; PASS=0; }
for i in "${!VARS[@]}"; do
    got="${VALS[$i]:-<none>}"
    if [ "$(norm "$got")" = "$(norm "${EXPECT[$i]}")" ]; then mark="ok  "; else mark="FAIL"; PASS=0; fi
    printf '   %-8s %-20s expect %-8s got %-10s %s\n' "${VARS[$i]}" "${LABELS[$i]}" "${EXPECT[$i]}" "$got" "$mark"
done
rm -f "$S_OUT" "$ELF" "$MDBSCRIPT" 2>/dev/null
echo ""
if [ "$PASS" = 1 ]; then echo "=== PASS — C++ verified on $DEVICE ==="; else echo "=== FAIL/INCOMPLETE on $DEVICE ==="; exit 1; fi
