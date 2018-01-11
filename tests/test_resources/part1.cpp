

extern "C"
int pass_through(int x) {
    return x;
}


int important_data = pass_through(42);


extern "C"
int square(int x) {
    return x * x;
}


extern "C"
int pow4(int x) {
    return square(square(x));
}

