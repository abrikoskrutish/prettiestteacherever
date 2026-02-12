#include <iostream>

// однострочный комментарий

using namespace std;

/*
   многострочный
   комментарий
*/

int main() {
    int x;
    int y;

    x = 2;
    y = 2;

    if (x < y) {
        cout << "x less y" << endl;
    }
    else if (x > y) {
        cout << "x greater y" << endl;
    }
    else {
        cout << "x equal y" << endl;
    }
}