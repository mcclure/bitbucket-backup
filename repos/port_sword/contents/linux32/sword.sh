export TCODDIR=$(dirname $0)
export LD_LIBRARY_PATH=../lib32/libtcod-1.5.0:${LD_LIBRARY_PATH}
cd ${TCODDIR} && ./sword_bin $*
