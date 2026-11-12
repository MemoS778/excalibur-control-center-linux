#!/usr/bin/env bash
# Run from the installed application directory. Hardware writes require root.
set -u
app_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [[ ${1:-} == --preview ]]; then
    exec env ECC_PREVIEW=1 "$app_dir/ecc"
fi
if (( EUID == 0 )); then
    "$app_dir/ecc"
else
    printf '%s\n' 'Excalibur RGB Control — klavye erişimi için yönetici parolası gerekiyor.'
    sudo -- "$app_dir/ecc"
fi
result=$?
if (( result != 0 )); then
    printf '\nUygulama hata koduyla kapandı: %s\n' "$result"
    read -r -p 'Hata çıktısını okuyup kapatmak için Enter tuşuna basın. ' _ || true
fi
exit "$result"
