#!/bin/bash

# 查询 git 状态并打印
git status

# 询问用户是否提交
read -p "Confirm Submission ? (y/n): " answer

if [ "$answer" = "y" ]; then
    git add .
    git commit -m "Please grade homework9 windows, this is possibly wrong"
    git push
else
    echo "Submission cancelled. Oh!"
fi