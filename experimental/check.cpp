#include<iostream>
using namespace std;

int main(){
    int n;
    int result = 0;
    while(cin>>n){
        result ^= n;
    }
    cout<<result<<endl;
    return 0;
}