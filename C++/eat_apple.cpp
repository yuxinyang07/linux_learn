#include<stdlib.h>
#include<bits/stdc++.h>
using namespace std;





int main(int argc ,char **argv)
{
    unsigned  int n,x,y,z;
    cin>>n>>x>>y;
    if(x ==0) //如果输入的为0，无意义。直接返回
    {
        return 0;
    }
    if(y < x)
        z = n -1;
    else if((y%x)>0)
        z = n-(y/x)-1;
    else
        z= n-(y/x);
    cout<<z<<endl;
        
}
