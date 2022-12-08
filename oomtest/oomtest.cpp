#include <iostream>
#include <cstdlib>

using namespace std;

int main()
{
	    int *array=new int[10];
	        array[11] = 12;
		    int *p = new int(0);
		        p=0;
			    *p= 12;
			        free(array);
				    cout << "Hello World!" << endl;
				        return 0;
}

