// Maximum size of the print buffer in assembly. Basically I am too lazy to
// implement dynamic string size chunking to the print buffer
#define PRINT_BUFFER_SIZE 4096
#define MAX_STRING_SIZE (PRINT_BUFFER_SIZE / 2)