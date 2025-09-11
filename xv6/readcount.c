#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  int initial_count, after_count;
  char buf[100];
  int fd;
  
  // Get initial count
  initial_count = getreadcount();
  printf("Initial read count: %d bytes\n", initial_count);
  
  // Read from a file
  fd = open("README", O_RDONLY);
  if(fd < 0) {
    printf("Failed to open README\n");
    exit(1);
  }
  
  // Read 100 bytes
  if(read(fd, buf, 100) != 100) {
    printf("Failed to read 100 bytes\n");
    close(fd);
    exit(1);
  }
  close(fd);
  
  // Get count after reading
  after_count = getreadcount();
  printf("Read count after reading 100 bytes: %d bytes\n", after_count);
  
  // Verify increase
  if(after_count - initial_count == 100) {
    printf("Test passed: read count increased by 100 bytes\n");
  } else {
    printf("Test failed: expected increase of 100 bytes, got %d bytes\n", 
           after_count - initial_count);
  }
  
  exit(0);
}