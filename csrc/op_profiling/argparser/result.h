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

#ifndef __PARSER_RESULT_H__
#define __PARSER_RESULT_H__

#include <string>

#include "tokens.h"

namespace Parser {

enum class ErrorType {
    ParseSuccess = 0,  // Parsing success
    ParseNonOption,    // Parsing with a non-option argument. It should be skipped
    ParseMissMatch,    // Parsing with an unrecognized argument. Parsing should stop
    ParseLeakValue,    // Option leaks with value. Parsing should stop
    ParseInvalidValue, // Option with an invalid value. Parsing should stop
    ParseEos           // Parsing with end-of-stream
};

struct Error {
    ErrorType type;
    std::string msg;
};

class Either {
public:
    static Either Left(Error const &e)
    {
        return {false, e, TokenS{}};
    }
    static Either Right(TokenS const &v)
    {
        return {true, Error{}, v};
    }
    bool Valid(void) const
    {
        return valid_;
    }
    Error const& Left(void) const
    {
        return left_;
    }
    TokenS const& Right(void) const
    {
        return right_;
    }

private:
    Either(bool valid, Error const& left, TokenS const& right) : valid_(valid), left_(left), right_(right) { }

private:
    bool valid_;
    Error left_;
    TokenS right_;
};

};

#endif  // __PARSER_RESULT_H__
