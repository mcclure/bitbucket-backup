if [ -z $1 ]; then echo "Must enter a .nim file as argument"; exit 1; fi

# --tlsEmulation:off seems to be needed to make SDL+threads on Windows not crash
# It seems to only be available in Nim 0.13.1 (current beta version), however
nim compile --threads:on --tlsEmulation:off --run $1
