/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <stdio.h>

#define PATH "/usr/bin/deepin-editor"

int main(int argc, char *argv[])
{
    FILE *fp;

    std::string command = PATH;
    command.push_back(' ');
    for (int i = 1; i < argc; ++i) {
        command.append(argv[i]);
        command.push_back(' ');
    }

    fp = popen(command.c_str(), "r");
    pclose(fp);

    return 0;
}
