/*
# # # # # # # # # # # # # # # # # # # # # # # # # #
# TCP MANDELBROT CLIENT FOR TCP MANDELBROT SERVER #
# # # # # # # # # # # # # # # # # # # # # # # # # #
*/

/*
Keep in mind the Python implementation is better, this is just me practicing with socket in C/C++
Initially, the code I wrote only worked in Linux, but I was able to add Windows support thanks to
		---	 https://gist.github.com/FedericoPonzi/2a37799b6c601cce6c1b     ---
		--- https://gist.github.com/willeccles/3ba0741143b573b74b1c0a7aea2bdb40 ---

one problem : for some reason, the Windows client does not close if you run it after everything has been rendered
I have no idea why this keeps happening, making it work for this crap OS was hard enough
*/

#define USER_TIMEOUT 15000

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
	#include <ws2tcpip.h>
#else
	# include <arpa/inet.h>
	# include <sys/socket.h>
	# include <sys/types.h>
	# include <sys/time.h>
	# include <netdb.h>
	# include <unistd.h>
#endif
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h> 

/* Types */
#ifdef _WIN32
	typedef SOCKET socket_t;
	typedef int socklen_t;
	typedef signed long long int ssize_t;
#else
	typedef int socket_t;
	# define INVALID_SOCKET -1
	# define closesocket(x) close(x)
#endif 

inline int sock_init() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2,2), &wsaData);
#else
    return 0;
#endif 
}

inline void sock_cleanup() {
#ifdef _WIN32
    WSACleanup();
#else
    return;
#endif 
}

inline int sock_setrecvtimeout(socket_t sock, int32_t ms) {
#ifdef _WIN32
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<char*>(&ms), sizeof(ms));
#else
    struct timeval tv;
    tv.tv_usec = 1000L * ((long)ms - (long)ms / 1000L * 1000L);
    tv.tv_sec = (long)ms / 1000L;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
            reinterpret_cast<void*>(&tv), sizeof(tv));
#endif 
}

inline int sock_setsendtimeout(socket_t sock, int32_t ms) {
#ifdef _WIN32
    return setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<char*>(&ms), sizeof(ms));
#else
    struct timeval tv;
    tv.tv_usec = 1000L * ((long)ms - (long)ms / 1000L * 1000L);
    tv.tv_sec = (long)ms / 1000L;
    return setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<void*>(&tv), sizeof(tv));
#endif 
}



typedef struct{
	int r, g, b;
}Color;

using namespace std;

string colorToStr(Color c){
	string result = "(" + to_string(c.r) + ", " + to_string(c.g) + ", " + to_string(c.b) + ")";
	return result;
}

string chunkToStr(Color* points, int num_cols){
	string result = "[";
	for(int i = 0; i < num_cols - 1; ++i){
		result += colorToStr(points[i]) + ", ";
	}
	
	result += colorToStr(points[num_cols - 1]) + "]";
	return result;
}

int clamp(int num, int min_value, int max_value){
	return max(min(num, max_value), min_value);
}

Color** run_calculation(int start_region_x, int end_region_x, int start_region_y, int end_region_y, double render_scale, double epsilon, int resolution_x, int resolution_y){
	// allocate points matrix
	int num_rows = end_region_x - start_region_x;
	int num_cols = end_region_y - start_region_y;
	
	//Color** points = (Color**)(malloc(num_rows * sizeof(Color*)));	-- don't use this merci
	Color** points = new Color*[num_rows];
	
	for(int i = 0; i < num_rows; ++i)
		points[i] = new Color[num_cols];	
		//points[i] = (Color*)(malloc(num_cols * sizeof(Color)));
	
	// proceed with rendering
	double scale_x = render_scale * ((float)resolution_x / resolution_y);
	int point_found_val;
	
	for(int i = start_region_x; i < end_region_x; ++i){
		double x = (i - 7.0 * resolution_x / 11.0) / resolution_x * scale_x;
		for(int j = start_region_y; j < end_region_y; ++j){
			double y = (j - resolution_y / 2.0) / resolution_y * render_scale;
			int max_iter_steps = 100, iter_steps = 0;
			
			complex<double> z = 0 + 0 * I;
			
			for(int t = 0; t < max_iter_steps; ++t){
               complex<double> cy = 0 + y * I;
			   z = z * z + x + cy;

               if ((abs(real(z)) < epsilon) && (abs(imag(z)) < epsilon))
			   {
					// make it black
					points[i - start_region_x][j - start_region_y].r = 0;
					points[i - start_region_x][j - start_region_y].g = 0;
					points[i - start_region_x][j - start_region_y].b = 0;
			   }
                   
                   
               if ((cabs(real(z)) > 10) || (cabs(imag(z)) > 10))
                   break;
			   
               iter_steps++;
            }
			point_found_val = floor(((double)iter_steps / max_iter_steps) * 169);
            // I'm blue
            points[i - start_region_x][j - start_region_y].r = clamp(floor(point_found_val * 0.9), 0, 255);
			points[i - start_region_x][j - start_region_y].g = clamp(floor(point_found_val * 1 ), 0, 255); 
            points[i - start_region_x][j - start_region_y].b = clamp(floor(point_found_val * 2.85), 0, 255);                        
		}
	}
	return points;
}

void parseArgs(int argc, char** argv, string& ip, int* port){
	if(argc == 1)
		return;
	else if(argc == 2)
		ip = argv[1];
	else if(argc == 3){
		ip = argv[1];
		*port = atoi(argv[2]);
	}
	else{
		printf("Usage: %s <IP> [<PORT>]\nDefault port is 1762\n", argv[0]);
		exit(-1);
	}
}

int main(int argc, char** argv){ 
	string ipadr = "192.168.1.3";
	int port = 1762;
	parseArgs(argc, argv, ipadr, &port);
	
	struct sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	int sock_fd;
	char received[1000];

    // connect to server
	printf("Connecting to %s : %d\n", ipadr.c_str(), port);
	sock_init();
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);		

	if(sock_fd < 0){
		printf("Cannot create socket\n");
		closesocket(sock_fd);
		sock_cleanup();
		return -1;
	}
		
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);
	sock.sin_addr.s_addr = inet_addr("192.168.1.3"); // server IP
		
	// timeout options; default is 15 seconds
	sock_setrecvtimeout(sock_fd, USER_TIMEOUT);
	sock_setsendtimeout(sock_fd, USER_TIMEOUT);

	// time to connect
	if ((connect(sock_fd, (struct sockaddr*)&sock, sizeof(sock))) < 0) {
		printf("Connection failed, closing application...\n");
		closesocket(sock_fd);
		sock_cleanup();
		return -1;
	}
    
    printf("Awaiting further instructions from Mandelbrot TCP server...\n");
    
	while(true){
		float result[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		Color** points;
		while(true){
			recv(sock_fd, received, sizeof(received), 0);
			received[strlen(received)] = '\0';
							
			// found this on the net, really cool way of doing it; thanks
			int count = sscanf(received, "(%f, %f, %f, %f, %f, %f, %f, %f)", &result[0], &result[1], &result[2], &result[3], &result[4], &result[5], &result[6], &result[7]);
				
			if (count == 8) {
				send(sock_fd, "ok", strlen("ok"), 0);
				break;
			} else {
				send(sock_fd, "again", strlen("again"), 0);
			}
		}
			   
		int start_region_x, end_region_x, start_region_y, end_region_y;
		float render_scale, epsilon;
		int resolution_x, resolution_y;
			
        start_region_x = (int)result[0];
        end_region_x = (int)result[1];
        start_region_y = (int)result[2];
		end_region_y = (int)result[3];
		render_scale = result[4];
		epsilon = result[5];
		resolution_x = (int)result[6];
		resolution_y = (int)result[7];

		printf("Received a region to calculate: %d %d %d %d %f %f %d %d\n", 
                start_region_x, end_region_x, start_region_y, end_region_y,
                render_scale, epsilon, resolution_x, resolution_y);
            
        points = run_calculation(start_region_x, end_region_x, start_region_y, end_region_y,
                    render_scale, epsilon, resolution_x, resolution_y);
            
        printf("Done rendering region, sending...\n");

		char confirm[16] = "";
		int error_count = 0;
		int num_rows = end_region_x - start_region_x;
		int num_cols = end_region_y - start_region_y;
			
		for(int i = 0; i < num_rows; ++i){
			string chunk = chunkToStr(points[i], num_cols);
			send(sock_fd, chunk.c_str(), chunk.length(), 0);
			
			recv(sock_fd, confirm, sizeof(confirm), 0);
			confirm[strlen(confirm)] = '\0';
				
			if (strcmp(confirm, "ok") == 0) {
				error_count = 0;
			} else {
				error_count++;
			}
				
			if (error_count >= 10) {
				break;
			}
		}
			
		// just C things
		for(int i = 0; i < num_rows; ++i)
			delete[] points[i];
			
		delete[] points;

        if(strcmp(confirm, "ok") != 0){
			printf("Failed to send render, reconnecting...\n");
			closesocket(sock_fd);
			sock_cleanup();
			
			printf("Connecting to %s : %d\n", ipadr.c_str(), port);
			sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	

			if(sock_fd < 0){
				printf("Cannot create socket\n");
				return -1;
			}
			
			sock_setrecvtimeout(sock_fd, USER_TIMEOUT);
			sock_setsendtimeout(sock_fd, USER_TIMEOUT);
				
			if ((connect(sock_fd, (struct sockaddr*)&sock, sizeof(sock))) < 0) {
				printf("Connection failed, closing application...\n");
				return -1;
			}
            continue;
		}
		// send 'done' at the end
        send(sock_fd, "done", strlen("done"), 0);
		// wait for more!...
        printf("Sent region over...\n");
		printf("Awaiting further instructions from Mandelbrot TCP server...\n");
	}
	closesocket(sock_fd);
	sock_cleanup();
	return 0;
}
