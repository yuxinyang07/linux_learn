#!/bin.sh


perf  -F 99 -g -p  $(pidof  ultracore-makers) --sleep 30

perf script > out.perf
