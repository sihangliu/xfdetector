#!/bin/bash
./run.sh btree 5 5 race1
./run.sh btree 5 5 race2
./run.sh btree 0 10 race3
./run.sh btree 5 5 race4
./run.sh btree 5 5 perf1
./run.sh btree 5 5 perf2

./run.sh ctree 0 10 race1
./run.sh ctree 0 10 perf1

./run.sh rbtree 1 1 race1