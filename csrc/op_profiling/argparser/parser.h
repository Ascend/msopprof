/* -------------------------------------------------------------------------
 *  This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */

#ifndef __PARSER_PARSER_H__
#define __PARSER_PARSER_H__

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "result.h"
#include "common/prof_args.h"

namespace Parser {

constexpr std::size_t shortNamePrefixLen = 1;
constexpr std::size_t longNamePrefixLen = 2;

struct Parser {
    virtual Either Parse(TokenS const &stream) const = 0;
};

struct Switch : public Parser {
    Switch(char shortName, std::string longName, bool &value)
        : value_(value), shortName_(shortName), longName_(std::move(longName)) { }
    inline Either Parse(TokenS const &stream) const override;

private:
    inline Either ParseShortName(TokenS const &stream, std::string const &arg) const;
    inline Either ParseLongName(TokenS const &stream, std::string const &arg) const;

private:
    bool& value_;
    char shortName_;
    std::string longName_;
};

Either Switch::Parse(TokenS const &stream) const
{
    if (stream.Eos()) {
        return Either::Left({ErrorType::ParseEos, "unexpected eos"});
    }
    std::string arg = stream.Get();
    if (arg.length() < shortNamePrefixLen || arg[0] != '-') {
        return Either::Left({ErrorType::ParseNonOption, "unexpected non-option argument " + arg});
    }
    if (arg.length() < longNamePrefixLen || arg[1] != '-') {
        return ParseShortName(stream, arg.substr(shortNamePrefixLen));
    } else {
        return ParseLongName(stream, arg.substr(longNamePrefixLen));
    }
}

Either Switch::ParseShortName(TokenS const &stream, std::string const &arg) const
{
    if (shortName_ == '\0') {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument -" + arg});
    }
    if (arg.length() == 1 && arg[0] == shortName_) {
        value_ = true;
        return Either::Right(stream.Consume(1));
    } else {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument -" + arg});
    }
}

Either Switch::ParseLongName(TokenS const &stream, std::string const &arg) const
{
    if (longName_.empty()) {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument --" + arg});
    } else if (arg == longName_) {
        value_ = true;
        return Either::Right(stream.Consume(1));
    } else {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument --" + arg});
    }
}

template<typename T>
struct Option : public Parser {
    Option(char shortName, std::string longName, std::string meta, T &value)
        : value_(value), shortName_(shortName), longName_(std::move(longName)),
        meta_(std::move(meta)) { }
    inline Either Parse(TokenS const &stream) const override;

private:
    inline Either ParseShortName(TokenS const &stream, std::string const &arg) const;
    inline Either ParseLongName(TokenS const &stream, std::string const &arg) const;

private:
    T &value_;
    char shortName_;
    std::string longName_;
    std::string meta_;
};

template<typename T>
Either Option<T>::Parse(const TokenS &stream) const
{
    if (stream.Eos()) {
        return Either::Left({ErrorType::ParseEos, "unexpected eos"});
    }
    std::string arg = stream.Get();
    if (arg[0] != '-') {
        return Either::Left({ErrorType::ParseNonOption, "unexpected non-option argument " + arg});
    }
    if (arg[1] == '-') {
        return ParseLongName(stream, arg.substr(longNamePrefixLen));
    } else {
        return ParseShortName(stream, arg.substr(shortNamePrefixLen));
    }
}

template<typename T>
Either Option<T>::ParseShortName(TokenS const &stream, std::string const &arg) const
{
    if (shortName_ == '\0') {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument -" + arg});
    }
    if (arg.length() == 1 && arg[0] == shortName_) {
        TokenS s = stream.Consume(1);
        if (s.Eos()) {
            return Either::Left({ErrorType::ParseLeakValue, "argument -" + arg + " miss value"});
        }
        if (ParseValue(s.Get(), value_)) {
            return Either::Right(s.Consume(1));
        } else {
            return Either::Left({ErrorType::ParseInvalidValue, "argument -" + arg + " " + s.Get() + " is invalid"});
        }
    } else {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument -" + arg});
    }
}

template<typename T>
Either Option<T>::ParseLongName(TokenS const &stream, std::string const &arg) const
{
    if (longName_.empty()) {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument --" + arg});
    }

    auto equal = arg.find_first_of('=');
    if (arg.substr(0, equal) != longName_) {
        return Either::Left({ErrorType::ParseMissMatch, "unexpected argument --" + arg});
    } else if (equal == std::string::npos) {
        return Either::Left({ErrorType::ParseLeakValue, "argument --" + arg + " miss value"});
    } else {
        if (ParseValue(arg.substr(equal + 1), value_)) {
            return Either::Right(stream.Consume(1));
        } else {
            return Either::Left({ErrorType::ParseInvalidValue, "argument --" + arg + " is invalid"});
        }
    }
}

class ArgParser {
public:
    inline ArgParser(std::string command, std::string description);
    template<typename ParserT>
    inline void Add(ParserT const &arg);
    inline Either Parse(TokenS const &stream, Common::ProfArgs &config) const;
private:
    inline Either ParseEach(TokenS const &stream) const;
    inline void ParseNoneOption(TokenS &stream, Common::ProfArgs &config) const;
private:
    std::string command_;
    std::string description_;
    std::vector<std::unique_ptr<Parser>> parsers_;
};

ArgParser::ArgParser(std::string command, std::string description)
    : command_{std::move(command)}, description_{description} { }

template<typename ParserT>
void ArgParser::Add(ParserT const &arg)
{
    parsers_.emplace_back(new ParserT(arg));
}

void ArgParser::ParseNoneOption(TokenS &stream, Common::ProfArgs &config) const
{
    while (!stream.Eos()) {
        config.argApps += stream.Get();
        stream = stream.Consume(1);
        if (!stream.Eos()) {
            config.argApps += " ";
        }
    }
}

Either ArgParser::Parse(const TokenS &stream, Common::ProfArgs &config) const
{
    TokenS s{stream};
    while (!s.Eos()) {
        std::string args = s.Get();
        if (args[0] != '-') {  // none option
            ParseNoneOption(s, config);
            return Either::Right(s);
        }
        Either ret = ParseEach(s);
        if (ret.Valid()) {
            s = ret.Right();
            continue;
        }
        return ret;
    }
    return Either::Right(s);
}

Either ArgParser::ParseEach(const TokenS &stream) const
{
    Either ret = Either::Right(stream);
    for (auto const &p : parsers_) {
        ret = p->Parse(stream);
        if (ret.Valid()) {
            return ret;
        }
        Error error = ret.Left();
        if (error.type != ErrorType::ParseMissMatch) {
            return ret;
        }
    }
    return ret;
}

};

#endif  // __PARSER_PARSER_H__
