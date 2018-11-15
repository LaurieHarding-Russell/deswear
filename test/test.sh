#!/bin/bash

test {
	cd ~/
	rm -rf test
	mkdir test
	cd test
	git init
	git commit --allow-empty -m "fuck 1"
	git commit --allow-empty -m "2"
	git commit --allow-empty -m "fuck 3"
	git commit --allow-empty -m "4"
	git commit --allow-empty -m "fuck 5"
}

test


