#!/bin/bash
# Opens around 8 test clients to stress-test the server...

python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py &
python3 mandelbrot_tcp_client.py
