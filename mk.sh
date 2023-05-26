#!/bin/bash
sudo systemctl stop motionplus
make
sudo systemctl start motionplus
