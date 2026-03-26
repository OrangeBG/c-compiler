// Pointer arithmetic with +=/-=
// int int_array(void) {
//     int arr[1] = {1};
//     int *ptr = arr;

//     return 0;
// }

int nested_array_param(int a[2][3]) {
    a[1][1] = 1;
    return 0;
}

/*
convert_by_assignment(e, target_type):
if get_type(e) == target_type:
return e
if get_type(e) is arithmetic and target_type is arithmetic:
return convert_to(e, target_type)
if is_null_pointer_constant(e) and target_type is a pointer type:
return convert_to(e, target_type)
else:
fail("Cannot convert type for assignment")
 */
