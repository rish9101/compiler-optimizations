#include "Common.h"

// The following code may be useful for both of your passes:
// If you recall, there is no "get the variable on the left
// hand side" function in LLVM. Normally this is fine: we
// just call getName(). This does not work, however, for
// instructions that are publically 'unnamed', but internally
// assigned a name like '%0' or '%1'. To get these names, the
// following code does some really nasty stuff. It also grabs
// raw integer values so our expressions can look a little
// cleaner.
//
// Feel free to improve this code if you want nicer looking
// results. For example, ConstantInts are the only pretty
// printed Constants.
//
// Sadly, this code is a great example of not doing things
// 'the LLVM way', especially since we're using std::string.
// I encourage you to think of a way to make this code nicer
// and let me know :)
std::string getShortValueName(Value *v)
{
    if (v->getName().str().length() > 0)
    {
        return "%" + v->getName().str();
    }
    else if (isa<Instruction>(v))
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        v->print(*strm);
        std::string inst = strm->str();
        size_t idx1 = inst.find("%");
        size_t idx2 = inst.find(" ", idx1);
        if (idx1 != std::string::npos && idx2 != std::string::npos)
        {
            return inst.substr(idx1, idx2 - idx1);
        }
        else
        {
            return "\"" + inst + "\"";
        }
    }
    else if (ConstantInt *cint = dyn_cast<ConstantInt>(v))
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        cint->getValue().print(*strm, true);
        return strm->str();
    }
    else
    {
        std::string s = "";
        raw_string_ostream *strm = new raw_string_ostream(s);
        v->print(*strm);
        std::string inst = strm->str();
        return "\"" + inst + "\"";
    }
}