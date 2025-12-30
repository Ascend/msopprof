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

#ifndef __PARSER_TOKENS_H__
#define __PARSER_TOKENS_H__

#include <string>

namespace Parser {

class TokenS {
public:
    TokenS(void) : argc_{0}, argv_{nullptr} { }
    TokenS(int argc, char const* const argv[]) : argc_{argc}, argv_{argv} { }

    bool Eos(void) const
    {
        return argc_ <= 0 || argv_ == nullptr || argv_[0] == nullptr;
    }

    std::string Get(void) const
    {
        return std::string(argv_[0]);
    }

    TokenS Consume(int count) const
    {
        return TokenS{argc_ - count, argv_ + count};
    }

private:
    int argc_;
    char const *const *argv_;
};

}  // namespace Parser

#endif  // __PARSER_TOKENS_H__
