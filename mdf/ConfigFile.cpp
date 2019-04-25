// ConfigFile.cpp: implementation of the ConfigFile class.
//
//////////////////////////////////////////////////////////////////////

#include "../../include/mdf/mapi.h"
#include "../../include/mdf/ConfigFile.h"
#include <stdlib.h>

using namespace std;
namespace mdf {
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

    ConfigFile::ConfigFile() {
        m_strName = "";
        m_pFile = NULL;
        m_exeDir = new char[2048];
        memset(m_exeDir, 0, 2048);
        GetExeDir(m_exeDir, 2048);
    }

    ConfigFile::ConfigFile(const char* strName) {
        m_strName = "";
        m_pFile = NULL;
        m_exeDir = new char[2048];
        memset(m_exeDir, 0, 2048);
        GetExeDir(m_exeDir, 2048);
        mdf_assert(ReadConfig(strName));
    }

    ConfigFile::~ConfigFile() {
        Save();
        MDF_SAFE_DELETE_ARRAY(m_exeDir)
    }

    bool ConfigFile::ReadConfig(const char* fileName, bool force) {
        if (!force) {
            if ("" != m_strName) return false; //已经读取了配置
        }
        if (NULL == fileName) return false;
        m_strName = m_exeDir;
        m_strName += "/";
        m_strName += fileName;
        if (!ReadFile()) {
            m_strName = "";
            return false;
        }

        return true;
    }

//关闭文件
    void ConfigFile::Close() {
        if (NULL == m_pFile) return;
        fclose(m_pFile);
        m_pFile = NULL;
    }

//打开文件
    bool ConfigFile::Open(bool bRead) {
        Close();
        if (bRead)
            m_pFile = fopen(m_strName.c_str(), "r");
        else
            m_pFile = fopen(m_strName.c_str(), "w");

        return NULL != m_pFile;
    }

    bool ConfigFile::ReadFile() {
        if (!Open(true)) return false; //以读方式打开文件

        fseek(m_pFile, SEEK_SET, 0);
        char lpszLine[4096];
        string line;
        string name;
        string value;
        string m_description = "";
        CFGItem item;
        CFGSection section(NULL, 0);

        int nPos = 0;
        while (NULL != fgets(lpszLine, 4096, m_pFile)) {
            line = lpszLine;
            TrimString(line, "\t \r\n"); //删除换行符空白符
            if ("" == line) continue; //跳过空行
            //跳过注释行
            if ('#' == line.c_str()[0] || ('/' == line.c_str()[0] && '/' == line.c_str()[1]) || ('\\' == line.c_str()[0] && '\\' == line.c_str()[1])) {
                m_description += lpszLine;
                continue;
            }

            //重新载入行，保留名字中的空白符
            line = lpszLine;
            TrimString(line, "\r\n"); //删除换行符
            if ("" == line) continue; //跳过空行

            //段标志
            if ('[' == line.c_str()[0] && ']' == line.c_str()[line.size() - 1]) {
                if ('/' == line.c_str()[1]) //段读取完成
                {
                    //m_sections.insert(CFGSectionMap::value_type(section.m_name, section));
                    m_sections[section.m_name] = section;
                    continue;
                }
                //读取新段
                name.assign(line.begin() + 1, line.begin() + line.size() - 1);
                TrimStringRight(name, " \t");
                TrimStringLeft(name, " \t");
                section.m_name = name;
                section.m_index = (int) m_sections.size();
                section.m_content.clear();
                section.m_description = m_description;
                m_description = "";
                continue;
            }

            //读取配置参数
            nPos = (int) line.find('=', 0);
            if (-1 == nPos) continue;
            int nEnd = (int) (line.size() - 1);
            if (nPos >= nEnd) continue;
            name.assign(line.begin(), line.begin() + nPos);
            TrimStringRight(name, " \t");
            TrimStringLeft(name, " \t");

            value.assign(line.begin() + nPos + 1, line.end());
            TrimStringRight(value, " \t");
            TrimStringLeft(value, " \t");
            item = value;
            item.m_description = m_description;
            item.m_index = (int) section.m_content.size();
            item.m_valid = true;
            //section.m_content.insert(ConfigMap::value_type(name, item));
            section.m_content[name] = item;
            m_description = "";
        }
        Close();

        return true;
    }

    CFGSection::CFGSection() {

    }

    CFGSection::CFGSection(const char* name, int index) {
        m_name = "";
        m_index = -1;
        if (NULL != name) m_name = name;
        if (0 <= index) m_index = index;
    }

    CFGSection::~CFGSection() {

    }

    void CFGSection::SetDescription(const char* description) {
        m_description = "";
        if (NULL != description) {
            string des = description;
            int start = 0;
            string line;
            int pos = 0;
            m_description = "";
            while (start < (int) des.size()) {
                pos = (int) des.find("\n", start);
                if (-1 == pos) {
                    line.assign(des.begin() + start, des.end());
                } else
                    line.assign(des.begin() + start, des.begin() + pos);
                m_description += "//";
                m_description += line;
#ifdef WIN32
                m_description += "\n";
#else
                m_description += "\r\n";
#endif
                start = pos + 1;
                if (-1 == pos) break;

            }
        }
    }

    CFGItem& CFGSection::operator[](std::string key) {
        mdf_assert("" != key);
        ConfigMap::iterator it = m_content.find(key);
        if (it == m_content.end()) {
            CFGItem item;
            item.m_index = (int) m_content.size();
            m_content.insert(ConfigMap::value_type(key, item));
            it = m_content.find(key);
            mdf_assert(it != m_content.end());
        }
        return it->second;
    }

    bool CFGSection::Save(FILE* pFile) {
        /*
         写入段头
         //段注释
         格式:[段名]
         */
#ifdef WIN32
        fprintf( pFile, "%s", m_description.c_str() );
        fprintf( pFile, "[%s]\n", m_name.c_str() );
#else
        fprintf(pFile, "%s", m_description.c_str());
        fprintf(pFile, "[%s]\r\n", m_name.c_str());
#endif

        /*
         写入配置参数
         格式:
         //参数注释
         参数1=value1
         //参数注释
         参数2=value2
         */
        ConfigMap::iterator it;
        int i = 0;
        int count = (int) m_content.size();
        for (i = 0; i < count; i++) {
            for (it = m_content.begin(); it != m_content.end(); it++) {
                if (i != it->second.m_index) continue;
                fprintf(pFile, "%s", it->second.m_description.c_str());
                fprintf(pFile, "\t%s=%s", it->first.c_str(), it->second.m_value.c_str());
#ifdef WIN32
                fprintf( pFile, "\n\n" );
#else
                fprintf(pFile, "\r\n\r\n");
#endif
                break;
            }
        }

        /*
         写入段尾
         格式:[/段名]
         */
#ifdef WIN32
        fprintf( pFile, "[/%s]\n\n\n", m_name.c_str() );
#else
        fprintf(pFile, "[/%s]\r\n\r\n\r\n", m_name.c_str());
#endif
        return true;
    }

    CFGSection& ConfigFile::operator[](std::string name) {
        mdf_assert("" != name);
        CFGSectionMap::iterator it = m_sections.find(name);
        if (it == m_sections.end()) {
            CFGSection sections(name.c_str(), (int) m_sections.size());
            m_sections.insert(CFGSectionMap::value_type(name, sections));
            it = m_sections.find(name);
            mdf_assert(it != m_sections.end());
        }
        return it->second;
    }

    bool ConfigFile::Save() {
        if (!Open(false)) return false; //以写方式打开文件

        fseek(m_pFile, SEEK_SET, 0);
        string line;
        CFGItem item;
        CFGSectionMap::iterator itSection;
        int i = 0;
        int count = (int) m_sections.size();
        string name;

        ConfigMap::iterator itItem;
        //int itemIndex = 0;

        for (i = 0; i < count; i++) {
            for (itSection = m_sections.begin(); itSection != m_sections.end(); itSection++) {
                if (i != itSection->second.m_index) continue;
                itSection->second.Save(m_pFile);
                break;
            }
        }

        Close();

        return true;
    }

    CFGItem::CFGItem() {
        m_value = "";
        m_description = "";
        m_index = -1;
        m_valid = false;
    }

    CFGItem::~CFGItem() {
    }

    CFGItem::operator std::string() {
        return m_value;
    }

    CFGItem& CFGItem::operator=(double value) {
        char strValue[128];
        sprintf(strValue, "%f", value);
        m_value = strValue;
        TrimStringRight(m_value, "0");
        m_valid = true;
        return *this;
    }

    CFGItem& CFGItem::operator=(int value) {
        char strValue[128];
        sprintf(strValue, "%d", value);
        m_value = strValue;
        m_valid = true;
        return *this;
    }

    CFGItem& CFGItem::operator=(string value) {
        m_value = value;
        m_valid = true;
        return *this;
    }

    CFGItem::operator char() {
        return (char) atoi(m_value.c_str());
    }

    CFGItem::operator unsigned char() {
        return (unsigned char) atoi(m_value.c_str());
    }

    CFGItem::operator short() {
        return (short) atoi(m_value.c_str());
    }

    CFGItem::operator unsigned short() {
        return (unsigned short) atoi(m_value.c_str());
    }

    CFGItem::operator int() {
        return atoi(m_value.c_str());
    }

    CFGItem::operator unsigned int() {
        return (unsigned int) atoi(m_value.c_str());
    }

    CFGItem::operator int64() {
        int64 nInt;
        sscanf(m_value.c_str(), "%ld", &nInt);
        return nInt;
    }

    CFGItem::operator uint64() {
        uint64 uInt;
        sscanf(m_value.c_str(), "%lu", &uInt);
        return uInt;
    }

    CFGItem::operator float() {
        return (float) atof(m_value.c_str());
    }

    CFGItem::operator double() {
        return atof(m_value.c_str());
    }

    bool CFGItem::IsNull() {
        return !m_valid;
    }

    void CFGItem::SetDescription(const char* description) {
        m_description = "";
        if (NULL != description) {
            string des = description;
            int start = 0;
            string line;
            int pos = 0;
            m_description = "";
            while (start < (int) des.size()) {
                pos = (int) des.find("\n", start);
                if (-1 == pos) {
                    line.assign(des.begin() + start, des.end());
                } else
                    line.assign(des.begin() + start, des.begin() + pos);
                m_description += "\t//";
                m_description += line;
#ifdef WIN32
                m_description += "\n";
#else
                m_description += "\r\n";
#endif
                start = pos + 1;
                if (-1 == pos) break;

            }
        }
    }

} //namespace mdf
