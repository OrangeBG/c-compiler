int main(void) {
    // long nested_array[2][3][4] = {{{0}}, {{-12, -13, -14, -15}, {-16}}};
    //
    // if (nested_array[1][2][1] != 100) {
    //     return 2;
    // }

    long nested_array[2][1] = {{0}, {3}};

    if (nested_array[1][0] != 100) {
        return 2;
    }

    return 0;
}
