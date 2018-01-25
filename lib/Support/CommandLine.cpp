
#include "enfield/Support/CommandLine.h"
#include "enfield/Support/Defs.h"

#include <cstdlib>

#include <iostream>
#include <iomanip>

#include <map>
#include <vector>
#include <memory>

using namespace efd;

namespace efd {

    class ArgsParser {
        public:
            std::map<std::string, std::vector<OptBase*> > mArgMap;

            ArgsParser() {}

            bool hasOpt(OptBase* opt);
            void addOpt(OptBase* opt);
            void delOpt(OptBase* opt);
    };

};

bool efd::ArgsParser::hasOpt(OptBase* opt) {
    return mArgMap.find(opt->mName) != mArgMap.end();
}

void efd::ArgsParser::addOpt(OptBase* opt) {
    std::string name(opt->mName);

    if (!hasOpt(opt)) {
        mArgMap.insert(std::make_pair(name, std::vector<OptBase*>()));
    }

    mArgMap[name].push_back(opt);
}

void efd::ArgsParser::delOpt(OptBase* opt) {
    if (hasOpt(opt)) {
        std::vector<OptBase*> &optVector = mArgMap[opt->mName];

        if (optVector.size() == 1)
            mArgMap.erase(mArgMap.find(opt->mName));
        else {
            for (auto it = optVector.begin(), e = optVector.end(); it != e; ++it)
                if (opt == *it) {
                    optVector.erase(it);
                    break;
                }
        }

    }
}

static efd::Opt<bool> PrintHelp
("help", "Prints a list with the available commands.", false, false);

static std::shared_ptr<ArgsParser> GetParser() {
    static std::shared_ptr<ArgsParser> Parser(new ArgsParser());
    return Parser;
}

template <>
void efd::Opt<bool>::parseImpl(std::vector<std::string> args) {
    mVal = !mVal;
}

template <>
uint32_t efd::Opt<bool>::argsConsumed() {
    return 0;
}

template <>
std::string efd::Opt<bool>::getStringVal() {
    if (mVal) return "true";
    else return "false";
}

template <>
void efd::Opt<int>::parseImpl(std::vector<std::string> args) {
    mVal = std::stoi(args[0]);
}

template <>
void efd::Opt<uint32_t>::parseImpl(std::vector<std::string> args) {
    mVal = std::stoul(args[0]);
}

template <>
void efd::Opt<long long>::parseImpl(std::vector<std::string> args) {
    mVal = std::stoll(args[0]);
}

template <>
void efd::Opt<unsigned long long>::parseImpl(std::vector<std::string> args) {
    mVal = std::stoull(args[0]);
}

template <>
void efd::Opt<float>::parseImpl(std::vector<std::string> args) {
    mVal = std::stof(args[0]);
}

template <>
void efd::Opt<double>::parseImpl(std::vector<std::string> args) {
    mVal = std::stod(args[0]);
}

template <>
void efd::Opt<std::string>::parseImpl(std::vector<std::string> args) {
    mVal = args[0];
}

template <>
std::string efd::Opt<std::string>::getStringVal() {
    return mVal;
}

template <>
void efd::Opt<std::vector<std::string>>::parseImpl(std::vector<std::string> args) {
    mVal.push_back(args[0]);
}

template <>
std::string efd::Opt<std::vector<std::string>>::getStringVal() {
    std::string str;
    for (auto s : mVal) str += s + "; ";
    return str;
}

efd::OptBase::OptBase(std::string name, std::string description, bool isRequired) : 
    mIsRequired(isRequired), mIsParsed(false), mName(name), mDescription(description) {

    mParser = GetParser();
    mParser->addOpt(this);
}

efd::OptBase::~OptBase() {
    mParser->delOpt(this);
}

bool efd::OptBase::isParsed() {
    return mIsParsed;
}

bool efd::OptBase::isRequired() {
    return mIsRequired;
}

void efd::OptBase::parse(std::vector<std::string> args) {
    parseImpl(args);
    mIsParsed = true;
}

static void PrintCommandLineHelp() {
    std::shared_ptr<ArgsParser> Parser = GetParser();

    std::cout << "Required:" << std::endl;

    const int nCols = 20;
    for (auto pair : Parser->mArgMap) {
        if (pair.second[0]->isRequired()) {
            std::cout << "\t" << std::left << std::setw(nCols) << "-" + pair.first;
            std::cout << pair.second[0]->mDescription << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Options:" << std::endl;

    for (auto pair : Parser->mArgMap) {
        if (!pair.second[0]->isRequired()) {
            std::cout << "\t" << std::left << std::setw(nCols) << "-" + pair.first;
            std::cout << pair.second[0]->mDescription << std::endl;
            std::cout << "\t" << std::left << std::setw(nCols) << " " <<
                "Default: " << pair.second[0]->getStringVal() << std::endl;
        }
    }
}

void efd::ParseArguments(int argc, char** argv) {
    const int cArgc = argc;
    const char** cArgv = (const char**) argv;
    efd::ParseArguments(cArgc, cArgv);
}

void efd::ParseArguments(const int argc, const char **argv) {
    std::shared_ptr<ArgsParser> Parser = GetParser();
    std::vector<std::string> rawArgs;

    for (int32_t i = 0; i < argc; ++i)
        rawArgs.push_back(argv[i]);

    for (int32_t i = 1; i < argc; ++i) {
        std::string arg(rawArgs[i]);

        // Removing the initial '-'
        if (arg[0] == '-') {
            arg = arg.substr(1);
        }

        if (Parser->mArgMap.find(arg) != Parser->mArgMap.end()) {
            std::vector<OptBase*>& optVector = Parser->mArgMap[arg];

            int32_t toBeConsumed = optVector[0]->argsConsumed();
            if (i + toBeConsumed >= argc) {
                ERR << "There should be " << toBeConsumed << " arguments for -" << arg
                    << ", but there was only " << argc - i << "." << std::endl;
                assert(false && "Not enough command line arguments.");
            }

            std::vector<std::string> optArgs;
            for (int32_t j = 0; j < toBeConsumed; ++j) {
                optArgs.push_back(rawArgs[i + 1 + j]);
            }

            for (OptBase *opt : optVector) {
                opt->parse(optArgs);
            }

            i += toBeConsumed;
        }
    }

    bool requirementsFulfilled = true;
    for (auto pair : Parser->mArgMap) {
        OptBase *opt = pair.second[0];
        if (opt->isRequired() && !opt->isParsed()) {
            requirementsFulfilled = false;
            break;
        }
    }

    if (PrintHelp.getVal() || !requirementsFulfilled) {
        PrintCommandLineHelp();
        exit(0);
    }
}
