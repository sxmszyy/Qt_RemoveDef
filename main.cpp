#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStack>

#define DEFINESIZE 7
#define IFDEFSIZE 6
#define IFNDEFSIZE 7

void AddDefine(QString line, QStringList *defs) {
    int begin = line.indexOf("`define");
    //截取define，去除空格
    QString define = line.mid(begin + DEFINESIZE, -1).simplified();
    //去除多余空格，只留一个
    define.replace(QRegExp("[\\s]+"), " ");
    QStringList list = define.split(" ");
    defs->append(list[0]);
    printf("`define %s\n", list[0].toStdString().data());
}

bool JudgeIf(QString line, QStringList *defs) {
    //是ifdef还是ifndef
    bool isornot = false;
    //define是否存在
    bool isexist = false;
    if (line.contains("`ifdef", Qt::CaseSensitive)){
        isornot = true;
    }
    else if (line.contains("`ifndef", Qt::CaseSensitive)) {
        isornot = false;
    }
    int begin;
    QString define;
    if (isornot) {
        begin = line.indexOf("`ifdef");
        define = line.mid(begin + IFDEFSIZE, -1).simplified();
    }
    else {
        begin = line.indexOf("`ifndef");
        define = line.mid(begin + IFNDEFSIZE, -1).simplified();
    }
    define = line.mid(begin + DEFINESIZE, -1).simplified();
    define.replace(QRegExp("[\\s]+"), " ");
    QStringList list = define.split(" ");

    for (int i = 0; i < defs->length(); i++) {
        if ((*defs)[i] == list[0]) {
            isexist = true;
            break;
        }
    }
    if (isexist == isornot) {
        return true;
    }
    else {
        return false;
    }
}

bool ReadWriteFile(QString inputfile, QString outputfile, QStringList *defs) {
    QFile infile(inputfile);
    if (infile.exists()) {
        if(!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            printf("文件 %s 不可读\n", inputfile.toStdString().data());
            return false;
        }
        else {
            QFile outfile(outputfile);
            if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                printf("文件 %s 不可读\n", outputfile.toStdString().data());
                return false;
            }
            QStack <bool> ifelsevalue;
            //当前内容是否define
            bool flag = true;
            //当前内容是否为注释内容
            bool _flag = true;
            int ifnum = 0;
            QString nextline = "";
            QString original = infile.readLine();
            QString line;
            while(!original.isEmpty()) {
                line = original;
                //去除tab键
                line = line.replace("\t", " ").simplified();
                //是否在/**/范围内
                if (!_flag) {
                    if (line.contains("*/", Qt::CaseSensitive)) {
                        _flag = true;
                    }
                    QString temp = line.mid(line.indexOf("*/") + 2, - 1);
                    if (!temp.isEmpty()) {
                        original = temp + "\n";
                        continue;
                    }
                    /*else {
                        original = infile.readLine();
                        continue;
                    }*/
                }
                //只处理在开头的"//"
                else if (line.indexOf("//") == 0) {
                    /*original = infile.readLine();
                    continue;*/
                }
                //处理"/*"
                else if (line.contains("/*", Qt::CaseSensitive)) {
                    QString temp = line.mid(0, line.indexOf("/*"));
                    if (temp.isEmpty()) {
                        _flag = false;
                        original = line.mid(line.indexOf("/*") + 2, - 1);
                        if (!original.isEmpty()) {
                            continue;
                        }
                        /*else {
                            original = infile.readLine();
                            continue;
                        }*/
                    }
                    else {
                        original = temp;
                        nextline = line.mid(line.indexOf("/*"), - 1);
                        continue;
                    }
                }
                //找define
                else if (line.contains("`define", Qt::CaseSensitive)) {
                    if (flag) {
                        AddDefine(line, defs);
                        outfile.write(original.toLatin1());
                    }
                }
                else if (line.contains("`ifdef", Qt::CaseSensitive) || line.contains("`ifndef", Qt::CaseSensitive)) {
                    if (flag) {
                        ifelsevalue.push(JudgeIf(line, defs));
                        flag = ifelsevalue.top();
                    }
                    else {
                        //记录无效的if个数
                        ifnum++;
                    }
                }
                else if (line.contains("`elsif", Qt::CaseSensitive)) {
                    if (ifnum == 0) {
                        if (!ifelsevalue.top()) {
                            ifelsevalue.pop();
                            ifelsevalue.push(JudgeIf(line, defs));
                            flag = ifelsevalue.top();
                        }
                        else {
                            flag = false;
                        }
                    }
                    /*else {
                        original = infile.readLine();
                        continue;
                    }*/
                }
                else if (line.contains("`else", Qt::CaseSensitive)) {
                    if (ifnum == 0) {
                        ifelsevalue.push(!ifelsevalue.pop());
                        flag = ifelsevalue.top();
                    }
                    /*else {
                        original = infile.readLine();
                        continue;
                    }*/
                }
                else if (line.contains("`endif", Qt::CaseSensitive)) {
                    if (ifnum == 0) {
                        ifelsevalue.pop();
                        if (ifelsevalue.isEmpty()) {
                            flag = true;
                        }
                        else {
                            flag = ifelsevalue.top();
                        }
                    }
                    else {
                        ifnum--;
                        /*original = infile.readLine();
                        continue;*/
                    }
                }
                else {
                    if (flag) {
                        outfile.write(original.toLatin1());
                    }
                }

                if (nextline == "") {
                    original = infile.readLine();
                }
                else {
                    original = nextline;
                    nextline = "";
                }
            }
            printf("\u751f\u6210\u6587\u4ef6 %s\n", outfile.fileName().toStdString().data());
            outfile.close();
        }
        infile.close();
    }
    else {
        printf("文件 %s 不存在\n", inputfile.toStdString().data());
        return false;
    }
    return true;
}

bool AnalysisFilelist(QString filelist) {
    QFile file(filelist);
    if (file.exists()) {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            printf("filelist不可读\n");
            return false;
        }

        //收集defs
        QStringList defs;
        QString inputfile = file.readLine();
        while(!inputfile.isEmpty()) {
            inputfile = inputfile.simplified();
            QFileInfo fileinfo(inputfile);
            QString outputfile = fileinfo.path() + "/" + fileinfo.baseName() + "_worked." + fileinfo.suffix();

            if (ReadWriteFile(inputfile, outputfile, &defs)) {
            }
            else {
                printf("读取文件 %s 失败\n", inputfile.toStdString().data());
            }

            inputfile = file.readLine().simplified();
        }
        file.close();
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("请输入filelist\n");
        return 0;
    }

    QString filelist = argv[1];
    if (!AnalysisFilelist(filelist)) {
        printf("文件解析失败n");
    }

    return 0;
}