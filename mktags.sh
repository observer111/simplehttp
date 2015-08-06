#!/bin/sh
ctags `find | grep -E '\.(c|cpp|h|hpp)'`
