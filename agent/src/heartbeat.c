// TODO: implement vom_heartbeat_start() as a dedicated pthread or a
//       timer-driven event in the main poll loop (latter is preferred
//       for less context-switch overhead on tablets).
// TODO: between heartbeats, sample /proc/stat and /proc/self/stat so
//       we can report meaningful cpu_pct instead of "last seen" only.
 
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "agent/heartbeat.h"
#include "agent/capabilities.h"

#define VOM_HEARTBEAT_INTERVAL 5  // seconds
#define VOM_HEARTBEAT_TIMEOUT 30  // seconds
#define VOM_HEARTBEAT_MAX_LOAD 0.9  // 90% cpu
#define VOM_HEARTBEAT_MIN_LOAD 0.01  // 1% cpu
#define VOM_HEARTBEAT_MAX_MEM 0.9  // 90% memory
#define VOM_HEARTBEAT_MIN_MEM 0.01  // 1% memory
#define VOM_HEARTBEAT_MAX_DISK 0.9  // 90% disk
#define VOM_HEARTBEAT_MIN_DISK 0.01  // 1% disk

int main(void) {
    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO; // Monitors normal input
    fds[0].events = POLLIN; // Looks if any data is coming in

    int running = 1;
    while (running) {
        int ret = poll(fds, 1, VOM_HEARTBEAT_INTERVAL * 1000); // polls 5 sec

        if (ret == -1) {
            perror("poll");
            return 1;
        } else if (ret == 0) {
            printf("No data within %d seconds. We're cooked\n", VOM_HEARTBEAT_INTERVAL);
        } else {
            if (fds[0].revents & POLLIN) {
                char buffer[1024];
                int len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                if (len > 0) {
                    buffer[len] = '\0';
                    printf("Received input: %s\n", buffer);
                    running = 0; // Stop the loop after receiving input
                }
            }
        }
    }

    return 0;
}
