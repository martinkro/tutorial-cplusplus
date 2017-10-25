#include <iostream>
#include <list>
#include <memory>
using namespace std;

class ITestHandler
{
public:
	virtual void handle(const char* msg) = 0;
};

class HandlerA :public ITestHandler
{
public:
	virtual void handle(const char* msg)
	{
		cout << "[HandlerA]:" << msg << endl;
	}

	HandlerA()
	{
		cout << "[HandlerA]constructor" << endl;
	}

	virtual ~HandlerA()
	{
		cout << "[HandlerA]destructor" << endl;
	}
};

class HandlerB :public ITestHandler
{
public:
	virtual void handle(const char* msg)
	{
		cout << "[HandlerB]:" << msg << endl;
	}

	HandlerB()
	{
		cout << "[HandlerB]constructor" << endl;
	}

	virtual ~HandlerB()
	{
		cout << "[HandlerB]destructor" << endl;
	}
};



using ITestHandlerPtr = shared_ptr<ITestHandler>;
class HandleEngine
{
public:
	HandleEngine(){
		cout << "[HandleEngine]constructor" << endl;
		m_handlers.clear();
		m_handlerB = make_shared<HandlerB>();
	}
	~HandleEngine() {
		cout << "[HandleEngine]destructor" << endl;
		m_handlers.clear();
	}

public:
	void registerHandler(ITestHandlerPtr handler)
	{
		cout << "[registerHandler]:" << handler.use_count() << endl;
		m_handlers.push_front(handler);
	}

	void unregisterHandler(ITestHandlerPtr handler)
	{
		for (auto x = m_handlers.begin(); x != m_handlers.end(); x++)
		{
			if ((*x) == handler)
			{
				m_handlers.erase(x);
				break;
			}
		}
		cout << "[unregisterHandler]:" << handler.use_count() << endl;
	}

	void handle(const char* msg)
	{
		for (auto x : m_handlers)
		{
			x->handle(msg);
		}
	}

private:
	list<ITestHandlerPtr> m_handlers;
	shared_ptr<HandlerB> m_handlerB;
};

auto g_engine = make_shared<HandleEngine>();
class HandlerC :public ITestHandler
{
public:
	virtual void handle(const char* msg)
	{
		cout << "[HandlerC]:" << msg << endl;
		
	}

	HandlerC()
	{
		cout << "[HandlerC]constructor" << endl;
		g_engine->registerHandler(shared_ptr<HandlerC>(this));
		cout << "[HandlerC]constructor END" << endl;
	}

	virtual ~HandlerC()
	{
		cout << "[HandlerC]destructor" << endl;
		g_engine->unregisterHandler(shared_ptr<HandlerC>(this));
		cout << "[HandlerC]destructor END" << endl;
	}

};

class HandlerD :public ITestHandler,public std::enable_shared_from_this<HandlerD>
{
public:
	virtual void handle(const char* msg)
	{
		cout << "[HandlerD]:" << msg << endl;

	}

	HandlerD()
	{
		cout << "[HandlerD]constructor" << endl;
		//g_engine->registerHandler(shared_from_this());
	}

	virtual ~HandlerD()
	{
		cout << "[HandlerD]destructor" << endl;
		//g_engine->unregisterHandler(shared_from_this());
	}

};


int main()
{
	cout << "C++ 11 pointer" << endl;
	//auto engine = make_shared<HandleEngine>();
	//auto handleA = make_shared<HandlerA>();
	//auto handleB = make_shared<HandlerB>();
	//auto handleC = make_shared<HandlerC>();
	//auto handleD = make_shared<HandlerD>();

	//g_engine->registerHandler(handleA);
	//g_engine->registerHandler(handleB);
	//g_engine->handle("msg xxx");

	//cout << handleA.use_count() << endl;
	//cout << handleB.use_count() << endl;

	//g_engine->unregisterHandler(handleA);
	//g_engine->handle("msg yyy");

	//cout << handleA.use_count() << endl;
	//cout << handleB.use_count() << endl;
	{
		//auto handleC = make_shared<HandlerC>();
		g_engine->handle("msg xxx");
		g_engine->handle("msg yyy");
	}

	cout << "PROGRAM END" << endl;

	return 0;
}