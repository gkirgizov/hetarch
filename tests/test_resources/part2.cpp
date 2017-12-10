

extern int important_data;
extern int square(int);


int important_result;


int pow_smart(int base, unsigned int power) {
    if (power == 0) {
        return 1;
    }
    if (power % 2 == 0) {
        return pow_smart(square(base), power / 2);
    } else {
        return base * pow_smart(base, power - 1);
    }
//    return power % 2 == 0 ? pow_smart(square(base), power >> 1) : base * pow_smart(base, power - 1);
}


void try_pow() {
    important_result = pow_smart(important_data, 5);
}
