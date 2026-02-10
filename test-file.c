int main(void) {
    //static int nested[1][2] = { { 0, 0 } };

    int array[3] = { 1, 2, 3 };
    int *ptr = array - 1;

    int help = *ptr;
    return 0;
}
