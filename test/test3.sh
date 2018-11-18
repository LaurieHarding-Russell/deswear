#!/bin/bash

test {
	cd ~/
	rm -rf test
	mkdir test
	cd test
	git init
	touch cat1;
	git add .
	git commit --allow-empty -m "1"
	git add .
	touch cat2;
	git commit --allow-empty -m "2"
	git add .
	touch cat3;
	git commit --allow-empty -m "fuck 3"
	git add .
	touch cat4;
	git commit --allow-empty -m "4"
	git add .
	touch cat5;
	git commit --allow-empty -m "5"
}

test


