#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import platform
import unittest
from unittest.mock import patch
from utils import check_helper


class TestCheckHelper(unittest.TestCase):
    def test_check_path_length_valid(self):
        self.assertTrue(check_helper.check_path_length_valid('test' * 4))
        self.assertFalse(check_helper.check_path_length_valid('test' * 100))

    def test_check_path_pattern_valid(self):
        self.assertRaises(ValueError, check_helper.check_path_pattern_valid, '$sdfaf')
        self.assertIsNone(check_helper.check_path_pattern_valid('sdfaf'))
        with patch('platform.system', return_value='windows'):
            self.assertRaises(ValueError, check_helper.check_path_pattern_valid, '$sdfaf')
            self.assertIsNone(check_helper.check_path_pattern_valid('sdfaf'))

    def test_check_is_subdirectory(self):
        dir_a = './one'
        dir_b = './one/two'
        self.assertTrue(check_helper.check_is_subdirectory(dir_a, dir_b))
        dir_b = './two'
        self.assertFalse(check_helper.check_is_subdirectory(dir_a, dir_b))

    def test_check_type(self):
        self.assertRaises(ValueError, check_helper.check_type, 'test', 1, str)
        self.assertIsNone(check_helper.check_type('test', 1, int))

    def test_check_natural_number(self):
        self.assertRaises(ValueError, check_helper.check_natural_number, 'test', -1)
        self.assertIsNone(check_helper.check_natural_number('test', 0))

    def test_check_positive_number(self):
        self.assertRaises(ValueError, check_helper.check_positive_number, 'test', 0)
        self.assertIsNone(check_helper.check_positive_number('test', 1))

    def test_check_shape(self):
        self.assertRaises(ValueError, check_helper.check_shape, 1)
        self.assertRaises(ValueError, check_helper.check_shape, [0.5, 1])
        self.assertRaises(ValueError, check_helper.check_shape, [1, -1])
        self.assertIsNone(check_helper.check_shape([1, 1]))
