/*******************************************************************************
 *  Resource management library
 *  Unit test
 *  
 *  © 2026, Sauron
 ******************************************************************************/

#include <cassert>
#include <iostream>
#include <string>

using std::cerr;
using std::string;

extern string unescape(const string &input);

int main(int argc, char ** argv) {
    try {
        assert(unescape("")=="");
        assert(unescape("abc123")=="abc123");
        assert(unescape("\\a\\b\\e\\f\\n\\r\\t\\v")=="\a\b\e\f\n\r\t\v");
        assert(unescape("\\!37")=="\x25");
        assert(unescape("\\+37")=="\x4a");
        assert(unescape("\\-37")=="\x49");
        assert(unescape("\\!1337")=="\xb9\x0a");
        assert(unescape("\\+1337")=="\xf2\x14");
        assert(unescape("\\-1337")=="\xf1\x14");
        assert(unescape("\\!31337")=="\xe9\xf4\x01");
        assert(unescape("\\+31337")=="\xd2\xe9\x03");
        assert(unescape("\\-31337")=="\xd1\xe9\x03");
        
        cerr << "SUCCESS\n";
        return 0;
    }
    catch (const char * error) {
        cerr << "Unexpected exception during test: " << error << "\n";
        return 1;
    }
}
