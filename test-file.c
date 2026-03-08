int set_nested_element(int (*arr)[2], double *arr_dbl, int i, int j) {
    arr_dbl[i] = 8;
    arr[i][j] = 10;

    return 0;
}
