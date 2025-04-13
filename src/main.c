#include <stdio.h>
#include <stdlib.h>

char* load_file(const char* file_path); 

int main(int argc, const char* argv[]) {
  #ifdef _WIN32
    system("cls");
  #else
    system("clear");
  #endif

  load_file(argv[1]);

  printf("sucessfully loaded file");
}

char* load_file(const char* file_path) {
  FILE* file = fopen(file_path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file: \"%s\".\n", file_path);
    exit(1);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read: \"%s\".\n", file_path);
    exit(1);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}
