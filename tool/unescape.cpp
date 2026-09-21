/*******************************************************************************
 *  Resource management library
 *  Escaped string processing
 *  
 *  © 2026, Sauron
 ******************************************************************************/

#include <string>

using std::string;

static void appendVariableInteger(string &output, unsigned long long value) {
    while (value>=0x80) {
        output.push_back(0x80|(0x7f&value));
        value>>=7;
    }
    output.push_back(char(value));
}

static void appendVariableInteger(string &output, const string &valueStr) {
    try {
        appendVariableInteger(output, std::stoull(valueStr));
    }
    catch (...) {
        throw "integer value is out of range";
    }
}

static void appendVariableIntegerZigzag(string &output, const string &valueStr) {
    try {
        signed long long value=std::stoll(valueStr);
        appendVariableInteger(output, value>=0?(value<<1):(value<<1)^(~0));
    }
    catch (...) {
        throw "integer value is out of range";
    }
}

static const signed char HEX_TABLE[256]={
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
    0,  10, 11, 12, 13, 14, 15, 0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  10, 11, 12, 13, 14, 15, 0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

string unescape(const string &input) {
    string result;
    string buffer;
    enum {
        NORMAL,
        ESCAPE,
        UNICODE_START,
        HEXADECIMAL_START,
        HEXADECIMAL,
        ESCAPE_FRAGMENT,
        VINT_START,
        VINT,
        ZINT_START,
        ZINT,
        VSTRING,
        VSTRING_ESCAPE
    } state=NORMAL;
    
    for (size_t i=0; i<input.size(); i++) {
        char c=input[i];
        bool start=false;
        if (state==NORMAL) {
            start=true;
        }
        else if (state==ESCAPE) {
            state=NORMAL;
            if (c=='!') {
                state=VINT_START;
                buffer.clear();
            }
            else if (c=='+') {
                state=ZINT_START;
                buffer.clear();
            }
            else if (c=='-') {
                state=ZINT_START;
                buffer='-';
            }
            else if (c=='0')
                result+='\0';
            else if (c=='a')
                result+='\a';
            else if (c=='b')
                result+='\b';
            else if (c=='e')
                result+='\e';
            else if (c=='f')
                result+='\f';
            else if (c=='n')
                result+='\n';
            else if (c=='r')
                result+='\r';
            else if (c=='t')
                result+='\t';
            else if (c=='u'||c=='U')
                state=UNICODE_START;
            else if (c=='v')
                result+='\v';
            else if (c=='x'||c=='X')
                state=HEXADECIMAL_START;
            else if (c=='`')
                state=ESCAPE_FRAGMENT;
            else if (c=='[') {
                state=VSTRING;
                buffer.clear();
            }
            else if (isalpha(c))
                throw "unknown escape sequence";
            else
                result+=c;
        }
        else if (state==UNICODE_START)
            throw "unicode sequences are not implemented";
        else if (state==HEXADECIMAL_START) {
            if (isxdigit(c)) {
                //char character=HEX_TABLE[(unsigned char)c];
                state=HEXADECIMAL;
            }
            else
                throw "\\x used with no following hex digits";
        }
        else if (state==HEXADECIMAL)
            throw "not implemented yet";
        else if (state==ESCAPE_FRAGMENT)
            throw "fragments are not implemented";
        else if (state==VINT_START) {
            if (isdigit(c)) {
                buffer+=c;
                state=VINT;
            }
            else
                throw "expected decimal digit after \\!";
        }
        else if (state==VINT) {
            if (isdigit(c))
                buffer+=c;
            else {
                appendVariableInteger(result, buffer);
                state=NORMAL;
                start=true;
            }
        }
        else if (state==ZINT_START) {
            if (isdigit(c)) {
                buffer+=c;
                state=ZINT;
            }
            else
                throw "expected decimal digit after \\+ or \\-";
        }
        else if (state==ZINT) {
            if (isdigit(c))
                buffer+=c;
            else {
                appendVariableIntegerZigzag(result, buffer);
                state=NORMAL;
                start=true;
            }
        }
        else if (state==VSTRING) {
            if (c=='\\')
                state=VSTRING_ESCAPE;
            else if (c==']') {
                appendVariableInteger(result, buffer.length());
                result+=buffer;
            }
            else
                state=NORMAL;
        }
        else if (state==VSTRING_ESCAPE) {
            buffer+=c;
            state=VSTRING;
        }
        
        if (start) {
            if (c=='\\')
                state=ESCAPE;
            else
                result+=c;
        }
    }
    
    if (state==VINT)
        appendVariableInteger(result, buffer);
    else if (state==ZINT)
        appendVariableIntegerZigzag(result, buffer);
    else if (state!=NORMAL)
        throw "uninished escape sequence";
    
    return result;
}
