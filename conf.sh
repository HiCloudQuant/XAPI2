#! /bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DIR/common/Queue:$DIR/apis/CTP:$DIR/include/CTP/linux64
export MALLOC_CHECK_=0

