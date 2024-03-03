// simple pow implementation
int pow(int x, int y) {
    if (y == 0) {
        return 1;
    }
    return x * pow(x, y - 1);
}