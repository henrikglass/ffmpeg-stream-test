#!/bin/bash
ffmpeg -re -i bbb.mp4 -f mpegts -b:v 10000k udp://localhost:1337

