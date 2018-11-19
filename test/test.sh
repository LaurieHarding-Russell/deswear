#!/bin/bash

test {
	cd ~/
	rm -rf test
	mkdir test
	cd test
	git init

	touch 1
	git add .
	git commit --allow-empty -m "fuck 1"

	touch 2
	git add .
	git commit --allow-empty -m "2"

	touch 3
	git add .
	git commit --allow-empty -m "fuck 3"

	touch 4
	git add .
	git commit --allow-empty -m "4"

	touch 5
	git add .
	git commit --allow-empty -m "fuck 5"
}

test


