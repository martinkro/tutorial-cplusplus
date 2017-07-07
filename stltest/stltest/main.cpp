#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <cstdio>
#include <memory>

using namespace std;

typedef struct _res_check_result_info
{
	uint32_t report_type;
	string path;
}TSS_RES_RESULT_INFO;

class A
{
public:
	void Add()
	{
		m_ptr.push_back(std::shared_ptr<A>(this));
	}

	void Delete()
	{
		m_ptr.remove(std::shared_ptr<A>(this));
	}

	~A()
	{
		cout << "A destructor" << endl;
	}

private:
	std::list<std::shared_ptr<A> > m_ptr;
};

void Test()
{
	A* a = new A();
	a->Add();
	a->Delete();

}

int main()
{
	Test();
	cout << "STL Test" << endl;
	TSS_RES_RESULT_INFO a = TSS_RES_RESULT_INFO();
	a.path = "Hello,WorldHello,WorldHello,WorldHello,WorldHello,WorldHello,World";
	TSS_RES_RESULT_INFO b = a;

	list<TSS_RES_RESULT_INFO> list_report = list<TSS_RES_RESULT_INFO>(10);
	list_report.clear();

	
	cout << a.path << endl;
	cout << b.path << endl;
	return 0;
}