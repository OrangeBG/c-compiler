#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>

static DIR* get_directory(char *file_path);
static void run_directory_tests(DIR* directory, char *test_folder_name, bool is_negative_test);
 
int main(int argc, char** argv) {
  #ifdef _WIN32
    system("cls");
  #else
    system("clear");
  #endif

  if (argc == 1 || strcmp(argv[1],  "-v") == 0) {
    printf(">> RUNNING VALID TESTS <<\n");
    DIR * valid_directory = get_directory("../valid-tests");
    run_directory_tests(valid_directory, "valid-tests", false);
  }

  if (argc == 1 || strcmp(argv[1],  "-i") == 0) {
    printf("\n\n>> RUNNING INVALID TESTS <<\n");
    DIR * invalid_directory = get_directory("../invalid-tests");
    run_directory_tests(invalid_directory, "invalid-tests", true);
  }

  return 0;
}

static DIR* get_directory(char *file_path) {
  DIR *directory;
  directory = opendir(file_path);

  return directory;
}

static void run_directory_tests(DIR* directory, char *test_folder_name, bool is_negative_test) {
  struct dirent *en;

  if (directory) {
    int test_count = 1;
    for (en=readdir(directory); en!=NULL; en=readdir(directory)) {
      if (en->d_name[0] == '.') {
        continue;
      }
      
      printf("%d. %s\n", test_count,  en->d_name);

      char system_call[150] = "./c-compiler t ../../test/";
      strcat(system_call, test_folder_name);
      strcat(system_call, "/");
      strcat(system_call, en->d_name);
      int return_code = system(system_call);

      if (return_code == 0 && is_negative_test) {
        printf("** FAILURE TO THROW AN ERROR **\n");
      }

      test_count++;
    }

    closedir(directory);
  } else {
    printf("Directory not found");
  }
}
