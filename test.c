char  find(char *str){
    int tmp[0xff] = {0};
    char temp[2] = {0};
    
    while(*str){
        tmp[*str]++;
        (tmp[*str] > temp[0])?(temp[0]=tmp[*str],temp[1]=*str):0;
        str++;
    }
    return temp[1];

}

int main(){
    char * str = "aabbccddddeeeeeeeeeeeefggggaaa";
    cout<<find(str);



}
