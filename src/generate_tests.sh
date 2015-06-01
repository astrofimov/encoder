#!/bin/bash

mkdir -p test
for i in {1..10}
do
    cp test.wav test/test$i.wav
done