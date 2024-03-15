double pow(double x, double y){
    if (y==0){
        return 1;
    }
    return  x * pow(x, y-1);
}