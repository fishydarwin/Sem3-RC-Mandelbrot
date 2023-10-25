# # # # # # # # # # # # # # # # # # # #
# MULTITHREADED MANDELBROT TCP SERVER #
# # # # # # # # # # # # # # # # # # # #

from easygraphics import *
from math import floor
from threading import Thread
from asyncio import Queue
import socket
from ast import literal_eval
import argparse

# command line arguments
parser = argparse.ArgumentParser(prog='TCP Mandelbrot server', usage='server -port <PORT> -ip <OPTIONALIP>',description='TCP Mandelbrot server renderer.')
parser.add_argument('-port', dest='port', default=1762, type=int, help="Port of the server")
parser.add_argument('-dbg', dest='dbg', type=bool, default=False, help="Display debug information")

args = parser.parse_args()

# window size, a common set of parameters...
resolution_x = 1000
resolution_y = 700

# adjustable render_scale parameter to zoom in/out of Mandelbrot set
render_scale = 2.75

# magic epsilon constant to know when to stop
epsilon = 0.0000001

# each chunk is 10% of the image width, and 10% of image height
chunk_size = 0.05

# bytes per region approximation
bytes_per_region_approx = int(chunk_size ** 2 * resolution_x * resolution_y * 12)

print(int(resolution_x / (resolution_x * chunk_size)) * int(resolution_y / (resolution_y * chunk_size)))
region_queue = Queue(int(resolution_x / (resolution_x * chunk_size)) * int(resolution_y / (resolution_y * chunk_size)))

def start_listen_thread():
    # boot up socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("", args.port))       # if you have trouble on OS X: lsof -i tcp:1762
    sock.listen()

    print("Bound to port ", args.port)
    input("Type anything and press enter to begin whenever you feel like it...\n >")
    print("Will start processing data to<->from connections!")
    
    while True:
        # maybe a socket has arrived...
        client_sock, client_address = sock.accept()
        print("Connection from", client_address, "received! Starting thread...")
        # send each "client_socket" connection as a parameter to a thread.
        Thread(target=server_thread, args=(client_sock,), daemon=True).start()

def server_thread(client_socket):
    client_socket.settimeout(15)
    region = ""
    try:
        # if nothing in queue, just wait
        while not region_queue.empty():

            # get a region from queue
            region = region_queue.get_nowait()
            start_region_x = region[0]
            end_region_x = region[1]
            start_region_y = region[2]
            end_region_y = region[3]
            print("Sending region to client:", start_region_x, end_region_x, start_region_y, end_region_y)
            
            # send region over, in an eval()-safe format...
            while True:
                client_socket.sendall(repr(region).encode())
                confirm = client_socket.recv(16).decode().strip()
                
                # confirm that client has correctly received the region
                if confirm == "ok":
                    break
                if(args.dbg):
                    print("Failed to send region, trying again...")

            # wait to receive result
            # this is an 2D RGB-tuple list of the form [[(255, 255, 255), (0, 0, 0), ...], [(...)]...] etc.
            # we will take it in chunks
            result = []
            received = ""
            while True:
                received = client_socket.recv(bytes_per_region_approx).decode().strip()
                    # all chunks have been sent
                if received == "done":
                    break
                try:
                    chunk = literal_eval(received)
                    result.append(chunk)
                    client_socket.sendall("ok".encode())
                except:
                    if(args.dbg):
                        print("Failed to receive, trying again...")
                    client_socket.sendall("again".encode())
                        

            print("Received calculated region", start_region_x, end_region_x, start_region_y, end_region_y)

            # place pixels, multithreaded!
            result_index_i = 0
            result_index_j = 0
            for i in range(start_region_x, end_region_x):
                for j in range(start_region_y, end_region_y):
                    result_rgb = result[result_index_i][result_index_j]
                    put_pixel(i, j, color_rgb(result_rgb[0], result_rgb[1], result_rgb[2]))
                    result_index_j += 1
                    if result_index_j == len(result[result_index_i]):
                        result_index_i += 1
                        result_index_j = 0
                            
            print("Done rendering region", start_region_x, end_region_x, start_region_y, end_region_y)

        # if nothing is happening, just instruct to wait
        # why?
        #client_socket.sendall("wait".encode())
    except socket.error:
        client_socket.close()
        region_queue.put_nowait(region)
        print("Connection to terminated, asking someone else to finish task...")
        return

def main():
    # initialize EasyGraphics window
    init_graph(resolution_x, resolution_y)
    # we will render everything pixel-by-pixel
    set_render_mode(RenderMode.RENDER_MANUAL)

    print("Start Mandelbrot TCP Server")

    # fill in queue with initial values
    for region_i in range(int(resolution_x / (resolution_x * chunk_size))):
        for region_j in range(int(resolution_y / (resolution_y * chunk_size))):
            # enqueue a region rectangle based on chunk_size...
            # this is represented by a tuple: (x1, x2, y1, y2, scale, epsilon, res_x, res_y)
            region_queue.put_nowait(
                (
                    floor((region_i * chunk_size) * resolution_x),
                    floor(((region_i + 1) * chunk_size) * resolution_x),
                    floor((region_j * chunk_size) * resolution_y),
                    floor(((region_j + 1) * chunk_size) * resolution_y),
                    render_scale,
                    epsilon,
                    resolution_x,
                    resolution_y
                )
            )

    print("Queue Size (Region Count)", region_queue.qsize(), "Approximate Max Bytes per Region", bytes_per_region_approx)

    # initialize socket off thread - this needs to be done to allow for rendering
    Thread(target=start_listen_thread, daemon=True).start()

    # just constantly render at 60fps, which in this
    # case is the refresh rate of our monitor
    while is_run():
        # 60fps render
        if delay_jfps(60):
            continue
    
    # when app is stopped, quit
    close_graph()
    
easy_run(main)
