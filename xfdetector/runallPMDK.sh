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

./run.sh hashmap_tx 0 5 race1
./run.sh hashmap_tx 50 10 race2

./run.sh hashmap_atomic 2 2 sem1 # segfault
./run.sh hashmap_atomic 2 2 sem2
./run.sh hashmap_atomic 2 2 sem3
./run.sh hashmap_atomic 2 2 sem4

# real bug
./run.sh hashmap_atomic 2 2