#!/bin/bash
cp ".transifexrc" ${HOME}/

cd ./translations

#rm -f dde-launcher.ts
#根据源码生成翻译英文翻译文件
lupdate ../src/ -ts -no-obsolete dde-application-manager.ts

cd ../

lupdate ./ -ts -no-obsolete translations/dde-application-manager.ts

tx push -s -b m23
