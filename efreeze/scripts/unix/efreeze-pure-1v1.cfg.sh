#!/bin/sh

PORT="27983"


cd ../../../
quake3 +set dedicated 2 +set fs_game efreeze +set net_port $PORT +exec efreeze/cfgs/efreeze-pure-1v1.cfg