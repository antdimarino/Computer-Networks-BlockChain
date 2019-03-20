#!/bin/bash

gcc nodon.c -o nodon -lpthread
gcc blockserver.c -o blockserver -lpthread
gcc blockclient.c -o blockclient -lpthread


