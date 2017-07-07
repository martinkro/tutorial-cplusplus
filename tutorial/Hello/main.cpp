#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

class A
{
public:
	int a;

public:
	virtual ~A()
	{
		cout << "A destructor" << endl;
	}
};

int GetNum(int a)
{
	if (a > 10)
	{
		return a * -1;
	}
	else
	{
		return a * -2;
	}
}

int main(int argc, char* argv[])
{
	cout << "C++" << endl;
	A* obj = new A();
	delete obj;
	int a = -1;
	int b = 8;
	int c = a % b;
	cout << c << endl;

	int d = GetNum(a);
	c = d % 8;
	cout << c << endl;

	string hello = "Hello ";
	std::stringstream ss;
	ss  << 20;
	hello += ss.str();
	cout << hello << endl;
	return 0;
}