#!/bin/bash

test {
	cd ~/
	rm -rf test
	mkdir test
	cd test
	git init
	git commit --allow-empty -m "fuck 1"
	git checkout -b b1
	git commit --allow-empty -m "2.1"
	git checkout master
	git commit --allow-empty -m "2.2"
	git merge b1
	git commit --allow-empty -m "4"
}

test


