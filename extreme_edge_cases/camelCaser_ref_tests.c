/**
 * extreme_edge_cases
 * CS 241 - Fall 2022
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

int main() {
    // Enter the string you want to test with the reference here.
    char *input = "hello. welcome to cs241";

    // This function prints the reference implementation output on the terminal.
    print_camelCaser(input);

    char * big_text = """Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ante sapien, egestas quis urna nec, venenatis fermentum lacus. In ut ex ligula. Aliquam vehicula, diam vel ultrices rutrum, massa est dictum enim, ac pellentesque quam purus a justo. Phasellus non nibh orci. Fusce scelerisque id leo eget accumsan. Fusce volutpat nibh dui, ut dignissim magna rutrum in. Pellentesque eu convallis tellus, ac aliquam velit. Suspendisse volutpat aliquam elit ut convallis. Aliquam sit amet dictum dui. Suspendisse sodales enim nec justo fringilla laoreet eget id nibh. Pellentesque tristique odio eu ante vestibulum, eu dignissim quam gravida. Suspendisse pharetra odio tellus, non sodales diam aliquet molestie""";
    print_camelCaser(big_text);

    // Put your expected output for the given input above.
    char *correct[] = {"hello", NULL};
    char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // Compares the expected output you supplied with the reference output.
    printf("check_output test 1: %d\n", check_output(input, correct));
    printf("check_output test 2: %d\n", check_output(input, wrong));

    // Feel free to add more test cases.
    return 0;
}
