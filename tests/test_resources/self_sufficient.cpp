

int gpublic = 69;

static int gprivate = 269;


void add2globals(int x) {
    gpublic += x;
    gprivate += x;
}


int pow_naive(int base, int power) {
    for (int i = 0; i < power; ++i) {
        base *= base;
    }
    return base;
}
