## Sem3-RC-Mandelbrot

This is a TCP/IP project me and my colleague [@NinthBurn](https://www.github.com/NinthBurn) made for our Computer Networks class.  
It includes a server and a client:
- The server does not draw anything, but opens a window made using EasyGraphics in Python 3.
- The clients can connect to the server, and they will receive instructions on what to draw & send it back to the server.

There are two implementations of the client - one in Python (original) and another in C++ (by my colleague).  
I kindly thank him for his tremendous work on deciphering weird Socket API bugs.

### What does it do?

The server contains a queue of generated regions in the beginning and will send one region to each client into smaller chunks to ensure that even in bad connection situations, the server and client can perform the task. The client then will calculate its drawing (in this case, the **Mandelbrot Set**) and submit only the results back to the server.

If a connection is lost mid-way, then the server will enqueue the lost region back into the main queue, so it should be drawn at the end anyways by whichever client is willing to take the task.

Ultimately, the image that is calculated should result in a pinkish-blue Mandelbrot Set.  
The server's code has a few parameters at the top of the file, which let you change the zoom, precision etc...
