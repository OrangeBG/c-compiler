/* subtract pointers to two elements in a multi-dimensional array */
int get_multidim_ptr_diff(double (*ptr1)[3][5], double (*ptr2)[3][5]) {
    return (ptr2 - ptr1);
}

int main(void) {
    static double multidim[6][7][3][5];

    if (get_multidim_ptr_diff(multidim[2] + 1, multidim[2] + 4) != 3) {
        return 3;
    }

    return 0;
}
