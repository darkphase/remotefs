
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TextTestRunner.h>

int main(int argc, char **argv)
{
	CppUnit::TextTestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	
	runner.addTest(registry.makeTest());
	bool success = runner.run(argc > 1 ? argv[1] : "");

	return (success ? 0 : 1);
}

