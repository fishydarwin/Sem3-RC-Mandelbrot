# # # # # # # # # # # # # # # # # # # # # # # # # #
# TCP MANDELBROT CLIENT FOR TCP MANDELBROT SERVER #
# # # # # # # # # # # # # # # # # # # # # # # # # #

from math import floor
import socket
from ast import literal_eval
import random
# random rgb to identify client

def clamp(num, min_value, max_value):
   return max(min(num, max_value), min_value)

# run calculation subroutine
def run_calculation(start_region_x, end_region_x, start_region_y, end_region_y,
                    render_scale, epsilon, resolution_x, resolution_y):
    # define our points
    points = [[0 for _ in range(start_region_y, end_region_y)] for _ in range(start_region_x, end_region_x)]

    # useful later
    scale_x = render_scale * (resolution_x / resolution_y)

    # perform rendering...
    for i in range(start_region_x, end_region_x):
        # 7 and 11 are magic numbers, that scale the image properly
        x = (i - 7 * resolution_x / 11) / resolution_x * scale_x
        for j in range(start_region_y, end_region_y):

            # the 2 here is also magic
            y = (j - resolution_y / 2) / resolution_y * render_scale

            # iterate Mandelbrot fc(z) function 100 times
            z = 0
            max_iter_steps = 100
            iter_steps = 0
            for _ in range(max_iter_steps):
               z = z * z + complex(x, y)
               if ((abs(z.real) < epsilon) and (abs(z.imag) < epsilon)):
                   # make it black...
                   points[i - start_region_x][j - start_region_y] = (0, 0, 0)
                   break
               if ((abs(z.real) > 10) or (abs(z.imag) > 10)):
                   break
               iter_steps += 1
            
            point_found_val = floor((iter_steps / max_iter_steps) * 169)
            # should look like a pink-ish Mandelbrot...
            points[i - start_region_x][j - start_region_y] = (
                clamp(floor(point_found_val * 0.9), 0, 255),           # R
                clamp(floor(point_found_val * 1 ), 0, 255),             # G
                clamp(floor(point_found_val * 2.85), 0, 255)           # B
            )

    # return calculation
    return points

# connect to server...
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# sock.connect(("str(input("Which IP to connect to? > "))", 1762))
sock.connect(("192.168.1.4", 1762))
sock.settimeout(15)

print("Awaiting further instructions from Mandelbrot TCP server...")
while True:
    try: # wait for KeyboardInterrupt or something... basically app close

        # 1000 bytes is enough, the render region is never that big...
        while True:
            received = sock.recv(1000).decode().strip()
            # if received == "wait":
            #     continue
            try:
                result = literal_eval(received)
                sock.sendall("ok".encode())
                break
            except:
                sock.sendall("again".encode())

        start_region_x = result[0]
        end_region_x = result[1]
        start_region_y = result[2]
        end_region_y = result[3]
        render_scale = result[4]
        epsilon = result[5]
        resolution_x = result[6]
        resolution_y = result[7]

        print("Received a region to calculate:", 
            start_region_x, end_region_x, start_region_y, end_region_y,
            render_scale, epsilon, resolution_x, resolution_y)
        
        points = run_calculation(start_region_x, end_region_x, start_region_y, end_region_y,
                        render_scale, epsilon, resolution_x, resolution_y)
        
        print("Done rendering region, sending...")
        #sock.sendall("sending".encode())

        # send result in chunks
        for chunk in points:
            print("Sending chunk...", len(repr(chunk)))
            sock.sendall(repr(chunk).encode())
            # receive confirmation
            confirm = sock.recv(16).decode().strip()
            # if not ok, send again...
            while confirm != "ok":
                print("Failed to send, trying again...")
                sock.sendall(repr(chunk).encode())
                print("Sent again...")
                confirm = sock.recv(16).decode().strip()

        # send 'done' at the end
        #for _ in range(4):
        sock.sendall("done".encode())

        # wait for more!...
        print("Sent region over...")
        print("Awaiting further instructions from Mandelbrot TCP server...")

    except socket.error:
        sock.close()
        print("Connection terminated, shutting down...")
        break
    except Exception as ex:
        continue
