int strsize(char* arr){
	int counter=0;
	while(arr[counter++]!='\0');
	return counter-1;
}

int strcmp(char* s1,char* s2){
	while(*s1 && *s2 && *s1 == *s2){s1++;s2++;}
	return *s1-*s2;
}
