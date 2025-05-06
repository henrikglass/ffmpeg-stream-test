#!/bin/bash
ffmpeg -stream_loop -1 -re -i videos/bbb.mp4 -f mpegts -b:v 10000k -vf "drawtext=fontfile=/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf: fontsize=30: text='%{localtime\:%T}': fontcolor=black@0.8: x=20: y=20" udp://localhost:1337

