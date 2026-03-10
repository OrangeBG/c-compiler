//@Bug: Abstract declarator is not properly iterating through all dimensions of the array in the 'if' statement
int main(void) {
    int multi_dim[2][3] = {{0, 1, 2}, {3, 4, 5}};

    // get pointer to whole array
    int (*array_pointer)[2][3] = &multi_dim;

    // get pointer to one row
    int (*row_pointer)[3] = (int (*)[3]) array_pointer;

    // now set row_pointer back to the beginning, cast it back to an array,
    // and make sure it round-tripped
    row_pointer = row_pointer - 1;
    if ((int (*)[2][3]) row_pointer != array_pointer) {
        return 5;
    }

    return 0;
}
