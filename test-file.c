int main(void) {
    //int my_array[3] = {1, 2, 3};
    //int (*my_pointer)[3] = &my_array;
    int nested_array[2][2] = {{1,2}, {3,4}};
    **nested_array = 10;

    return 0;
}
