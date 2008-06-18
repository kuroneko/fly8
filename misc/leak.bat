@echo off
awk -f leak.awk <fly.log >leak.log
