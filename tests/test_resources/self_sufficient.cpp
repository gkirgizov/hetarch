

int gpublic = 69;

static int gprivate = 269;

volatile int gpublic_vol = 0;


extern "C"
void add2globals(int x) {
    gpublic += x;
    gprivate += x;

    int ldummy = 0;
    ldummy = 2 + gpublic;
}


namespace test_namespace {

extern "C"
void use_volatile(int x) {
    int current_val = gpublic_vol;
    gpublic_vol = current_val + x;

    add2globals(current_val);
}

}


extern "C"
int use_branch(bool flag) {
    if (flag) {
        return 69;
    } else {
        return 369;
    }
}


extern "C"
int pow_naive(int base, int power) {
    for (int i = 0; i < power; ++i) {
        base *= base;
    }
    return base;
}
