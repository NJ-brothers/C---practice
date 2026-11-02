#include <iostream>
#include <string>
using namespace std;
int main() {
    cout << "**********************************"<<"\n";
    cout << "Welcome to LAB-07\n";
    cout << "**********************************"<<"\n";

    string Your_Name;
    string Your_Rollno;
    string Your_Depart;

    cout << "Your name: ";
    getline(cin, Your_Name);
    cout << "Your RollNo.: ";
    getline(cin, Your_Rollno);
    cout << "Your Depart: ";
    getline(cin, Your_Depart);

    cout << "**********************************"<<"\n";
    cout << "Your name is "<< Your_Name <<"\n";
    cout << "Your Rollno is "<< Your_Rollno <<"\n";
    cout << "Your Depart is "<< Your_Depart << "\n";
    cout << "**********************************";

    return 0;
}