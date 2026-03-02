// x[i] == i[x] - doesn't matter which is the index
// int reverse_subscript(long *arr, long expected)  {
    // taking address of both expression should yield same address
    // if (&3[arr] != &arr[3]) {
    // if (&3[arr] != &arr[3]) {
    //     return 7;
    // }
int main(void) {
    int i = 5;
    int b = -7;

    if (i != b) {
        return 1;
    }

    return b;
}
